/*
 * LinkLayer.cpp
 *
 *  Created on: 11.9.2012
 *      Author: kubanec
 */

#include "rfmIncludeCpp.h"

namespace rfm
{

packet_t::packet_t()
{
	checksum = 0;
	DestAddr = 0;
	//vyplnit packet nulama
	for (unsigned i = 0; i < LOAD_LENGTH; i++)
		load[i] = 0;
}

uint8_t packet_t::GetChecksum()
{
	uint8_t temp = 0;

	temp = LinkLayer::GetAddress();
	temp += DestAddr;

	for (unsigned i = 0; i < LOAD_LENGTH; i++)
	{
		temp += load[i];
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

/**
 * vrátí true pokud se všecko povede
 * false pokud je mimo synchro nebo nedostal mutex
 */
bool LinkLayer::SendPacket(packet_t * packet)
{
	if (IsSynchronized())
	{
		if (thd_send->SendMessage((msg_t) packet))
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

}

} /* namespace rfm */
