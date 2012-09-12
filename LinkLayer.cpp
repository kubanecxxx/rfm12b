/*
 * LinkLayer.cpp
 *
 *  Created on: 11.9.2012
 *      Author: kubanec
 */

#include "rfmIncludeCpp.h"

namespace rtfm
{

int8_t LinkLayer::SourceAddress = -1;

void LinkLayer::Init(uint8_t address)
{
	if (SourceAddress != -1)
	{
		return;
	}

	if (address < 8)
		SourceAddress = address;
}

void LinkLayer::SendPacket(packet_t & packet)
{

}

void LinkLayer::ReceivePacket()
{

}

} /* namespace rfm */
