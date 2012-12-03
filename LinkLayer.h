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

	typedef enum
	{
		IDLE, GET, SET, OKSET, OKGET, NOKSET, NOKGET, IDLEOK
	} command_t;

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
	inline void MakeIdle()
	{
		data.b.load.b.Command = IDLE;
	}
	inline bool IsIdle()
	{
		return (data.b.load.b.Command == IDLE);
	}
	/**
	 * @brief nastaví paket na příkaz SET (v masteru)
	 */
	inline void Set(uint8_t destination, uint16_t mod_address,
			uint32_t mod_data)
	{
		data.b.DestAddr = destination;
		data.b.load.b.Command = SET;
		data.b.load.b.address[0] = mod_address;
		data.b.load.b.address[1] = mod_address >> 8;
		data.b.load.b.data[0] = mod_data;
		data.b.load.b.data[1] = mod_data >> 8;
		data.b.load.b.data[2] = mod_data >> 16;
		data.b.load.b.data[3] = mod_data >> 24;
	}
	/**
	 * @brief nastaví paket na příkaz GET (v masteru)
	 */
	inline void Get(uint8_t destination, uint16_t mod_address)
	{
		data.b.DestAddr = destination;
		data.b.load.b.Command = GET;
		data.b.load.b.address[0] = mod_address;
		data.b.load.b.address[1] = mod_address >> 8;
	}

	/**
	 * @brief nastaví packet na odpověď idle (ve slavu)
	 */
	inline void AnswerIdle()
	{
		data.b.load.b.Command = IDLEOK;
	}

	/**
	 * @brief nastaví paket na odpověd getu (ve slavu)
	 */
	inline void AnswerGet(uint32_t mod_data, bool ok)
	{
		data.b.DestAddr = MASTER;
		if (ok)
			data.b.load.b.Command = OKGET;
		else
			data.b.load.b.Command = NOKGET;
		data.b.load.b.data[0] = mod_data;
		data.b.load.b.data[1] = mod_data >> 8;
		data.b.load.b.data[2] = mod_data >> 16;
		data.b.load.b.data[3] = mod_data >> 24;
	}
	/**
	 * @brief nastaví paket na odpověď setu (ve slavu)
	 */
	inline void AnswerSet(bool ok)
	{
		data.b.DestAddr = MASTER;
		if (ok)
			data.b.load.b.Command = OKSET;
		else
			data.b.load.b.Command = NOKSET;
	}

	/**
	 * @brief zjistí jakén je to command
	 */
	inline command_t Command()
	{
		return (command_t)data.b.load.b.Command;
	}


private:
	uint8_t checksum;
};

} /* namespace rfm */
#endif /* LINKLAYER_H_ */
