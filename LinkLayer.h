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
	static simpleStack<PACKET_BUFFER_LENGTH, packet_t> buffer2;
};

} /* namespace rfm */
#endif /* LINKLAYER_H_ */
