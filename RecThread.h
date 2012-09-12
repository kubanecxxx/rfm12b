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
class RecThread: public chibios_rt::EnhancedThread<128>, public rfmNew
{
private:
	msg_t Main(void);

	static uint32_t offset;
	static uint16_t listen;
	static void Wait(uint8_t slave_address = 0);
	static bool ReadPacket(packet_t & packet);

public:
	RecThread();
	static uint32_t GetOffset()
	{
		return offset;
	}

};

}
} /* namespace rfm */
#endif /* RECTHREAD_H_ */
