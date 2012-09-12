/**
 * @file SendThread.cpp
 * @author kubanec
 * @date 12.9.2012
 *
 */

#include "rfmIncludeCpp.h"

namespace rfm
{

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
	rf_send(packet.DestAddr);
	for (unsigned i = 0; i < LOAD_LENGTH; i++)
		rf_send(packet.load[i]);
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
	systime_t temp = time % TIME;
	time += TIME - temp;

	if (LinkLayer::IsMaster())
	{
		/*
		 * master vysilá v prvni sadě timeslotů podle adresy příjemců (slavů)
		 */
		time += destination * TIMESLOT;
	}
	else
	{
		/*
		 * slave vysilá v druhé sadě timeslotů podle svoji lokálni adresy
		 * offset už musi byt naladěné
		 */
		time += RecThread::GetOffset() + (TIME / 2)
				+ LinkLayer::GetAddress() * TIMESLOT;
	}

	chibios_rt::BaseThread::SleepUntil(time);
}

msg_t SendThread::Main(void)
{

	packet_t * packet = new packet_t; //tohle se naplni pomoci msg

	packet->DestAddr = 2;

	systime_t time2;
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

		/*
		 * gain mutex
		 */

		/*
		 * wait for right time
		 */
		Wait(packet->DestAddr);

		/*
		 * send packet
		 */
		systime_t time1 = chibios_rt::System::GetTime();
		Send(*packet);
		time2 = chibios_rt::System::GetTime() - time1;

		/*
		 * release mutex
		 */
	}
	return 0;
}

}
} /* namespace rfm */
