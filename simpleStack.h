/*
 * simpleStack.h
 *
 *  Created on: 4.12.2012
 *      Author: kubanec
 */

#ifndef SIMPLESTACK_H_
#define SIMPLESTACK_H_

template<int T, class CO> class simpleStack
{
public:
	bool push(CO * data)
	{
		if (!full())
		{
			buffer[index++] = data;
			return true;
		}
		return false;
	}
	CO * pop()
	{
		if (!empty())
			return buffer[index--];
		else
			return NULL;
	}
	bool empty()
	{
		return (index == 0);
	}
	bool full()
	{
		return (index == T);
	}
private:
	CO * buffer[T];
	uint8_t index;
};

#endif /* SIMPLESTACK_H_ */
