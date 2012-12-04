/**
 * @file RecThread.h
 * @author kubanec
 * @date 12.9.2012
 *
 */

#ifndef RECTHREAD_H_
#define RECTHREAD_H_

namespace rfm
{

namespace threads
{
class RecThread: public chibios_rt::EnhancedThread<256>
{
private:
	msg_t Main(void);

	static uint32_t offset;
	//bitově určuje ktery slavy má poslouchat
	static uint16_t listen;
	static uint8_t synchronized;
	static void Wait(uint8_t slave_address);
	static bool ReadPacket(packet_t & packet);
	static void Read();
	static void Synchro();
	static void Mate();
	static uint8_t listeningAddress;

public:

	static chibios_rt::Mutex * mutex;
	RecThread();
	friend class SendThread;
	inline static uint32_t GetOffset()
	{
		return offset;
	}
	inline static uint8_t IsSynchronized()
	{
		return synchronized;
	}
	inline static void SetSlaves(uint16_t slaves)
	{
		listen = slaves;
	}
	inline static uint16_t GetSlaves()
	{
		return listen;
	}
};

}
} /* namespace rfm */
#endif /* RECTHREAD_H_ */
