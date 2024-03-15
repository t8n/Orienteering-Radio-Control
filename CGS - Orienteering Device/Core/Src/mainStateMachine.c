/*
 * mainStateMachine.c
 *
 *  Created on: Mar 14, 2024
 *      Author: tateneedham
 */

#include "mainStateMachine.h"
#include "stdio.h"
#include "stdlib.h"
#include "main.h"
#include "leds.h"
#include "stmSerial.h"
#include "masterSlaveMode.h"
#include "beep.h"
#include "buffers.h"
#include "string.h"
#include "usart.h"
#include "xbeeMesh.h"
#include "xbeeConstants.h"
#include "xbeeConfiguration.h"
#include "xbeeChecksum.h"
#include "srrConstants.h"
#include "slaveLoop.h"
#include "masterLoop.h"

void toggleStatusLED(int blinkRate);
void findXBeeMaster();
void resetXBeeIfRequired();
void masterModeLoop(void);

uint32_t xbeeReinitTimeout = 0;

Machine_State machineState = LookingForXBee;

void stateMachineLoop() {
    switch (machineState) {

	case LookingForXBee:
	    startLEDSequence(LED_SEQUENCE_LOOKINGFORXBEE);
		resetXBeeIfRequired();
		break;

	case ConfigureXBee:
        startLEDSequence(LED_SEQUENCE_CONFIGURING);
		if (masterSlaveMode == Master) {
			serialLogMessage("Configuring XBee as Master...", true);
			if (xbeeConfigMaster()) {
				serialLogMessage("XBee configured as Master", true);
				serialLogMessage("", true);
				machineState = MasterLoop;
			} else {
				machineState = LookingForXBee;
			}
		} else {
			serialLogMessage("Configuring XBee as Slave...", true);
			if (xbeeConfigSlave()) {
				serialLogMessage("XBee configured as Slave", true);
				machineState = FindMaster;
			} else {
				machineState = LookingForXBee;
			}
			// startBeep();
		}
		break;

	case FindMaster:
	    startLEDSequence(LED_SEQUENCE_SEARCHING);
		findXBeeMaster();
		break;

	case MasterLoop:
	    startLEDSequence(LED_SEQUENCE_RUNNING);
		masterLoop();
		break;

	case SlaveLoop:
        startLEDSequence(LED_SEQUENCE_RUNNING);
		slaveLoop();
		break;

	}
}



void findXBeeMaster(void) {
	serialLogMessage("", true);
	serialLogMessage("Searching for Master...", true);

	if(meshFindMaster()) {
		serialLogMessage("Master found, address: ", false);
		serialLogBuffer(xbeeMasterAddress, sizeof(xbeeMasterAddress), true, true);
		machineState = SlaveLoop;
		serialLogMessage("", true);
		serialLogMessage("Slave loop. Listening for SRR punches...", true);
	} else {
		// A failed search takes a while, so do nothing and let the main loop try again.
	}
}

// System reset if the XBee is not found within 500ms
void resetXBeeIfRequired(void) {
	if (HAL_GetTick() - xbeeReinitTimeout > 500) {
		serialLogMessage("", true);
		serialLogMessage("XBee not found, resetting...", false);
		xbeeReinitTimeout = HAL_GetTick();
		HAL_NVIC_SystemReset();
	}
}

//void CheckForXBeeTimeout(void)
//{
//	/* If it's taken 100ms to get through the process of receiving a whole Xbee packet, it must have failed */
//	/* So, we reset the steps */
//	if ((HAL_GetTick() - xbeeTimeout) > 100)
//	{
//		xbeeStep = 1;
//		HAL_UART_Receive_IT(&huart1, &xbeeBuffer[1], 1);
//	}
//}

void xbeeFound() {
	updateLED(Rssi1LED, LED_OFF);
	updateLED(Rssi2LED, LED_OFF);
	updateLED(Rssi3LED, LED_OFF);
	endBeep();
	serialLogMessage("FOUND", true);
	serialLogMessage("", true);

	machineState = ConfigureXBee;
}

void xbeeNotFound() {
	updateLED(Rssi1LED, LED_ON);
	updateLED(Rssi2LED, LED_ON);
	updateLED(Rssi3LED, LED_ON);
	startBeep();

	// Main loop does a retry after a timeout
	xbeeReinitTimeout = HAL_GetTick();
}
