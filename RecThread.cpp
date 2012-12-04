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
			}
		}
	}
	return 0;
}

void RecThread::Mate()
{
	static uint8_t i = 0;
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
		while (!((listen >> i) & 1))
		{
			i++;
			if (i == MAX_UNITS)
				i = 0;
		}
		Wait(i);
		i++;
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
#ifdef DEBUG_RFM
		pole[index++] = 7;
		pole[index++] = chibios_rt::System::GetTime();
#endif
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
			error_counter = 0;
			offset = time2;
			offset -= 7;
			offset %= TIME;
		}

		LinkLayer::Callback(packet, checksum);
	}
	else
	{
		error_counter += 3;
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

			LinkLayer::Callback(packet, true);

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
