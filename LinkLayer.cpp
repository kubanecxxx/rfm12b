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

bool packet_t::ChecksumOK(uint8_t checksum)
{
	return (GetChecksum() == checksum);
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
	//poslat zprávu do posilaciho vlákna
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

uint8_t LinkLayer::GetAddress()
{
	return (uint8_t) SourceAddress;
}

uint8_t LinkLayer::IsMaster()
{
	return (SourceAddress == MASTER);
}

uint8_t LinkLayer::IsSynchronized()
{
	if (IsMaster()) //pak sem přidat něco z vlákna receive
		return 1;

	if (threads::RecThread::IsSynchronized())
		return 1;

	return 0;
}

} /* namespace rfm */
