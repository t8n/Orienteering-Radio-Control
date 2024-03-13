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

uint8_t meshResultBuffer[100] = {0};
int meshResultLength;
char meshError[100] = {0};
//uint8_t meshParameter[10] = {0};

bool meshFindMaster(uint8_t *masterAddress) {

	memset(meshResultBuffer, '\0', 100);

	if (!xbeeSendATCommand("DN", true, (uint8_t *)MASTER_RADIO_NAME, strlen((char *)MASTER_RADIO_NAME), meshResultBuffer, &meshResultLength, meshError)) {
		serialLogMessage("", true);
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
    memcpy(masterAddress, &meshResultBuffer[2], 8);

    return true;
}
