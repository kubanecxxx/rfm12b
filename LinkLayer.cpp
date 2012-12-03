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

packet_t::packet_t()
{
	checksum = 0;
	//vyplnit packet nulama
	for (unsigned i = 0; i < PACKET_LENGTH; i++)
		data.rawData[i] = 0;
}

uint8_t packet_t::GetChecksum()
{
	uint8_t temp = 0;

	temp = LinkLayer::GetAddress();

	for (unsigned i = 0; i < PACKET_LENGTH; i++)
	{
		temp += data.rawData[i];
	}

	checksum = temp;
	checksum ^= 0b01010101;

	return checksum;
}

int8_t LinkLayer::SourceAddress = -1;
threads::SendThread * LinkLayer::thd_send;
threads::RecThread * LinkLayer::thd_rec;

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

void LinkLayer::GetPacket()
{
	//dat paket když bude něco přijatyho
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
 */
void LinkLayer::Callback(packet_t packet, bool checksumOk)
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

}

void LinkLayer::CallbackSend(packet_t * packet_, bool ok)
{
	static int be = 0 ,dobry = 0 ;

	if (!ok)
		be++;
	else
		dobry++;

	static int j = 0;
	for (int i = 0; i < rfm::LOAD_LENGTH; i++)
		packet.data.b.load.load[i] = j++;

#if RFM_MASTER
	packet.Send();
#else
	packet.Send();
#endif
}

} /* namespace rfm */
