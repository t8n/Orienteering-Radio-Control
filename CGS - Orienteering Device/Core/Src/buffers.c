/*
 * buffers.c
 *
 *  Created on: Mar 14, 2024
 *      Author: tateneedham
 */

#include "buffers.h"

/* Incoming channel buffers */
volatile uint8_t radioRedBuffer  	[100];
volatile uint8_t radioBlueBuffer 	[100];
volatile uint8_t radioAuxBuffer		[100];

volatile uint8_t PCBuffer  			[100];
volatile uint8_t xbeeRxBuffer 		[100];

uint8_t xbeeTXBuffer [XBEE_MAX_PACKET_SIZE];

/* Incoming single byte (we only handle 1 byte at a time) */
uint8_t radioRedIn = 0;
uint8_t radioBlueIn = 0;
uint8_t radioAuxIn = 0;

uint8_t PCIn = 0;
uint8_t xbeeIn = 0;

/* Tracker to keep track of where in the buffer we are */
uint8_t radioRedTracker = 0;
uint8_t radioBlueTracker = 0;
uint8_t radioAuxTracker = 0;

uint8_t PCTracker = 0;
uint8_t xbeeTracker = 0;

/* Boolean flag for when a packet is complete and ready to be interpreted */
bool radioRedPacketComplete = false;
bool radioBluePacketComplete = false;
bool radioAuxPacketComplete = false;

bool PCPacketComplete = false;
bool xbeePacketComplete = false;

/* Variable for which stage of the packet we are at */
volatile uint8_t xbeeStep = 1;
volatile uint16_t xbeeSize = 0;

/* Outgoing buffer */
uint8_t transmitBuffer	[100];

/* Timeout counters */
uint32_t xbeeTimeout = 0;

