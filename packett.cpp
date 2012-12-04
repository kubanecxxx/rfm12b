/*
 * packett.cpp
 *
 *  Created on: 4.12.2012
 *      Author: kubanec
 */

#include "rfmIncludeCpp.h"

namespace rfm
{

packet_t::packet_t()
{
	//vyplnit packet nulama
	for (unsigned i = 0; i < PACKET_LENGTH; i++)
		data.rawData[i] = 0;
}

uint8_t packet_t::GetChecksum()
{
	uint8_t temp = 0;
	uint8_t checksum;

	temp = LinkLayer::GetAddress();

	for (int i = 0; i < PACKET_LENGTH; i++)
	{
		temp += data.rawData[i];
	}

	checksum = temp;
	checksum ^= 0b01010101;

	return checksum;
}

bool packet_t::Send()
{
	return LinkLayer::SendPacket(this);
}
bool packet_t::SendI()
{
	return LinkLayer::SendPacketI(this);
}

} /* namespace rfm */
