/*
 * LinkLayer.h
 *
 *  Created on: 11.9.2012
 *      Author: kubanec
 */

#ifndef LINKLAYER_H_
#define LINKLAYER_H_

namespace rfm
{

class packet_t
{
public:
	uint8_t GetChecksum(void);
	bool ChecksumOK (uint8_t checksum);
	packet_t();
	static void * operator new(size_t size)
	{
		return chCoreAlloc(size);
	}

	uint8_t DestAddr;
	uint8_t load[LOAD_LENGTH];
private:
	uint8_t checksum;
};

class LinkLayer
{
public:
	static void Init(uint8_t address);
	static void SendPacket(packet_t & packet);
	static void GetPacket(void);
	static uint8_t GetAddress();
	static uint8_t IsMaster();

private:
	static int8_t SourceAddress;

};

} /* namespace rfm */
#endif /* LINKLAYER_H_ */
