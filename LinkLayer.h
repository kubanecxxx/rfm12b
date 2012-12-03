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
	static bool SendPacketI(packet_t * packet);
	static void Init(uint8_t address, uint16_t listen);
	static void GetPacket(void);
	inline static uint8_t GetAddress()
	{
		return (uint8_t) SourceAddress;
	}
	inline static uint8_t IsMaster()
	{
		return (SourceAddress == MASTER);
	}
	static uint8_t IsSynchronized();

private:
	static int8_t SourceAddress;
	static threads::SendThread * thd_send;
	static threads::RecThread * thd_rec;
	static void Callback(packet_t packet, bool checksumOk);
	static void CallbackSend(packet_t * packet, bool ok);
	friend class threads::RecThread;
	friend class threads::SendThread;
};

class packet_t
{
public:
	uint8_t GetChecksum(void);
	inline bool ChecksumOK(uint8_t checksum)
	{
		return (GetChecksum() == checksum);
	}
	packet_t();

	union
	{
		struct
		{
			uint8_t DestAddr;
			union
			{

				struct
				{
					uint8_t Command;
					uint8_t reserved;
					uint8_t address[2];
					uint8_t data[4];
				} b;

				uint8_t load[LOAD_LENGTH];
			} load;
		} b;
		uint8_t rawData[PACKET_LENGTH];
	} data;

	/*
	 * vrátí false pokud není synchronizace a nepodařilo se poslat paket
	 * vrátí true pokud všecko v pořádku
	 */
	inline bool Send()
	{
		return LinkLayer::SendPacket(this);
	}
	inline bool SendI()
	{
		return LinkLayer::SendPacketI(this);
	}
	inline uint32_t GetData()
	{
		uint32_t temp = 0;
		for (int i = 0; i < 4; i++)
			temp |= data.b.load.b.data[i] << (i * 8);

		return temp;
	}
	inline uint16_t GetAddress()
	{
		uint16_t temp = 0;
		for (int i = 0; i < 2; i++)
			temp |= data.b.load.b.address[i] << (i * 8);

		return temp;
	}

private:
	uint8_t checksum;
};

} /* namespace rfm */
#endif /* LINKLAYER_H_ */
