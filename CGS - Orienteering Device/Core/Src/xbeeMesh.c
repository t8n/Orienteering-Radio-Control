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

uint8_t xbeeMasterAddress [8] = {0};

uint8_t meshResultBuffer[100] = {0};
int meshResultLength;
char meshError[100] = {0};

bool meshFindMaster() {

	memset(meshResultBuffer, '\0', 100);

	if (!xbeeSendATCommand("DN", true, (uint8_t *)RADIO_NAME_MASTER, strlen((char *)RADIO_NAME_MASTER), meshResultBuffer, &meshResultLength, meshError)) {
		serialLogMessage("DN error: ", false);
		serialLogMessage(meshError, true);
		return false;
	}

	if (meshResultLength != 10) {
		char message[100] = {0};
		sprintf(message, "DN error: Invalid response length: %i, expected 10", meshResultLength);
		serialLogMessage("", true);
		serialLogMessage(message, true);
		return false;
	}

	// The 64 bit address of the Master is the 3rd - 10th bytes of the response
    memcpy(xbeeMasterAddress, &meshResultBuffer[2], 8);

    return true;
}
