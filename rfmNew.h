/**
 * @file rfmNew.h
 * @author kubanec
 * @date 12.9.2012
 *
 */

#ifndef RFMNEW_H_
#define RFMNEW_H_

namespace rfm
{

class rfmNew
{
public:
	static void * operator new (size_t size)
	{
		return chCoreAlloc(size);
	}
};

} /* namespace rfm */
#endif /* RFMNEW_H_ */
