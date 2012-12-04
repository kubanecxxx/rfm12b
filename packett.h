/*
 * packett.h
 *
 *  Created on: 4.12.2012
 *      Author: kubanec
 */

#ifndef PACKETT_H_
#define PACKETT_H_

namespace rfm
{

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

	/*
	 * vrátí false pokud není synchronizace a nepodařilo se poslat paket
	 * vrátí true pokud všecko v pořádku
	 *
	 * @todo vymyslet kopirování, buď tady nebo když se dává do mailboxu
	 */
	bool Send();
	bool SendI();

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
	inline bool IsIdleOk()
	{
		return (data.b.load.b.Command == IDLEOK);
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
		data.b.DestAddr = MASTER;
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
		return (command_t) data.b.load.b.Command;
	}
};

} /* namespace rfm */
#endif /* PACKETT_H_ */
