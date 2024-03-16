/*
 * xbeeMesh.c
 *
 *  Created on: Mar 13, 2024
 *      Author: tateneedham
 */

#include "xbeeATCommand.h"
#include "string.h"
#include "stdbool.h"
#include "stm32l0xx_hal.h"
#include "xbeeConstants.h"
#include "stmSerial.h"
#include "stdio.h"
#include "tim.h"
#include "xbeeHeartbeat.h"
#include "xbeeMesh.h"
#include "mainStateMachine.h"
#include "buffers.h"
#include "uartInterruptHandlers.h"

uint8_t xbeeMasterAddress [8] = {0};

uint8_t meshResultBuffer[100] = {0};
int meshResultLength;
char meshError[100] = {0};
int heartbeatTimerCount = 0;

bool meshFindMaster() {

	memset(meshResultBuffer, '\0', 100);

	if (!xbeeSendATCommand_blocking(XBEE_AT_DISCOVER_NODE, true, (uint8_t *)RADIO_NAME_MASTER, strlen((char *)RADIO_NAME_MASTER), meshResultBuffer, &meshResultLength, meshError)) {
		serialLogMessage("Discover Node error: ", false);
		serialLogMessage(meshError, true);
		return false;
	}

	if (meshResultLength != 10) {
		char message[100] = {0};
		sprintf(message, "Discover Node error: Invalid response length: %i, expected 10", meshResultLength);
		serialLogMessage("", true);
		serialLogMessage(message, true);
		return false;
	}

	// The 64 bit address of the Master is the 3rd - 10th bytes of the response
    memcpy(xbeeMasterAddress, &meshResultBuffer[2], 8);

    return true;
}

void processHeartbeatResponse() {
    if (xbeeRxBuffer[XBEE_AT_PREAMBLELENGTH + XBEE_AT_COMMANDLENGTH] != XBEE_AT_STATUS_OK) {
        char error[100] = {0};
        sprintf(error, "Heartbeat returned false: 0x%x", xbeeRxBuffer[XBEE_AT_PREAMBLELENGTH + XBEE_AT_COMMANDLENGTH]);
        serialLogMessage(error, true);
        machineState = FindMaster;
        return;
    }

    int addressStartByte = XBEE_AT_PREAMBLELENGTH + XBEE_AT_COMMANDLENGTH + 1 + 2; // 1 = result byte, 2 = 0xFFFE
    memcpy(xbeeMasterAddress, (uint8_t *)&xbeeRxBuffer[addressStartByte], 8);
    serialLogMessage("Heartbeat success.", true);
    startHeartbeatTimer();
}

void startHeartbeatTimer() {
    if (HAL_TIM_Base_GetState(&htim7) == HAL_TIM_STATE_READY) {
        HAL_TIM_Base_Start_IT(&htim7);
        heartbeatTimerCount = 0;
    }
}

void stopHeartbeatTimer() {
    if (HAL_TIM_Base_GetState(&htim7) == HAL_TIM_STATE_BUSY) {
        HAL_TIM_Base_Stop_IT(&htim7);
    }
}

// This is fired once per second
void heartbeatTimerCallback() {
    heartbeatTimerCount += 1;

    // do a heartbeat every couple of minutes or so? (for testing: every 10 seconds)
    if (heartbeatTimerCount > 10) {
        stopHeartbeatTimer();
        bool success = xbeeSendATCommand(XBEE_AT_DISCOVER_NODE, (uint8_t *)RADIO_NAME_MASTER, strlen((char *)RADIO_NAME_MASTER), meshError);
        if (!success) {
            serialLogMessage("Failed to send heartbeat command.", true);
            startHeartbeatTimer();
        }
    }
}
