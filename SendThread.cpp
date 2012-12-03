/**
 * @file SendThread.cpp
 * @author kubanec
 * @date 12.9.2012
 *
 */

#include "rfmIncludeCpp.h"
#include "ch.hpp"

namespace rfm
{

static msg_t mbuffer[10];
MAILBOX_DECL(mbox, mbuffer, 10);

namespace threads
{

SendThread::SendThread() :
		EnhancedThread("rfm send", NORMALPRIO)
{
}

void SendThread::Send(packet_t & packet)
{
	rf_transmitter();
	rf_prepare();
	rf_writecmd(0);
	rf_send(LinkLayer::GetAddress());
	for (unsigned i = 0; i < PACKET_LENGTH; i++)
		rf_send(packet.data.rawData[i]);
	rf_send(packet.GetChecksum());
	rf_send(0xAA);
	rf_send(0xAA);
	rf_send(0xAA);
	rf_sleep();
}

void SendThread::Wait(uint8_t destination)
{
	/*
	 * podle adresy a offsetu vypočitat čas do kdy má čekat
	 * 20ms trvá jeden paket -> rezerva 25ms
	 */

	systime_t time = chibios_rt::System::GetTime();
	systime_t off = RecThread::GetOffset();

	if (LinkLayer::IsMaster())
	{
		systime_t temp = time % TIME;
		time += TIME - temp;

		// master vysilá v prvni sadě timeslotů podle adresy příjemců (slavů)
		time += destination * TIMESLOT;
	}
	else
	{
		/*
		 * slave vysilá v druhé sadě timeslotů podle svoji lokálni adresy
		 * offset už musi byt naladěné
		 * v offsetu je čas začátku uspěšnyho přijmu pro správnou adresu slave
		 * takže stači čekat jenom pulku TIME
		 */
		time = off + (TIME / 2) - 3; //5ms rezerva pro mastera
		while (time < chibios_rt::System::GetTime())
		{
			time += TIME;
		}
	}

	SleepUntil(time);
}

msg_t SendThread::Main(void)
{

	while (TRUE)
	{
		/*
		 * posilat podle adresy v nějakym time slotu kterej se naladi v jinym vlákně
		 * na příjem
		 * master (adresa 1) bude posilat podle svyho timecounteru
		 *
		 * počkat až bude něco k odeslání
		 * odeslat ve správnym timeslotu (gain+release mutex)
		 *		(zjistit jak dloho trva odeslání jednoho paketu, podle toho nastavit kon
		 *		stanty)
		 *
		 * time slot bude hlidat druhy vlákno na přijem, ktery uloži
		 * offset od globalniho timecounteru - doladění na synchro
		 *
		 * Master bude posilat v prvni sadě timeslotů podle
		 * adres příjemců a ti budou podle svojich adres zrovna v těhle timeslotech poslouchat
		 *
		 * slavy budou vysilat v druhe sadě timeslotů podle svoji adresy a master
		 * bude poslouchat podle adres slavů ktery má někde napsany, že na nich má
		 * poslouchat
		 */

		/*
		 * wait for message packet
		 */
		msg_t message;
		msg_t resp = chMBFetch(&mbox, &message, TIME_INFINITE );

		packet_t * packet = (packet_t *) message;
		bool neco = false;

		if (resp == RDY_OK)
		{
			/*
			 * wait for right time
			 */
			Wait(packet->data.b.DestAddr);

			/*
			 * send packet
			 */
			if (RecThread::mutex->TryLock() && RecThread::IsSynchronized())
			{
				Send(*packet);
				palTogglePad(GPIOD,15);
				RecThread::mutex->Unlock();
				neco = true;
			}
		}
		LinkLayer::CallbackSend(packet, neco);
	}

	return 0;
}

}
} /* namespace rfm */
