/*
 * slaveLoop.c
 *
 *  Created on: Mar 15, 2024
 *      Author: tateneedham
 */

#include "slaveLoop.h"
#include "stdbool.h"
#include "buffers.h"
#include "stm32l0xx_hal.h"
#include "string.h"
#include "usart.h"
#include "xbeeChecksum.h"
#include "beep.h"
#include "leds.h"

bool xbeeTransmit(uint8_t* txBuffer, uint8_t txBufferSize);
void blinkAndBeepForPunch();
void resetBlinkAndBeepForPunch();

/* Tracker variable for last time LEDs blinked */
uint32_t timeSinceLastPunchLEDBlink = 0;

void slaveLoop() {
	bool success = false;
	/* Handle the slave mode */
	/* Here, 4 things could happen:	 */
	/* - A packet from the red radio comes in. We send this out via the XBee */
	/* - A packet from the blue radio comes in. We send this out via the XBee */
	/* - A packet from the aux radio comes in. We send this out via the XBee */
	/* - A packet from the XBee comes in. We handle this command */

	if (radioRedPacketComplete == true)
	{
		success = xbeeTransmit((uint8_t *)radioRedBuffer, radioRedTracker);

		memset((uint8_t *)radioRedBuffer, 0, 100);
		radioRedTracker = 0;

		radioRedPacketComplete = false;
	}

	if (radioBluePacketComplete == true)
	{
		success = xbeeTransmit((uint8_t *)radioBlueBuffer, radioBlueTracker);

		memset((uint8_t *)radioBlueBuffer, 0, 100);
		radioBlueTracker = 0;

		radioBluePacketComplete = false;
	}

	if (radioAuxPacketComplete == true)
	{
		success = xbeeTransmit((uint8_t *)radioAuxBuffer, radioAuxTracker);

		memset((uint8_t *)radioAuxBuffer, 0, 100);
		radioAuxTracker = 0;

		radioAuxPacketComplete = false;
	}

	if (success) {
		blinkAndBeepForPunch();
	}

	resetBlinkAndBeepForPunch();
}

bool xbeeTransmit(uint8_t* txBuffer, uint8_t txBufferSize) {

	uint16_t packetLength;
	uint8_t checksum;

	/* Let's build the packet */
	/* First, it's the Start of File indicator byte */
	xbeeTXBuffer[0] = SOF;

	/* Second, the Packet Length. This equals Payload Length + 14 Bytes (CMD-ID(1B) + frame id(1B) +  64-bit addr + 16 bit-addr + 2B) */
	packetLength = txBufferSize + 14;

	xbeeTXBuffer[1] = (uint8_t) (packetLength >> 8);
	xbeeTXBuffer[2] = (uint8_t) (packetLength);

	/* Command ID */
	xbeeTXBuffer[3] = CMD_TX_REQUEST;

	/* Frame ID - 0x00 = disables response frame */
	xbeeTXBuffer[4] = 0x01;

	/* 64-Bit Destination Address. In broadcast mode, this is 0x000000000000FFFF */
	xbeeTXBuffer[5] = 0x00;
	xbeeTXBuffer[6] = 0x00;
	xbeeTXBuffer[7] = 0x00;
	xbeeTXBuffer[8] = 0x00;
	xbeeTXBuffer[9] = 0x00;
	xbeeTXBuffer[10] = 0x00;
	xbeeTXBuffer[11] = 0xFF;
	xbeeTXBuffer[12] = 0xFF;

	/* 16-Bit Address. In broadcast mode, this is 0xFFFE */
	xbeeTXBuffer[13] = 0xFF;
	xbeeTXBuffer[14] = 0xFE;

	/* Broadcast Radius - maximum number of hops. 0 = max */
	xbeeTXBuffer[15] = 0x00;

	/* Transmit Options. We won't use any here */
	xbeeTXBuffer[16] = 0x00;

	/* Paste the payload in */
	memcpy(&xbeeTXBuffer[17], txBuffer, txBufferSize);

	/* Calculate the checksum */
	checksum = xbeeChecksum(&xbeeTXBuffer[3], packetLength);

	/* Insert the checksum */
	xbeeTXBuffer[17 + txBufferSize] = checksum;

	/* Transmit the packet to the XBee */
	HAL_StatusTypeDef transmitStatus = HAL_UART_Transmit(&huart1, xbeeTXBuffer, packetLength + 4, 100);

	/* Flash a code to indicate the error state
	 * The Beeps and flashes are blocking, so remove for release build */
	switch (transmitStatus) {
	case HAL_OK:
		return true;
	case HAL_ERROR:
		BlockingErrorAlert(2);
		break;
	case HAL_BUSY:
		BlockingErrorAlert(3);
		break;
	case HAL_TIMEOUT:
		BlockingErrorAlert(4);
		break;
	}
	return false;
}

void blinkAndBeepForPunch(void) {
	timeSinceLastPunchLEDBlink = updateLED(Rssi1LED, LED_ON);  // PunchLED is not working on 1 board, so use RSSI1 for now
	startBeep();
}

void resetBlinkAndBeepForPunch(void) {
	if (HAL_GetTick() - timeSinceLastPunchLEDBlink > 300)
	{
		updateLED(Rssi1LED, LED_OFF);  // PunchLED is not working on 1 board, so use RSSI1 for now
		endBeep();
	}
}

