/*
 * stmSerial.c
 *
 *  Created on: Mar 13, 2024
 *      Author: tateneedham
 */

#include "stm32l0xx_hal.h"
#include "string.h"
#include "usart.h"
#include "stdio.h"
#include "stdbool.h"
#include "stmSerial.h"

void serialLogClearScreen() {
	HAL_UART_Transmit(&huart5, (uint8_t *)"\033[2J", 4, 1000);
	serialLogMessage("", true);
}

void serialLogMessage(char *message, bool addNewLine) {
	serialLogBuffer((uint8_t *)message, strlen(message), false, addNewLine);
}

void serialLogBuffer(uint8_t *buffer, int length, bool convertToHex, bool addNewLine) {
	if (convertToHex) {
		char hexString[3];
		for (int i = 0; i < length; i++) {
			sprintf(hexString, "%02x", buffer[i]);
			HAL_UART_Transmit(&huart5, (uint8_t *)hexString, 3, 1000);
		}
	} else {
		HAL_UART_Transmit(&huart5, buffer, length, 1000);
	}

	if (addNewLine) {
		HAL_UART_Transmit(&huart5, (uint8_t *)"\n\r", 2, 1000);
	}
}

void serialLogXBeeFrame(char *atCommand, Log_Frame_Type frameType, uint8_t *frame, int length) {
	switch (frameType) {
	case GetFrame:
		serialLogMessage(atCommand, false);
		serialLogMessage(" get: ", false);
		break;
	case SetFrame:
		serialLogMessage(atCommand, false);
		serialLogMessage(" set: ", false);
		break;
	case ResponseFrame:
		serialLogMessage("Response: ", false);
		break;
	}

	serialLogBuffer(frame, length, true, true);
}

