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
#include "fifoQueue.h"
#include <stdlib.h>
#include <stdint.h>

struct FifoQueue serialLogQueue;

void serialLogInitialise() {
    queueInit(&serialLogQueue);
}

void serialLogSendNextMessage() {
    if (queueIsEmpty(&serialLogQueue)) {
        return;
    }
    struct QueueData data = queuePop(&serialLogQueue);
    HAL_UART_Transmit(&huart5, data.data, data.size, 1000);
    free(data.data);
}

void serialLogSendAllMessages() {
    while (!queueIsEmpty(&serialLogQueue)) {
        serialLogSendNextMessage();
    }
}

void serialLogClearScreen() {
    struct QueueData data = {.data = (uint8_t *)"\033[2J\n\r", .size = 6};
    queuePush(&serialLogQueue, data);
}

void serialLogMessage(char *message, bool addNewLine) {
	serialLogBuffer((uint8_t *)message, strlen(message), false, addNewLine);
}

void serialLogBuffer(uint8_t *buffer, int length, bool convertToHex, bool addNewLine) {
    if (length > 0) {
        if (convertToHex) {
            char hexString[length * 2 + 1]; // two hex characters per byte plus null terminator
            for (int i = 0; i < length; i++) {
                sprintf(&hexString[i * 2], "%02x", buffer[i]);
            }
            struct QueueData data = {.data = (uint8_t *)hexString, .size = strlen(hexString)};
            queuePush(&serialLogQueue, data);
        } else {
            struct QueueData data = {.data = buffer, .size = length};
            queuePush(&serialLogQueue, data);
        }
    }

	if (addNewLine) {
        struct QueueData data = {.data = (uint8_t *)"\n\r", .size = 2};
        queuePush(&serialLogQueue, data);
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

