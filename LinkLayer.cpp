/*
 * LinkLayer.cpp
 *
 *  Created on: 11.9.2012
 *      Author: kubanec
 */

#include "rfmIncludeCpp.h"

extern rfm::packet_t packet;

namespace rfm
{
extern Mailbox mbox;
static packet_t pakety[PACKET_BUFFER_LENGTH] __attribute__((aligned(sizeof(stkalign_t))));
MEMORYPOOL_DECL(packet_pool,sizeof(packet_t),NULL);

int8_t LinkLayer::SourceAddress = -1;
threads::SendThread * LinkLayer::thd_send;
threads::RecThread * LinkLayer::thd_rec;
LinkBuffer LinkLayer::buffer2;

void LinkLayer::Init(uint8_t address)
{
	if (SourceAddress != -1)
	{
		return;
	}

	rf_init();
	rf_fifo_reset();
	rf_writecmd(0);
	rf_writecmd(0);
	rf_writecmd(0);
	rf_sleep();

	if (address < 8)
	{
		SourceAddress = address;
		//rozběhnout vlákna
		thd_send = new threads::SendThread;
		thd_rec = new threads::RecThread();
	}

	buffer2.clear();
	chPoolLoadArray(&packet_pool, pakety, PACKET_BUFFER_LENGTH);
}

void LinkLayer::Init(uint8_t address, uint16_t listen)
{
	Init(address);
	thd_rec->SetSlaves(listen);
}

/**
 * vrátí true pokud se všecko povede
 * false pokud je mimo synchro nebo nedostal mutex
 *
 */
bool LinkLayer::SendPacket(packet_t * packet)
{
	if (IsSynchronized())
	{
		packet = AllocPacket(packet);
		if (chMBPost(&mbox, (msg_t) packet, TIME_IMMEDIATE ) == RDY_OK)
			return true;
	}
	return false;
}

bool LinkLayer::SendPacketI(packet_t * packet)
{
	if (IsSynchronized())
	{
		packet = AllocPacket(packet);
		if (chMBPostI(&mbox, (msg_t) packet) == RDY_OK)
			return true;
	}
	return false;
}

uint8_t LinkLayer::IsSynchronized()
{
	if (IsMaster())
		return 1;

	if (threads::RecThread::IsSynchronized())
		return 1;

	return 0;
}

/**
 * @brief callback from receiving thread
 *
 * @param deep copy of packet
 * @param checksum state
 *
 * @note It should not take so much time because it's called
 * directly from recieving thread. It could break the synchronization
 *
 * @note volá se přimo z přijimaciho vlákna a měl by byt co nejkratši
 *
 * pokud je v pořádku tak uvolnit paket, pokud ne tak si znova nechat poslat
 *
 * pokud master a paket je v pořádku tak ho uvolni aji z druhyho bufferu a uvolni ho uplně
 * zpracuje a připadně hodi callback pro usera
 *
 * pokud je paket chybné tak se znova zařadi do fronty a vytahne se z druhyho bufferu
 *
 * pokud se jedná o slava tak se druhej buffer vubec nepouživa protože ten je jenom
 * kvuli arq a to si řeši sám master, pokud je okej tak ho zpracuje, hodi callback a odpovi
 * masterovi co chce vědět
 *
 */
void LinkLayer::Callback(packet_t packet, bool checksumOk, uint8_t ListenAddr)
{
#ifdef DEBUG_RFM
	static uint32_t pole[200];
	static uint16_t idx = 0;
	static uint16_t dobry = 0, spatny = 0;

	if (idx > 30)
	{
		asm("nop");
	}
	else
	{
		if (checksumOk)
		dobry++;
		else
		spatny++;
		pole[idx++] = chibios_rt::System::GetTime();
		pole[idx++] = checksumOk;
	}
#endif

	/*
	 * spárování paketu co přišel s tim co je ve frontě
	 */
	packet_t * packet_ = buffer2.process(ListenAddr);

	if (checksumOk)
	{

		/*
		 * nechat paket zpracovat vyšší vrstvu
		 * a pak ho uvolnit
		 */
		ApplicationLayer::processPacket(&packet);
		FreePacket(packet_);
	}
	else
	{
		/*
		 * spárovat pakety packet + ten co je v buffer2
		 * a vytáhnout ho z buffer2 a hodit znova do fronty
		 * podle adresy z které to mělo přijit
		 *
		 * projit pakety v buffer2 a pokud tam najde paket s touhle adresou
		 * tak si ho vyptat znova, mělo by se začinat od začátku zásobníku a ne od
		 * konce - FIFO
		 *
		 * arq
		 */
		if (IsMaster())
		{
			if (packet_ != NULL)
			{
				if (chMBPostAhead(&mbox, (msg_t) packet_,
						TIME_IMMEDIATE ) != RDY_OK)
					ApplicationLayer::processArqErrorCb();
			}
		}
		else
		{
			/*
			 * slave jenom uvolni a kašle na to
			 * máster se ho někdy zeptá sám znova
			 */

			FreePacket(packet_);
		}

	}
}

/**
 * @brief callback jak dopadlo odesilání paketu
 */
void LinkLayer::CallbackSend(packet_t * packet_, bool ok)
{
	/**
	 * podařilo se poslat tak se hodi do druhyho bufferu
	 * pokud se nepodařilo poslat tak se hodi zpátky
	 * do mailboxu a pošle se znova - připad mastera
	 *
	 * druhej buffer kvuli tomu až přinde timeout že slave neposlal
	 * nebo se to někde ztratilo
	 * tak aby věděl co má poslat znova
	 *
	 * pokud slave tak to rovnou uvolni protože arq si řeši jenom master sám
	 */
	if (IsMaster())
	{
		if (ok)
		{
			buffer2.pushBack(packet_);
		}
		else
		{
			/*
			 * hodi paket znova do fronty ale čeká až bude prázdná,
			 * sice se rozhodi synchronizace ale bude 100% splněno
			 * arq
			 *
			 * nedá se to tak udělat protože vlákno by zamčelo samo
			 * sebe a už by se to nerozjelo
			 */
			if (chMBPostAhead(&mbox, (msg_t) packet_, TIME_IMMEDIATE ) != RDY_OK)
				ApplicationLayer::processArqErrorCb();
		}
	}
	else
	{
		/*
		 * slave - rovnou uvolnit paket a kašlat na to
		 */
		FreePacket(packet_);
	}
}

/**
 * @brief sem to skoči jenom a pouze pokud seš master
 * a od slave nepřinde žádná odpověď,
 * ve všech typech paketů a user bude vědět že jednotka neodpověděla
 *
 * @note volá se přimo z přijimaciho vlákna a měl by byt co nejkratši
 */
void LinkLayer::CallbackNok(uint8_t slaveAddress)
{
#ifdef DEBUG_RFM
	static systime_t pole[10];
	static uint16_t index = 0;
	pole[index++] = chibios_rt::System::GetTime();
	if (index > 10)
	{
		index = 0;
	}
#endif
	/*
	 * tady muže poznat podle toho co má ve frontě jesli to byl jenom idle
	 * paket nebo nějaké vic crucial a muže ho nechat zopakovat
	 *
	 * taky se dá využit buffer2 - projit ho celej jesli tam neni nějaké packet
	 * s touhle adresou a dyžtak si ho vyptat znova mělo by se začit od začátku zásobníku
	 * ne od konce FIFO
	 */
	packet_t * packet = buffer2.process(slaveAddress);

	/*
	 * pokud tam paket nebyl tak se posral asi jenom idle paket
	 */
	if (packet != NULL)
	{
		chMBPostAhead(&mbox, (msg_t) packet, TIME_IMMEDIATE );
		//paket se strči do mailboxu a v callback sendu se uloži do bufferu..
		//buffer2.pushBack(packet);
	}

	ApplicationLayer::processTimeoutErrorCb(slaveAddress);
}

bool LinkLayer::FreePacket(packet_t * pack)
{
	if (pack)
	{
		chPoolFree(&packet_pool, pack);
		return true;
	}
	return false;
}

/*
 * alokuje místo pro paket, nahrne ho tam a vrátí pointer na
 * zkopirovanej paket v nové paměti
 */
packet_t * LinkLayer::AllocPacket(packet_t * source)
{
	packet_t * ret = (packet_t *) chPoolAlloc(&packet_pool);
	if (ret != NULL)
		*ret = *source;

	return ret;
}

} /* namespace rfm */
