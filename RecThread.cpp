/**
 * @file RecThread.cpp
 * @author kubanec
 * @date 12.9.2012
 *
 */

#include "rfmIncludeCpp.h"

namespace rfm
{

namespace threads
{
uint32_t RecThread::offset = 0;
uint16_t RecThread::listen = 0;

RecThread::RecThread() :
		EnhancedThread("rfm receive", HIGHPRIO)
{
}

bool RecThread::ReadPacket(packet_t & packet)
{
	uint8_t dest;
	uint8_t checksum;

	//adresy scvhálně obráceně kvuli checksumu
	packet.DestAddr = rf_read(); //ve skutečnosti adresa odesílatele
	dest = rf_read(); //ve skutečnosti adresa příjemce
	if (dest != LinkLayer::GetAddress())
		return false;

	for (unsigned i = 0; i < LOAD_LENGTH; i++)
	{
		packet.load[i] = rf_read();
	}
	checksum = rf_read();

	return packet.ChecksumOK(checksum);
}

msg_t RecThread::Main(void)
{
	packet_t packet;
	/*
	 * naladit offset
	 */
	while (TRUE)
	{
		rf_receiver();
		rf_writecmd(0);
		rf_fifo_reset();
		rf_writecmd(0);

		//wait ffit

		/*
		 * receive packet
		 */
		if (ReadPacket(packet))
		{
			offset = TIME - (chibios_rt::System::GetTime() % TIME);
			break;
		}
	}

	new SendThread;

	while (TRUE)
	{
		/*
		 * pokud adresa neni mástr tak se doladit na mástra
		 * až bude syncrhoni tak si uložit offset
		 *
		 * master musi vědět od kterejch bude přijimat
		 */

		/*
		 * projet všechny na kteréch má polochat pokud je to master,
		 * pokud je to slave tak poslocha jenom na svym timeslotu
		 */
		if (LinkLayer::IsMaster())
		{
			/*
			 * tady musi projet všechny adresy na kteréch má poslochat
			 * adresy bude mit zatim zadany
			 * v budocnu by se udělal nějaké autodiscover
			 */
		}
		else
		{
			/*
			 * wait for right time
			 */
			Wait();
		}

		rf_receiver();
		/*
		 * wait for ffit (event + timeout)
		 */

		systime_t time = chibios_rt::System::GetTime();

		/*
		 * read packet
		 */

		rf_sleep();
		/*
		 * tune offset
		 */
		offset = TIME - (time % TIME);

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
		 * slave přijimá v prvni sadě slotů 2ms rezerva
		 * offset už musi byt naladěné
		 */
		time += GetOffset() - 2 + LinkLayer::GetAddress() * TIMESLOT;
	}

	chibios_rt::BaseThread::SleepUntil(time);
}

} //namespáce threads
} /* namespace rfm */
