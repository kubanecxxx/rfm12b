/*
 * rfm_layer2.h
 *
 *  Created on: 10.6.2012
 *      Author: kubanec
 */

#ifndef RFM_LAYER2_H_
#define RFM_LAYER2_H_

typedef struct rfm_PacketTypedef
{
	uint16_t length;
	uint8_t * dataPtr;
	uint8_t Dest;
	uint8_t Source;
} rfm_PacketTypedef;

#define RFM_HLAVICKY 12

void rfm_SendPacket (const rfm_PacketTypedef * packet);


#endif /* RFM_LAYER2_H_ */
