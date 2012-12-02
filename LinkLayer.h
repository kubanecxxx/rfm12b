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

namespace threads
{
class SendThread;
class RecThread;
}

class LinkLayer;
class packet_t;

class LinkLayer
{
public:
	static void Init(uint8_t address);
	static bool SendPacket(packet_t * packet);
	static void GetPacket(void);
	static uint8_t GetAddress();
	static uint8_t IsMaster();
	static uint8_t IsSynchronized();

private:
	static int8_t SourceAddress;
	static threads::SendThread * thd_send;
	static threads::RecThread * thd_rec;

};

class packet_t
{
public:
	uint8_t GetChecksum(void);
	bool ChecksumOK(uint8_t checksum);
	packet_t();

	uint8_t DestAddr;
	uint8_t load[LOAD_LENGTH];

	/*
	 * vrátí false pokud není synchronizace a nepodařilo se poslat paket
	 * vrátí true pokud všecko v pořádku
	 */
	inline bool Send()
	{
		return LinkLayer::SendPacket(this);
	}

private:
	uint8_t checksum;
};

} /* namespace rfm */
#endif /* LINKLAYER_H_ */
