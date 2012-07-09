/*
 * rfm_layer2.c
 *
 *  Created on: 10.6.2012
 *      Author: kubanec
 */

#include "rfm_port.h"
#include "rfm.h"
#include "rfm_layer2.h"

#define RFM_PACKET_TUPOUN

void rfm_SendPacket (const rfm_PacketTypedef * packet)
{
#ifdef RFM_PACKET_TUPOUN
	unsigned char i;

	//hlavičky
	rf_prepare();
	rf_send(packet->Source);
	rf_send(packet->Dest);
	//data
	rf_send(packet->length);
	for (i = 0 ; i < packet->length; i++)
	{
		rf_send(packet->dataPtr[i]);
	}
	//dummy
	rf_send(0xAA);
	rf_send(0xAA);
#else
	unsigned char i;
	uint8_t * data;
	uint16_t delka;

	delka = RFM_HLAVICKY + packet->length;

	data = (uint8_t *) malloc(delka);

	//dmáčkem vyblit ven data

#endif
}
