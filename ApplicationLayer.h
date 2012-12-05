/**
 * @file ApplicationLayer.h
 * @author kubanec
 * @date 3.12.2012
 *
 */

#ifndef APPLICATIONLAYER_H_
#define APPLICATIONLAYER_H_

namespace rfm
{

class ApplicationLayer
{
public:
	/*
	 * sada chybovéch callbacku
	 * bude jenom jedna pro všechno
	 */
	typedef void (*timeout_cb_t)(uint8_t slave);
	typedef void (*arqError_cb_t)(void);
	typedef struct
	{
		timeout_cb_t timeout;
		arqError_cb_t arq;
	} error_cb_t;

	typedef enum
	{
		SET, GET
	} user_cb_packet_t;
	typedef void (*user_cb_t)(bool ok, user_cb_packet_t type, uint8_t address,
			uint16_t modbus_addr, uint32_t modbus_data);
	typedef struct
	{
		user_cb_t cb;
		uint32_t * modbus_address;
	} user_callbacks_t;

	static void Init(uint8_t address, uint16_t slaves,
			const error_cb_t * error_cb, const user_callbacks_t * user_cb,
			uint16_t count);

private:
	static void Send(uint8_t slave_address, uint8_t modbus_address,
			user_cb_packet_t type, uint32_t data);
	friend class LinkLayer;
	static const error_cb_t * error_callbacks;
	static const user_callbacks_t * user_callbacks;
	static uint16_t callback_count;

	inline static void processTimeoutErrorCb(uint8_t addr)
	{
		if (error_callbacks && error_callbacks->timeout)
			error_callbacks->timeout(addr);

	}
	inline static void processArqErrorCb()
	{
		if (error_callbacks && error_callbacks->arq)
			error_callbacks->arq();
	}

	static void processPacket(packet_t * packet);
	inline static bool setUserData(uint16_t address, uint32_t data);
	inline static uint32_t getUserData(uint16_t address);
	inline static void processUserCB(bool ok, user_cb_packet_t type,
			uint8_t address, uint16_t modbus_addr, uint32_t modbus_data);
};

} /* namespace rfm */
#endif /* APPLICATIONLAYER_H_ */
