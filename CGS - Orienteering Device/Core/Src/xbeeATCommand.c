/*
 * xbeeATCommand.c
 *
 *  Created on: Mar 6, 2024
 *      Author: tateneedham
 */

#include "xbeeATCommand.h"
#include "stm32l0xx_hal.h"
#include "xbeeConstants.h"
#include "string.h"
#include "usart.h"
#include "xbeeChecksum.h"
#include "stdio.h"
#include "xbeeState.h"
#include "stmSerial.h"

/// Send an AT Command to local XBee
/// This is a blocking call, using both blocking UART send and receive APIs
///  - Parameters
///    - atCommand: the AT Command, eg. NI to get Node Identifier
///    - parameterValue: if the AT Command needs a parameter
///    - parameterLength: how long is the parameter
///    - result: returns the result payload of the AT command here. Result array should be initialised and big enough to hold result.
///    - resultLength: so we know how much data to look for in the result
///    - error: if an error occurs, have a look in this to see what went wrong
///  - Returns: true for success
bool xbeeSendATCommand(char *atCommand, bool isGet, uint8_t *parameterValue, int parameterLength, uint8_t *result, int *resultLength, char *error) {

    // Example for Node Identifier:
    //   Request:  SOF 00 04 08 01 'N' 'I' 5F                        ['N' = 0x4E, 'I' = 0x49]
    //   Response: SOF 00 09 88 01 'N' 'I' 00 'N' 'O' 'D' 'E' CHK

    xbeeState = ATCommand;

    uint8_t txBuffer[100] = {0};
    uint8_t rxBuffer[100] = {0};

    int commandLength = strlen(atCommand);
    HAL_StatusTypeDef status;

    int txPacketLength = commandLength + parameterLength + 2;                            // 2 = command + frame id
    int preambleLength = 5;                                                              // 5 = SOF + length MSB + length LSB + command + frameid
    int fullFrameLength = commandLength + parameterLength + preambleLength + 1;          // 1 = checksum

    txBuffer[0] = SOF;                                                                   // Start of file
    txBuffer[1] = (uint8_t) (txPacketLength >> 8);                                       // Length MSB
    txBuffer[2] = (uint8_t) (txPacketLength);                                            // Length LSB
    txBuffer[3] = XBEE_AT_COMMAND;                                                       // Command ID: 0x08 = AT Command
    txBuffer[4] = 0x01;                                                                  // Frame ID
    memcpy(&txBuffer[preambleLength], atCommand, commandLength);                         // AT Command
    memcpy(&txBuffer[preambleLength + commandLength], parameterValue, parameterLength);  // Parameter (if required)
	int checksum = xbeeChecksum(&txBuffer[3], txPacketLength);                           // Calculate the checksum (excluding SOF & length)
    txBuffer[preambleLength + commandLength + parameterLength] = checksum;               // Insert the checksum

    serialLogXBeeFrame(atCommand, isGet ? GetFrame : SetFrame, txBuffer, fullFrameLength);

	// Transmit the packet to the XBee
	status = HAL_UART_Transmit(&huart1, txBuffer, fullFrameLength, 2000);
    if (status != HAL_OK) {
        sprintf(error, "Transmit fail: %c", status);
        xbeeState = Idle;
        return false;
    }

    // Receive the first 3 bytes so we get the length and can get the rest
    status = HAL_UART_Receive(&huart1, rxBuffer, 3, 13000);

    if (status != HAL_OK) {
        sprintf(error, "Receive first 3 bytes fail: %c", status);
        xbeeState = Idle;
        return false;
    }
    if (rxBuffer[0] != SOF) {
        sprintf(error, "SOF not found");
        xbeeState = Idle;
        return false;
    }

    int rxPacketLength = (rxBuffer[1] << 8) | (rxBuffer[2] << 0);

    // Receive the rest of the response
    status = HAL_UART_Receive(&huart1, &rxBuffer[3], rxPacketLength + 1, 2000);          // +1 for the checksum, insert in to the buffer from position 3
    if (status != HAL_OK) {
        sprintf(error, "Receive rest of bytes fail: %c", status);
        xbeeState = Idle;
        return false;
    }
    serialLogXBeeFrame(atCommand, ResponseFrame, rxBuffer, rxPacketLength + 4);

    // Check for command response
    if (rxBuffer[3] != XBEE_AT_COMMAND_RESPONSE) {                                       // 0x88 = AT Command response
        sprintf(error, "Invalid command response: %x", rxBuffer[3]);
        xbeeState = Idle;
        return false;
    }

    // Check that the command matches
    for(int i = 0; i < commandLength; i++) {
    	if(rxBuffer[preambleLength + i] != atCommand[i]) {
			sprintf(error, "Invalid command received");
			xbeeState = Idle;
			return false;
    	}
    }

    // Check for success
	//    0 = OK
	//    1 = ERROR
	//    2 = Invalid command
    //    3 = Invalid parameter
    //    4 = Status OK [at least for the DN command]
    if ((rxBuffer[preambleLength + commandLength] != XBEE_AT_SUCCESS) && (rxBuffer[preambleLength + commandLength] != XBEE_AT_STATUS_OK)) {
        sprintf(error, "Xbee returned false: %x", rxBuffer[preambleLength + commandLength]);
        xbeeState = Idle;
        return false;
    }

    // Extract the result
    int length = rxPacketLength - commandLength - 3;                                     // 3 = command response + frame ID + success
    *resultLength = length;
    memcpy(result, &rxBuffer[preambleLength + commandLength + 1], length);               // Index in rxBuffer: preamble + command + result,

    xbeeState = Idle;

    return true;
}

/// Get a value from the XBee using an AT Command
/// This is a blocking call
bool xbeeATCommandGetValue(char *atCommand, uint8_t *result, int *resultLength, char *error) {
	uint8_t parameterValue[0];
	return xbeeSendATCommand(atCommand, true, parameterValue, 0, result, resultLength, error);
}

/// Set a value on the XBee using an AT Command
/// This is a blocking call
bool xbeeATCommandSetValue(char *atCommand, uint8_t *parameterValue, int parameterLength, char *error) {
	uint8_t result[0];
	int *resultLength = 0;
	return xbeeSendATCommand(atCommand, false, parameterValue, parameterLength, result, resultLength, error);
}
