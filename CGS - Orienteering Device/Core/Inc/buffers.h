/*
 * buffers.h
 *
 *  Created on: Mar 14, 2024
 *      Author: tateneedham
 */

#ifndef INC_BUFFERS_H_
#define INC_BUFFERS_H_

#include "stm32l0xx_hal.h"
#include "xbeeConstants.h"
#include "stdbool.h"

/* Incoming channel buffers */
extern volatile uint8_t radioRedBuffer [100];
extern volatile uint8_t radioBlueBuffer [100];
extern volatile uint8_t radioAuxBuffer [100];

extern volatile uint8_t PCBuffer  			[100];
extern volatile uint8_t xbeeRxBuffer 		[100];

extern uint8_t xbeeTXBuffer [XBEE_MAX_PACKET_SIZE];

/* Incoming single byte (we only handle 1 byte at a time) */
extern uint8_t radioRedIn;
extern uint8_t radioBlueIn;
extern uint8_t radioAuxIn;

extern uint8_t PCIn;
extern uint8_t xbeeIn;

/* Tracker to keep track of where in the buffer we are */
extern uint8_t radioRedTracker;
extern uint8_t radioBlueTracker;
extern uint8_t radioAuxTracker;

extern uint8_t PCTracker;
extern uint8_t xbeeTracker;

/* Boolean flag for when a packet is complete and ready to be interpreted */
extern bool radioRedPacketComplete;
extern bool radioBluePacketComplete;
extern bool radioAuxPacketComplete;

extern bool PCPacketComplete;
extern bool xbeePacketComplete;

/* Variable for which stage of the packet we are at */
extern volatile uint8_t xbeeStep;
extern volatile uint16_t xbeeSize;

/* Outgoing buffer */
extern uint8_t transmitBuffer	[100];

/* Timeout counters */
extern uint32_t xbeeTimeout;

#endif /* INC_BUFFERS_H_ */
