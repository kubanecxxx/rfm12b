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

	for (unsigned i = 0; i < LOAD_LENGTH; i++)
		load[i] = 0;
}

uint8_t packet_t::GetChecksum()
{
	uint8_t temp = 0;

	if (checksum != 0)
		return checksum;

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

void LinkLayer::Init(uint8_t address)
{
	if (SourceAddress != -1)
	{
		return;
	}

	if (address < 8)
	{
		SourceAddress = address;
		new threads::RecThread;
		//rozběhnout vlákna
	}

}

void LinkLayer::SendPacket(packet_t & packet)
{
	//poslat zprávu do posilaciho vlákna, release mutex
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

} /* namespace rfm */
