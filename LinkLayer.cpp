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

int8_t LinkLayer::SourceAddress = -1;
threads::SendThread * LinkLayer::thd_send;
threads::RecThread * LinkLayer::thd_rec;
simpleStack<PACKET_BUFFER_LENGTH, packet_t> LinkLayer::buffer2;

void LinkLayer::Init(uint8_t address)
{
	if (SourceAddress != -1)
	{
		return;
	}

	if (address < 8)
	{
		SourceAddress = address;
		//rozběhnout vlákna
		thd_send = new threads::SendThread;
		thd_rec = new threads::RecThread();
	}
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
 * @todo vymyslet kopirování paketu někam do bufferu + alokace
 */
bool LinkLayer::SendPacket(packet_t * packet)
{
	if (IsSynchronized())
	{
		if (chMBPost(&mbox, (msg_t) packet, TIME_IMMEDIATE ) == RDY_OK)
			return true;
	}
	return false;
}

bool LinkLayer::SendPacketI(packet_t * packet)
{
	if (IsSynchronized())
	{
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

	if (IsMaster())
	{
		if (checksumOk)
		{
			/*
			 * rozeznat paket, spárovat ho s bufferem
			 * protože přinde odpověď ok/nok get/set
			 * uvolnit buffer
			 * zpracovat paket
			 * nastavit modbus pokud to bylo get
			 * hodit callback
			 * ok/nok dat do callbacku
			 * uvolnit uplně paket
			 */
		}
		else
		{
			/*
			 * spárovat pakety packet + ten co je v buffer2
			 * a vytáhnout ho z buffer2 a hodit znova do fronty
			 * podle adresy z které to mělo přijit
			 *
			 */
		}
	}
	else
	{
		if (checksumOk)
		{
			/*
			 * rozeznat paket
			 * nastavit modbus
			 * hodit callback
			 * hodit odpověď pro mastera
			 */
		}
		/*
		 * pokud je checksum špatně tak paket zahodit a master se zeptá znova
		 */
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
			buffer2.push(packet_);
		}
		else
		{
			chMBPostAhead(&mbox, (msg_t) packet_, TIME_IMMEDIATE );
		}
	}
	else
	{
		/*
		 * slave - rovnou uvolnit paket a kašlat na to
		 */
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
	/*
	 * tady muže poznat podle toho co má ve frontě jesli to byl jenom idle
	 * paket nebo nějaké vic crucial a muže ho nechat zopakovat
	 *
	 * taky se dá využit buffer2
	 */

}

} /* namespace rfm */
