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
class RecThread: public chibios_rt::EnhancedThread<256>, public rfmNew
{
public:
	typedef void (*rfm_cb)(packet_t & packet, bool checksumOK);

private:
	msg_t Main(void);

	static uint32_t offset;
	static uint16_t listen;
	static void Wait(uint8_t slave_address);
	static bool ReadPacket(packet_t & packet);
	static rfm_cb CallBack;

public:

	RecThread(rfm_cb cb = NULL);
	static uint32_t GetOffset()
	{
		return offset;
	}

};

}
} /* namespace rfm */
#endif /* RECTHREAD_H_ */
