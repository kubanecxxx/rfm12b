/*
 * LinkLayer.h
 *
 *  Created on: 11.9.2012
 *      Author: kubanec
 */

#ifndef LINKLAYER_H_
#define LINKLAYER_H_

namespace rtfm
{

class packet_t
{
public:
	uint8_t GetChecksum(void);

	uint8_t DestAddr;
	uint8_t * load;
	uint8_t LoadLength;
private:
	uint8_t checksum;
};

class LinkLayer
{
public:
	static void Init(uint8_t address);
	static void SendPacket(packet_t & packet);
	static void ReceivePacket(void);

	static int8_t SourceAddress;
};

} /* namespace rfm */
#endif /* LINKLAYER_H_ */
