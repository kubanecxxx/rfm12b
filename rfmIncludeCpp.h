/*
 * rfmIncludeCpp.h
 *
 *  Created on: 11.9.2012
 *      Author: kubanec
 */

#ifndef RFMINCLUDECPP_H_
#define RFMINCLUDECPP_H_

#include "rfmIncludeC.h"
#include "ch.hpp"
#include "hal.h"

/*
 * constants set
 */
namespace rfm
{
const uint8_t TIMESLOT = 25;
const uint8_t LOAD_LENGTH = 8;
const uint8_t PACKET_LENGTH = LOAD_LENGTH + 1;
const uint16_t TIME = TIMESLOT * LOAD_LENGTH * 2;
const int8_t MASTER = 1;
}

/*
 * include set
 */

#include "LinkLayer.h"
#include "RecThread.h"
#include "SendThread.h"
#include "ApplicationLayer.h"

#endif /* RFMINCLUDECPP_H_ */
