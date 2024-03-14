/*
 * masterLoop.c
 *
 *  Created on: Mar 15, 2024
 *      Author: tateneedham
 */

#include "masterLoop.h"
#include "stdbool.h"
#include "buffers.h"
#include "stm32l0xx_hal.h"
#include "string.h"
#include "usart.h"
#include "xbeeChecksum.h"
#include "beep.h"
#include "leds.h"

void masterLoop() {
	/* Handle the master mode */
	/* Here, 3 things could happen:	 */
	/* - A packet from the Xbee comes in. We handle this data */
	/* - A packet from the PC comes in. We handle this command (TODO) */
	/* - The heartbeat button is pushed. We send out a signal to make the slaves blink (TODO) */

	if (xbeePacketComplete == true)
	{

		/* We need to find where the packet starts. Sometimes this is 14, 15, 16 etc. indexes into the buffer */
		/* There's also a chance both the XBee AND the radio packet are atypical sizes, and with both having their own overhead/structure */
		/* You can see, it's actually proving quite hard to work out where one starts and the other ends because we have embedded radio packets */
		/* We also can't just look for the radio EOF indicator (0x03) since the XBee packet might have this inside of it, too! */
		/* Temporary solution is to look for what seems to be a unique key at the start of the punch radio message: */
		/* - 0xFF (alert, packet incoming) */
		/* - 0x02 (STX, start of text) */
		/* - 0xD3 (transmit punch data) */
		/* This is taken from Page 5 of the SportIdent PC programmers guide */
		/* We can then use the next byte to work out the length. This is similar to the XBee radio structure but is more appropriate to use here */
		/* If this exact key isn't received, then we can discard it and it must be noise or some message we aren't interested in */
		/* This is STILL flawed though! THere's a chance the XBee might have that exact ^ structure embedded in its header */
		uint8_t index;
		uint8_t radioPacketLength;

		for (index = 0; index <= 100; index++)
		{

			/* See if this matches the start of a valid punch data radio packet */
			if (xbeeRxBuffer[index] == 0xFF && xbeeRxBuffer[index + 1] == 0x02 && xbeeRxBuffer[index + 2] == 0xD3)
			{

				/* Grab the length - this is the next byte, but also add back in the overhead (4 bytes) and checksum (3 bytes) */
				/* This means rather than the interesting data, we are transmitting exactly as if the SRR was plugged into the PC */
				/* This can be easily removed later, if we don't want to worry about the checksum and overhead and just want the pure punch data */
				radioPacketLength = xbeeRxBuffer[index + 3] + 4 + 3;

				memcpy(transmitBuffer, (uint8_t *)&xbeeRxBuffer[index], radioPacketLength);
				HAL_UART_Transmit(&huart5, transmitBuffer, 20, 100);
				break;
			}

		}

		/* If the for loop ends normally, then nothing was transmitted and the message was discarded */
		memset((uint8_t *)xbeeRxBuffer, 0, 100);
		xbeePacketComplete = false;

		HAL_UART_Receive_IT(&huart1, (uint8_t *)xbeeRxBuffer, 1);

	}

	if (PCPacketComplete == true)
	{
		/* Do something with the PC Packet */

		memset((uint8_t *)PCBuffer, 0, 100);
		PCTracker = 0;

		PCPacketComplete = false;
	}

}
