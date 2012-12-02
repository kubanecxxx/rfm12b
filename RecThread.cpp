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
RecThread::rfm_cb RecThread::CallBack = NULL;

RecThread::RecThread(rfm_cb cb) :
		EnhancedThread("rfm receive", NORMALPRIO)
{
	CallBack = cb;
	rf_ffitThreadInit(thread_ref);
}

uint8_t pico;
packet_t packet2;

bool RecThread::ReadPacket(packet_t & packet)
{
	uint8_t dest;
	uint8_t checksum;
	const uint8_t timeout = 10;

	//adresy scvhálně obráceně kvuli checksumu
	//funkce rf_read() zablokuje vlákno dokud nic nepřinde nebo dojde timeout

	packet.DestAddr = rf_read(timeout); //ve skutečnosti adresa odesílatele
	dest = rf_read(timeout); //ve skutečnosti adresa příjemce
	pico = dest;

	if (dest != LinkLayer::GetAddress())
	{
		rf_fifo_reset();
		rf_writecmd(0);
		return false;
	}

	for (int i = 0; i < LOAD_LENGTH; i++)
	{
		packet.load[i] = rf_read(timeout);
	}
	checksum = rf_read(timeout);

	rf_fifo_reset();
	rf_writecmd(0);

	bool what = packet.ChecksumOK(checksum);
	return what;
}

msg_t RecThread::Main(void)
{
	packet_t packet;

	int16_t error_counter = 1;
	uint32_t dobry = 0;
	uint32_t spatny = 0;
	uint32_t timeout = 0;
	uint32_t synchronizace = 0;
	static uint32_t pole[300];
	uint16_t index = 0;

	while (TRUE)
	{

		if (!LinkLayer::IsMaster())
		{
			/*
			 * naladit offset pokud to neni mástr
			 */
			while (error_counter)
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
				if (ReadPacket(packet2) && packet2.DestAddr == MASTER)
				{
					rf_sleep();
					rf_fifo_reset();
					rf_writecmd(0);
					error_counter = 0;
					//tady je bod kdy už sme synchronizovani
					//v offsetu je čas začátku prvniho přijmu, ktere byl uspesne
					offset -= 8;
					offset %= TIME;
					synchronizace++;
					pole[index++] = 30;
					pole[index++] = chibios_rt::System::GetTime();
					//palClearPad(GPIOD, 1 << 13);
				}
			}
		}
		uint16_t temp = 0;

		while (error_counter < 5)
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
				 * v budocnu by se udělal nějaké autodiscover
				 */
				uint8_t i;
				if (temp == 0)
				{
					temp = offset;
					i = 0;
				}

				while (!(temp & 1))
				{
					temp >>= 1;
					i++;
				}
				Wait(i);
			}
			else
			{
				//počkat na správné synchro čas
				Wait(LinkLayer::GetAddress());
			}

			systime_t time = chibios_rt::System::GetTime();
			rf_receiver();
			rf_writecmd(0);
			rf_fifo_reset();
			//	rf_writecmd(0);

			systime_t time15 = chibios_rt::System::GetTime();

			/*
			 * wait for ffit
			 * pokud něco přinde tak to přijme
			 * pokud nic tak to zkusi přiště znova
			 */

			if (low_level_wait_ffit_high(10))
			{
				//read packet
				systime_t time2 = chibios_rt::System::GetTime();
				bool checksum = ReadPacket(packet);

				systime_t time3 = chibios_rt::System::GetTime();

				if (index > 200)
				{
					asm ("nop");
				}

				pole[index++] = 50;
				pole[index++] = time15;
				pole[index++] = time2;

				if (!checksum)
				{

					error_counter++;
					spatny++;
					pole[index++] = 10;
					pole[index++] = time3;
					pole[index++] = pico;
					pole[index++] = packet.DestAddr;
					if (pico == 5 && packet.DestAddr == 1)
					{
						for (int j = 0; j < LOAD_LENGTH; j++)
							pole[index++] = packet.load[j];
						pole[index++] = packet.GetChecksum();
					}
				}
				else
				{
					dobry++;
					offset = time2;
					offset -= 5;
					offset %= TIME;
					pole[index++] = 20;
					pole[index++] = chibios_rt::System::GetTime();

				}

				//poslat chcal back pokud je naštelované
				if (CallBack)
					CallBack(packet, checksum);
			}
			else
			{
				pole[index++] = 500;
				pole[index++] = time15;
				pole[index++] = chibios_rt::System::GetTime();
				error_counter++;
				timeout++;
			}
			rf_sleep();
			rf_writecmd(0);
			rf_fifo_reset();
			rf_writecmd(0);
		}
	}
	return 0;
}

void RecThread::Wait(uint8_t slave_address)
{
	/*
	 * podle adresy a offsetu vypočitat čas do kdy má čekat
	 * 20ms trvá jeden paket -> rezerva 25ms
	 */
	systime_t time = chibios_rt::System::GetTime();
	systime_t temp = time % TIME;
	time += TIME - temp;

	if (LinkLayer::IsMaster())
	{
		/*
		 * master přijimá v druhé sadě slotů podle adresy vysílače (slave)
		 */
		time += (TIME / 2) - 2 + slave_address * TIMESLOT; //2ms rezerva na začátku
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
