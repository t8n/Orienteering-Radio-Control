/*
 * uartInterruptHandlers.c
 *
 *  Created on: Mar 15, 2024
 *      Author: tateneedham
 */

#include "mainStateMachine.h"
#include "usart.h"
#include "stdio.h"
#include "stdlib.h"
#include "buffers.h"
#include "masterSlaveMode.h"
#include "srrConstants.h"
#include "string.h"
#include "stmSerial.h"
#include "xbeeConstants.h"
#include "xbeeMesh.h"
#include "uartInterruptHandlers.h"

uint8_t bytesLeft = 0;
uint8_t byteLocation = 0;

void handleXBeeRx();
void lookForXBee();
void handleSRRRedRx();
void handleSRRBlueRx();
void handleSRRAuxRx();
void handlePCRx();

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART1) {
		handleXBeeRx();
		return;
	}

	if (masterSlaveMode == Slave) {

		if (huart->Instance == USART4) {
			handleSRRRedRx();
			return;
		}

		if (huart->Instance == USART2) {
			handleSRRBlueRx();
			return;
		}

		if (huart->Instance == USART5) {
			handleSRRAuxRx();
			return;
		}

	} else if (masterSlaveMode == Master) {

		if (huart->Instance == USART5) {
			handlePCRx();
			return;
		}

	}
}

typedef enum {
  WaitingForSOF,
  WaitingForLength,
  WaitingForMessage
} XBee_RX_State;

XBee_RX_State rxState = WaitingForSOF;

void resetXBeeUartCallback() {
    rxState = WaitingForSOF;
    HAL_UART_Receive_IT(&huart1, (uint8_t *)xbeeRxBuffer, 1);
}

void processXBeeMessage() {
    int messageSize = (xbeeRxBuffer[1] << 8) | (xbeeRxBuffer[2] << 0);

    if (xbeeRxBuffer[3] == XBEE_AT_COMMAND_RESPONSE) {

        if (memcmp((uint8_t *)xbeeRxBuffer + XBEE_AT_PREAMBLELENGTH, XBEE_AT_DISCOVER_NODE, 2) == 0) {
            // serialLogMessage("Heartbeat response received", true);
            processHeartbeatResponse((uint8_t *)xbeeRxBuffer, messageSize);
            return;
        }

        char message[100] = {0};
        sprintf(message, "No handler for XBee command: %c%c", xbeeRxBuffer[XBEE_AT_PREAMBLELENGTH], xbeeRxBuffer[XBEE_AT_PREAMBLELENGTH + 1]);
        serialLogMessage(message, true);
        return;
    }

    serialLogMessage("No handler for XBee message", true);
}

void handleXBeeRx() {

    // A special case for when everything is booting up
	if (machineState == LookingForXBee) {
		lookForXBee();
		return;
	}

	switch (rxState) {
	case WaitingForSOF:
	    if (xbeeRxBuffer[0] == SOF) {
	        rxState = WaitingForLength;
	        HAL_UART_Receive_IT(&huart1, (uint8_t *)&xbeeRxBuffer[1], 2);
	        return;
	    }
	    break;

    case WaitingForLength:
        int messageSize = (xbeeRxBuffer[1] << 8) | (xbeeRxBuffer[2] << 0);
        HAL_UART_Receive_IT(&huart1, (uint8_t *)&xbeeRxBuffer[3], messageSize + 1); // include the checksum
        rxState = WaitingForMessage;
        break;

    case WaitingForMessage:
        serialLogMessage("", true);
        serialLogMessage("XBee message received", true);

        processXBeeMessage();

        memset((uint8_t *)xbeeRxBuffer, 0, 100);

        // listen for the next frame
        rxState = WaitingForSOF;
	    HAL_UART_Receive_IT(&huart1, (uint8_t *)xbeeRxBuffer, 1);
	    break;
	}









    return;

	/* Handle XBee input - this happens independent of slave or master mode */

	/* Unlike PC where we just look for newline ('\n'), or the radios where we look for NAK/ETX byte, this isn't trivial */
	/* First, we look for the SOF (start of file) byte. When we get this, we know the next 2 bytes will be the length */
	/* Second, we read the length which is 2 bytes. Then, we use this information to wait for the final amount of data incoming */
	/* Third and final, we read the rest of the data */

	serialLogMessage("handleXBeeRx", true);
	// All other commands start with 3 bytes: SOF, Length MSB and Length LSB
	if (xbeeStep == 1) {
		// check for SOF and calculate the size of the message
		if (xbeeRxBuffer[0] == SOF) {
			xbeeSize = (xbeeRxBuffer[1] << 8) | (xbeeRxBuffer[2] << 0);
			xbeeSize += 1;

			// save the first part of the message in the buffer, so that next time around the loop we know how big the payload is
			HAL_UART_Receive_IT(&huart1, (uint8_t *)&xbeeRxBuffer[3], 1);
			bytesLeft = xbeeSize - 1;
			byteLocation = 4;
			xbeeStep = 2;
			return;
		}
	}

	if (xbeeStep == 2) {
		HAL_UART_Receive_IT(&huart1, (uint8_t *)&xbeeRxBuffer[byteLocation], 1);
		byteLocation += 1;
		bytesLeft -= 1;
		if (bytesLeft == 0) {
			xbeeStep = 3;
		}
		return;
	}

	// This is receiving _most_ of the XBee reset sequence: 00 02 8A 00 75. It's missing the 7E at the start
	// https://www.digi.com/resources/documentation/Digidocs/90001500/Reference/r_frame_0x8A.htm
	// Changing it to interrupt doesn't work (ie. HAL_UART_Receive_IT)
//		memset(xbeeBuffer,'\0',10);
//		HAL_UART_Receive(&huart1, (uint8_t *)xbeeBuffer, 10, 100);

	xbeeTimeout = HAL_GetTick();

	if (xbeeStep == 1)
	{

		/* This is a valid XBee packet */
		/* Proceed to the next step, shift the buffer pointer up 1, and wait for 2 bytes */
		/* These 2 bytes will be the length */
		if (xbeeRxBuffer[0] == SOF)
		{

			xbeeStep = 2;
			xbeeSize = 2;
			HAL_UART_Receive_IT(&huart1, (uint8_t *)&xbeeRxBuffer[1], xbeeSize);

		}
		/* Otherwise, this is an invalid packet/we got out of sync */
		/* Restart and continue the hunt for a valid packet */
		else
		{
			xbeeStep = 1;
			HAL_UART_Receive_IT(&huart1, (uint8_t *)xbeeRxBuffer, 1);
		}

	}
	else if (xbeeStep == 2)
	{

		/* Here, we calculate the length of the packet incoming */
		/* This is then used for the final step where we grab the data we want */
		xbeeSize = (xbeeRxBuffer[1] << 8) | (xbeeRxBuffer[2] << 0);

		/* The real size is an additional 1 bytes on top of this for the checksum */
		xbeeSize += 1;

		/* We add a check here to make sure the length is sensible */
		/* If it is sensible, we move up the buffer a few positions and look for the remaining packet size */
		if (xbeeSize <= XBEE_MAX_PACKET_SIZE)
		{
			xbeeStep = 3;
			HAL_UART_Receive_IT(&huart1, (uint8_t *)&xbeeRxBuffer[3], xbeeSize);
		}
		/* Otherwise, this is an invalid packet/we got out of sync */
		/* Restart and continue the hunt for a valid packet */
		else
		{
			xbeeStep = 1;
			HAL_UART_Receive_IT(&huart1, (uint8_t *)xbeeRxBuffer, 1);
		}
	}
	else if (xbeeStep == 3)
	{

		/* Here, we have received the packet of data */
		/* There are several checks we need to make here */
		/* First, check that it's a valid Orienteering Device packet */
//			if (xbeeBuffer[0] == OD_ID
//			 && xbeeBuffer[1] == currentChannel)
//			{

			xbeePacketComplete = true;

//			}
		/* Whether it was a valid packet or not, we begin scanning again for new data */
		xbeeStep = 1;
		HAL_UART_Receive_IT(&huart1, (uint8_t *)xbeeRxBuffer, 1);

	}

}

void handleSRRRedRx() {
	radioRedBuffer[radioRedTracker] = radioRedIn;
	radioRedTracker++;

	if (radioRedIn == NAK || radioRedIn == ETX)
	{
		radioRedPacketComplete = true;
	}

	HAL_UART_Receive_IT(&huart4, &radioRedIn, 1);
}

void handleSRRBlueRx() {
	radioBlueBuffer[radioBlueTracker] = radioBlueIn;
	radioBlueTracker++;

	if (radioBlueIn == NAK || radioBlueIn == ETX)
	{
		radioBluePacketComplete = true;
	}

	HAL_UART_Receive_IT(&huart2, &radioBlueIn, 1);
}

void handleSRRAuxRx() {
	radioAuxBuffer[radioAuxTracker] = radioAuxIn;
	radioAuxTracker++;

	if (radioAuxIn == NAK || radioAuxIn == ETX)
	{
		radioAuxPacketComplete = true;
	}

	HAL_UART_Receive_IT(&huart5, &radioAuxIn, 1);
}

void lookForXBee() {
	// Look for the XBee Reset Frame - this will make sure we have an XBee
	if (memcmp((uint8_t *)xbeeRxBuffer, XBEE_RESET_SEQUENCE, sizeof(XBEE_RESET_SEQUENCE)) == 0) {
		xbeeFound();
	} else {
		// If we don't get the reset sequence we don't know if an XBee is attached.
		xbeeNotFound();
	}
}

void handlePCRx() {
	PCBuffer[PCTracker] = PCIn;
	PCTracker++;

	if (PCIn == '\n') {
		PCPacketComplete = true;
	}

	HAL_UART_Receive_IT(&huart5, &PCIn, 1);
}
