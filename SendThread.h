/**
 * @file SendThread.h
 * @author kubanec
 * @date 12.9.2012
 *
 */

#ifndef SENDTHREAD_H_
#define SENDTHREAD_H_

namespace rfm
{
namespace threads
{

class SendThread: public chibios_rt::EnhancedThread<128>
{
public:
	SendThread();

private:
	msg_t Main(void);
	void Send(packet_t & packet);
	void Wait(uint8_t destination);
};

} //namespace threads
} /* namespace rfm */
#endif /* SENDTHREAD_H_ */
