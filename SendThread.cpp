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

static msg_t mbuffer[PACKET_BUFFER_LENGTH];
MAILBOX_DECL(mbox, mbuffer, PACKET_BUFFER_LENGTH);

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

	//vypočte nejbližši možné čas kdy bude moct zase vysilat
	if (LinkLayer::IsMaster())
	{
		systime_t temp = time % TIME;
		time -= temp;
		time += destination * TIMESLOT;

		if (time < chibios_rt::System::GetTime())
			time += TIME;
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
	packet_t idle_packet;
	idle_packet.MakeIdle();
	idle_packet.data.b.DestAddr = -1;

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
		systime_t time = TIME_INFINITE;
		packet_t * packet;
		/*
		 * pokud máster tak se nečeká až uživatel něco pošle
		 * a rovnou se posilá na vybrany adresy idle zpráva
		 * kvůli synchronizaci
		 *
		 */
		/*
		 * pokud nic nepřišlo pošle se idle packet
		 * v případě slave to nemůže nastat protože čeká do nekonečna
		 * dokud nějaké paket nepřinde do mailu
		 *
		 * pokud chceme poslat paket kterej by předběhl idle tak se znova
		 * hodi do fronty až na něho přinde řada
		 */
		if (LinkLayer::IsMaster())
		{
			packet = &idle_packet;
			packet->data.b.DestAddr++;

			/*
			 * postupně projíždí všechny adresy ktery má registrovany
			 * od nejnižšiho aby se vysilalo hezky postupně
			 */
			while (!((RecThread::listen >> packet->data.b.DestAddr) & 1))
			{
				packet->data.b.DestAddr++;
				if (packet->data.b.DestAddr == MAX_UNITS)
					packet->data.b.DestAddr = 0;
			}
			time = TIME_IMMEDIATE;
		}

		msg_t resp = chMBFetch(&mbox, &message, time);
		/*
		 * pokud přišla zpráva tak ju načte do paketu
		 */
		if (resp == RDY_OK)
			packet = (packet_t *) message;
		bool neco = false;

		/*
		 * tady to zase postne zpátky do fronty pokud se mu to nehodi
		 * do správnyho času a musel by zahodit idle pakety ktery jsou před
		 * uživatelskym
		 *
		 * v připadě slave tahle podminka nebude nikdy splněna protože
		 * defaultně je idle adresa -1, která se nikdy nezmění
		 */
		if (resp == RDY_OK
				&& packet->data.b.DestAddr != idle_packet.data.b.DestAddr
				&& LinkLayer::IsMaster())
		{
			resp = !RDY_OK;
			chMBPostAhead(&mbox, (msg_t) packet, TIME_IMMEDIATE );
			packet = &idle_packet;
		}

		/*
		 * wait for right time
		 */
		Wait(packet->data.b.DestAddr);

		/*
		 * send packet
		 */
		if (RecThread::mutex->TryLock() && RecThread::IsSynchronized())
		{
			if (!packet->IsIdle() && !packet->IsIdleOk())
				palTogglePad(GPIOD, 15);
			Send(*packet);
			RecThread::mutex->Unlock();
			neco = true;
		}

		/*
		 * pokud to byla uživatelská zpráva tak
		 * vyhodi callback že už to skončilo
		 */
		if (resp == RDY_OK)
		{
			LinkLayer::CallbackSend(packet, neco);
			//uvolnit bazén paketu
		}
	}

	return 0;
}

}
} /* namespace rfm */
