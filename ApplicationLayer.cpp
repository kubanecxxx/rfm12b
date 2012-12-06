/**
 * @file ApplicationLayer.cpp
 * @author kubanec
 * @date 3.12.2012
 *
 */

#include "rfmIncludeCpp.h"

namespace rfm
{

const ApplicationLayer::error_cb_t * ApplicationLayer::error_callbacks = NULL;
const ApplicationLayer::user_callbacks_t * ApplicationLayer::user_callbacks =
		NULL;
uint16_t ApplicationLayer::callback_count = 0;

void ApplicationLayer::Init(uint8_t address, uint16_t slaves,
		const error_cb_t * error_cb, const user_callbacks_t * user_cb,
		uint16_t count)
{
	callback_count = count;
	error_callbacks = error_cb;
	user_callbacks = user_cb;
	LinkLayer::Init(address, slaves);
}

/**
 * tady je paket kterej prošel všema kontrolama a je v pořádku
 * v připadě mastera i slava
 *
 * paket se tady musí rozpoznat a zpracovat, vyházet callbacky userovi
 * zapsat data do modbusu
 * poslat odpověď po modbuse a tak
 *
 */

/*
 * rozeznat paket
 * protože přinde odpověď ok/nok get/set
 * zpracovat paket
 * nastavit modbus pokud to bylo get
 * hodit callback
 * ok/nok dat do callbacku
 *
 * rozeznání paketu co přišel k tomu co je v bufferu
 */
void ApplicationLayer::processPacket(packet_t * packet)
{
	/*
	 * projet typy paketů get/set ok/nok/get/set
	 */
	/*
	 * tyhle dva mužou nastat jedině pro slave
	 */
	if (packet->IsGet())
	{
		/*
		 * vybere ktery data si master žádá
		 * a vytahne je ze svoji paměti
		 * pokud se mu zadaři a adresa byla
		 * v rozsahu kterej má mapovanej
		 * tak je vytahne z tabulky a pošle je zpátky
		 * + vyhodi callback
		 */
		uint32_t data = getUserData(packet->GetAddress());
		bool ok = (data != 0xffffffff);
		packet_t pak(*packet);
		pak.AnswerGet(data, ok);
		pak.Send();

	}
	else if (packet->IsSet())
	{
		/*
		 * nastavi data ktery master chce takle nastavit
		 * pokud je to v rozsahu tak provede a odpovi
		 * masterovi a vyhodi chcallback
		 */
		bool ok = setUserData(packet->GetAddress(), packet->GetData());
		packet_t pak(*packet);
		pak.AnswerSet(ok);
		pak.Send();
	}
	/*
	 * zbytek muže nastat jenom u mastera
	 */
	/**
	 * @todo vymyslet jak adresovat modbus pole
	 */
	else if (packet->IsGetOK())
	{

	}
	else if (packet->IsSetOK())
	{

	}
	/**
	 * @todo vymyslet systém volání cb pro tyhle hlášky
	 * co mu cpat do parametru
	 */
	else if (packet->IsSetNok())
	{

	}
	else if (packet->IsGetNok())
	{

	}
}

bool ApplicationLayer::setUserData(uint16_t address, uint32_t data)
{
	if (user_callbacks && address < callback_count
			&& user_callbacks[address].modbus_address)
	{
		*(user_callbacks[address].modbus_address) = data;
		return true;
	}
	return false;
}
uint32_t ApplicationLayer::getUserData(uint16_t address)
{
	if (user_callbacks && address < callback_count
			&& user_callbacks[address].modbus_address)
	{
		return *user_callbacks[address].modbus_address;
	}
	return -1;
}

void ApplicationLayer::processUserCB(bool ok, user_cb_packet_t type,
		uint8_t address, uint16_t modbus_addr, uint32_t modbus_data)
{
	if (user_callbacks && user_callbacks[address].cb)
	{
		user_callbacks[address].cb(ok, type, address, modbus_addr, modbus_data);
	}
}

}
/* namespace rfm */
