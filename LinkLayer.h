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

class LinkBuffer: public simpleQueue<PACKET_BUFFER_LENGTH, packet_t *>
{
public:
	/**
	 * @brief vrátí paket se stejnou adresou destinace
	 * a vyhodi ho z fronty, ostatni tam nechá ve stejnym pořadi
	 *
	 * @return pokud tam paket je tak ho vrátí jinak vrátí NULL
	 */
	packet_t * process(uint8_t address)
	{
		packet_t * temp;
		packet_t * ret = NULL;
		uint8_t cant = count();

		for (int i = 0; i < cant; i++)
		{

			temp = popFront();

			if (temp->data.b.DestAddr == address)
			{
				ret = temp;
				if (i == 0)
					break;
			}
			else
			{
				pushBack(temp);
			}
		}
		return ret;
	}
};

class LinkLayer
{
public:
	static void Init(uint8_t address);
	static bool SendPacket(packet_t * packet);
	static bool SendPacketI(packet_t * packet);
	static void Init(uint8_t address, uint16_t listen);
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
	static void Callback(packet_t packet, bool checksumOk, uint8_t ListeAddr);
	static void CallbackSend(packet_t * packet, bool ok);
	static void CallbackNok(uint8_t slaveAddress);
	friend class threads::RecThread;
	friend class threads::SendThread;
	static LinkBuffer buffer2;
	static packet_t * AllocPacket(packet_t * source);
	static bool FreePacket(packet_t * packet);
};

} /* namespace rfm */
#endif /* LINKLAYER_H_ */
