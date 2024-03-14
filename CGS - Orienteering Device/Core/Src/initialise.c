/*
 * initialise.c
 *
 *  Created on: Mar 14, 2024
 *      Author: tateneedham
 */

#include "masterSlaveMode.h"
#include "main.h"
#include "stdbool.h"
#include "xbeeConstants.h"
#include "buffers.h"
#include "usart.h"
#include "stmSerial.h"

void initialiseHardware() {
	serialLogClearScreen();
	serialLogMessage("Hardware initialising...", true);

	// Read the switch to see if we are a master or slave
	GPIO_PinState pinState = HAL_GPIO_ReadPin(GPIOC, SWITCH_Pin);
	serialLogMessage("Mode: ", false);
	if (pinState == GPIO_PIN_SET) {
		masterSlaveMode = Master;
		serialLogMessage("Master", true);
	} else {
		masterSlaveMode = Slave;
		serialLogMessage("Slave", true);
	}

	if (masterSlaveMode == Slave) {
		// Enable 3 radio inputs
		HAL_UART_Receive_IT(&huart4, &radioRedIn, 1);
		HAL_UART_Receive_IT(&huart2, &radioBlueIn, 1);
		HAL_UART_Receive_IT(&huart5, &radioAuxIn, 1);
	} else if (masterSlaveMode == Master) {
		// Enable PC communications
		HAL_UART_Receive_IT(&huart5, &PCIn, 1);
	}

	serialLogMessage("Hardware initialised.", true);
	serialLogMessage("", true);

	// Wake up the XBee
	serialLogMessage("Looking for XBee...", false);
	HAL_GPIO_WritePin(GPIOA, XBEE_RESETn_Pin, GPIO_PIN_SET);
	// listen for the XBee reset sequence
	HAL_UART_Receive_IT(&huart1, (uint8_t *)xbeeRxBuffer, sizeof(XBEE_RESET_SEQUENCE) - 1);
}
