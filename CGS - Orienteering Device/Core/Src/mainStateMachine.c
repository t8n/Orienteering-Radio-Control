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
uint32_t timeSinceLastPowerLEDBlink = 0;

Machine_State machineState = LookingForXBee;

void stateMachineLoop() {
	switch (machineState) {

	case LookingForXBee:
		toggleStatusLED(100);
		resetXBeeIfRequired();
		break;

	case ConfigureXBee:
		BlinkLED(StatusLED, ON);
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
		findXBeeMaster();
		break;

	case MasterLoop:
		toggleStatusLED(500);
		masterLoop();
		break;

	case SlaveLoop:
		toggleStatusLED(500);
		slaveLoop();
		break;

	}
}



void findXBeeMaster(void) {
	serialLogMessage("", true);
	serialLogMessage("Searching for Master...", true);
	startSearchingLEDSequence();

	if(meshFindMaster()) {
	    stopSearchingLEDSequence();
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

void toggleStatusLED(int blinkRate) {
	if (HAL_GetTick() - timeSinceLastPowerLEDBlink > blinkRate)	{
		timeSinceLastPowerLEDBlink = ToggleLED(StatusLED);
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
	BlinkLED(Rssi1LED, OFF);
	BlinkLED(Rssi2LED, OFF);
	BlinkLED(Rssi3LED, OFF);
	endBeep();
	serialLogMessage("FOUND", true);
	serialLogMessage("", true);

	machineState = ConfigureXBee;
}

void xbeeNotFound() {
	BlinkLED(Rssi1LED, ON);
	BlinkLED(Rssi2LED, ON);
	BlinkLED(Rssi3LED, ON);
	startBeep();

	// Main loop does a retry after a timeout
	xbeeReinitTimeout = HAL_GetTick();
}
