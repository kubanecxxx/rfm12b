/*
 * simpleStack.h
 *
 *  Created on: 4.12.2012
 *      Author: kubanec
 */

#ifndef SIMPLESTACK_H_
#define SIMPLESTACK_H_

/**
 * @brief jednoduchý kruhový zásobník
 */
template<int T, class CO> class simpleQueue
{
public:
	inline simpleQueue()
	{
		clear();
	}
	bool pushBack(CO data)
	{
		if (!full())
		{
			buffer[write++] = data;
			if (write == T)
				write = 0;
			return true;
		}
		return false;
	}

	CO popFront()
	{
		if (!empty())
		{
			CO temp = buffer[read++];
			if (read == T)
				read = 0;
			return temp;
		}
		else
			return NULL;
	}

	inline bool empty() const
	{
		return (read == write);
	}
	uint8_t count() const
	{
		if (empty())
			return 0;

		uint8_t temp = write;
		if (read > write)
			temp += T;

		return (temp - read);
	}
	bool full() const
	{
		return (count() == T);
	}
	inline void clear()
	{
		read = 0;
		write = 0;
	}
private:
	CO buffer[T];
	uint8_t write;
	uint8_t read;
};

#endif /* SIMPLESTACK_H_ */
