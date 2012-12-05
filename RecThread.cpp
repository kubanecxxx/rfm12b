/**
 * @file RecThread.cpp
 * @author kubanec
 * @date 12.9.2012
 *
 */

#include "rfmIncludeCpp.h"
#include "ch.h"
#include "hal.h"

namespace rfm
{

namespace threads
{
uint32_t RecThread::offset = 0;
uint16_t RecThread::listen = 0;
uint8_t RecThread::synchronized = 0;
chibios_rt::Mutex * RecThread::mutex;
uint8_t RecThread::listeningAddress = 0;

RecThread::RecThread() :
		EnhancedThread("rfm receive", NORMALPRIO)
{
	rf_ffitThreadInit(thread_ref);
	mutex = new chibios_rt::Mutex;
}

bool RecThread::ReadPacket(packet_t & packet)
{
	uint8_t dest;
	uint8_t checksum;
	const uint8_t timeout = 10;

	//adresy scvhálně obráceně kvuli checksumu
	//funkce rf_read() zablokuje vlákno dokud nic nepřinde nebo dojde timeout

	packet.data.b.DestAddr = rf_read(timeout); //ve skutečnosti adresa odesílatele
	dest = rf_read(timeout); //ve skutečnosti adresa příjemce

	if (dest != LinkLayer::GetAddress())
	{
		rf_fifo_reset();
		rf_writecmd(0);
		return false;
	}

	for (int i = 0; i < LOAD_LENGTH; i++)
	{
		packet.data.b.load.load[i] = rf_read(timeout);
	}
	checksum = rf_read(timeout);

	rf_fifo_reset();
	rf_writecmd(0);

	bool what = packet.ChecksumOK(checksum);
	return what;
}

msg_t RecThread::Main(void)
{
	while (TRUE)
	{
		if (!LinkLayer::IsMaster())
		{
			//blokující funkce, vypadne až bude synchronizace správně
			palClearPad(GPIOD, 13);
			Synchro();
			palSetPad(GPIOD, 13);
		}

		synchronized = 1;

		while (synchronized)
		{
			//umrtvi vlákno na správné čas kdy se má přijimat
			Mate();

			if (mutex->TryLock())
			{
				Read();
				mutex->Unlock();
				listeningAddress++;
			}
		}
	}
	return 0;
}

void RecThread::Mate()
{
	/*
	 * projet všechny na kteréch má poslochat pokud je to master,
	 * pokud je to slave tak poslocha jenom na svym timeslotu
	 */

	if (LinkLayer::IsMaster())
	{
		/*
		 * tady musi projet všechny adresy na kteréch má poslochat
		 * adresy bude mit zatim zadany
		 */
		while (!((listen >> listeningAddress) & 1))
		{
			listeningAddress++;
			if (listeningAddress == MAX_UNITS)
				listeningAddress = 0;
		}
		Wait(listeningAddress);
	}
	else
	{
		//počkat na správné synchro čas
		Wait(LinkLayer::GetAddress());
	}
}

#ifdef DEBUG_RFM
uint16_t synchrocount = 0;
#endif
void RecThread::Read()
{
#ifdef DEBUG_RFM
	static uint32_t pole[1000];
	static uint16_t index = 0;
	static uint16_t timeout = 0;
	static uint16_t dobry = 0, spatny = 0, idx = 0;
#endif
	packet_t packet;
	static uint8_t error_counter = 0;

	rf_receiver();
	rf_writecmd(0);
	rf_fifo_reset();

#ifdef DEBUG_RFM
	if (index > 700)
	{
		asm ("nop");
		index = 0;
	}

	if (idx++ == 100)
	{
		asm ("nop");
		idx = 0;
	}
#endif

	/*
	 * wait for ffit
	 * pokud něco přinde tak to přijme
	 * pokud nic tak to zkusi přiště znova
	 */
#ifdef DEBUG_RFM
	pole[index++] = 5;
	pole[index++] = chibios_rt::System::GetTime();
#endif
	if (low_level_wait_ffit_high(10))
	{
		chEvtAddFlags(FFIT_EVENT_FLAG);
		//read packet
		systime_t time2 = chibios_rt::System::GetTime();
#ifdef DEBUG_RFM
		pole[index++] = 6;
		pole[index++] = chibios_rt::System::GetTime();
#endif
		bool checksum = ReadPacket(packet);
		bool be;
		if (LinkLayer::IsMaster())
			be = (listeningAddress == packet.data.b.DestAddr);
		else
			be = (MASTER == packet.data.b.DestAddr);

		checksum = be & checksum;
#ifdef DEBUG_RFM
		pole[index++] = 7;
		pole[index++] = chibios_rt::System::GetTime();
#endif
		/*
		 * kontrola ješli je paket v pořádku
		 * a jestli je pro správnyho
		 */
		if (!checksum)
		{
			error_counter++;
#ifdef DEBUG_RFM
			spatny++;
#endif
		}
		else
		{
#ifdef DEBUG_RFM
			dobry++;
#endif
			/*
			 * znova se naladi pokud
			 * přijal správně
			 */
			error_counter = 0;
			offset = time2;
			offset -= 7;
			offset %= TIME;
		}

		/*
		 * řešení zpracování idle paketu
		 * pokud master, tak očekává idleok a už neodešle
		 * nic zpátky
		 *
		 * pokud slave tak idle paket rovnou zpracuje a hned odešle
		 * odpověď na idle paket, aby o něm master věděl a počital
		 * s nim
		 *
		 * vymyšleno o pár řádků dole - při timeoutu hodi callback
		 * vymyslet mechanismus callbacku pokud nic neodpoví
		 * ten by mohl fungovat tak že dokud slave neodpovi tak neuvolni
		 * paket z linklayeru a nechá ho furt ve frontě a pak se pošle znovu dotaz
		 * což už je arq pro get/set ale zatim nevim jak na idle paket
		 * ten se bude muset řešit někde přimo tady
		 * jenom hodit nahoru callback že slave neodpověděl na idle ať user vi že
		 * slave neodpovidá
		 * spiš se to musi řešit ještě předtimhle protože určitě dojde k timeoutu
		 */
		if (LinkLayer::IsMaster())
		{
			/*
			 * master
			 *
			 * pokud přinde odpověď na idle paket idleok tak neudělá nic
			 * pokud přinde cokoli jinyho tak hodi callback zpátky userovi do linklayer
			 * v callbacku se nastavi zase modbus, připadně se hodi dalši callback userovi
			 */
			if (packet.IsIdleOk())
			{

			}
			else
			{
				LinkLayer::Callback(packet, checksum, listeningAddress);
			}
		}
		else
		{
			/*
			 * případ slava
			 * pokud přinde v pořádku idle paket tak na něho rovnou odpovi
			 * přimo tady ve vlákně ani nejde callback nahoru
			 *
			 * pokud je to normální paket tan hodi callback zpátky
			 * do linklayer
			 */
			if (packet.IsIdle() && checksum)
			{
				packet.AnswerIdle();
				packet.Send();
			}
			else
			{
				/*
				 * v callbacku už si uživatel vymysli jak paket zpracuje
				 * nastavi věci v modbusu, připadně pošle dalši callback
				 * kterej už bude v polu...
				 */
				LinkLayer::Callback(packet, checksum, listeningAddress);
			}
		}
	}
	else
	{
		/*
		 * timeout
		 * slave použivá error_counter
		 * pokud je jich moc tak se  znova synchronizuje
		 * timeout má větši váhu než jenom chybné paket
		 */
		error_counter += 3;

		/*
		 * pokud máster a bude timeout tak určitě chybí odpověď minimálně na idle
		 * paket takže se hodi userovi callback že odpověď je v prdeli a ne tady
		 */
		if (LinkLayer::IsMaster())
		{
			LinkLayer::CallbackNok(listeningAddress);
		}
#ifdef DEBUG_RFM
		pole[index++] = 50;
		pole[index++] = chibios_rt::System::GetTime();
		timeout++;
#endif
	}

	if (error_counter > 5 && !LinkLayer::IsMaster())
		synchronized = 0;

	rf_sleep();
	rf_writecmd(0);
	rf_fifo_reset();
	rf_writecmd(0);
}

/**
 * @brief volá se jenom v připadě slave
 * a zasynchronizuje se na správnej čas podle svoji adresy
 * a přijatéch paketu ktery sou jenom pro něho
 */
void RecThread::Synchro()
{
	packet_t packet;
#ifdef DEBUG_RFM
	synchrocount++;
#endif
	mutex->Lock();
	while (TRUE)
	{
		rf_receiver();
		rf_writecmd(0);
		rf_fifo_reset();
		rf_writecmd(0);

		low_level_wait_ffit_high(TIME_INFINITE );
		chEvtAddFlags(FFIT_EVENT_FLAG);
		/*
		 * čeká se až přinde paket pro mě a od mastera a navic bude v pořádku
		 * až se to uloží se čas v kterym přišla zpráva
		 * pak bude poslouchat dycky v tomhle čase za 400ms
		 */
		offset = chibios_rt::System::GetTime();
		if (ReadPacket(packet) && packet.data.b.DestAddr == MASTER)
		{

			rf_sleep();
			rf_fifo_reset();
			rf_writecmd(0);
			//tady je bod kdy už sme synchronizovani
			//v offsetu je čas začátku prvniho přijmu, ktere byl uspesne
			offset -= 8;
			offset %= TIME;

			mutex->Unlock();

			/*
			 * rovnou zpracuje paket
			 * pokud idle tak na něho odpoví
			 * pokud nějaké jiné tak ho rovnou zpracuje
			 * vyšší vrstva
			 */
			if (packet.IsIdle())
			{
				packet.AnswerIdle();
				packet.Send();
			}
			else
			{
				//na adrese nezáleži protože ta má vyznam jenom pro mástra
				LinkLayer::Callback(packet, true, 0);
			}

			break;
		}
	}
}

void RecThread::Wait(uint8_t slave_address)
{
	/*
	 * podle adresy a offsetu vypočitat čas do kdy má čekat
	 * 20ms trvá jeden paket -> rezerva 25ms
	 */
	systime_t time;

	if (LinkLayer::IsMaster())
	{
		/*
		 * master přijimá v druhé sadě slotů podle adresy vysílače (slave)
		 */

		time = chibios_rt::System::GetTime();
		systime_t temp = time % TIME;
		time -= temp;
		time += slave_address * TIMESLOT + TIME / 2;

		if (time < chibios_rt::System::GetTime())
			time += TIME;
	}
	else
	{
		/*
		 * slave přijimá v prvni sadě timeslotů
		 * v offsetu je čas prvniho přijmu pro tuhle adresu
		 * takže stači dycky počkat dalšich 400ms
		 */
		time = (chibios_rt::System::GetTime() / TIME + 1) * TIME + offset;
	}

	chibios_rt::BaseThread::SleepUntil(time);
}

} //namespáce threads
} /* namespace rfm */
