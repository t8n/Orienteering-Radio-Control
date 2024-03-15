/*
 * leds.c
 *
 *  Created on: Mar 2, 2024
 *      Author: tateneedham
 */

#include "main.h"
#include "leds.h"
#include "beep.h"
#include "tim.h"
#include "stmSerial.h"
#include "stdio.h"
#include "ledSearchingSequence.h"

LED_Sequence ledSequence = LED_SEQUENCE_OFF;

void toggleLED(LED_Type led, int period);

GPIO_TypeDef* GPIOPortForLEDPin(LED_Type led)
{
	switch (led) {
	case StatusLED: return STATUS_LED_GPIO_Port;
	case MasterLED: return MASTER_LED_GPIO_Port;  // NOT WORKING
	case PunchLED:  return TX_LED_GPIO_Port;      // NOT WORKING
	case Rssi1LED:  return RSSI_1_LED_GPIO_Port;
	case Rssi2LED:  return RSSI_2_LED_GPIO_Port;
	case Rssi3LED:  return RSSI_3_LED_GPIO_Port;
	}
	// just to stop the warning...
	return (GPIO_TypeDef*)STATUS_LED_GPIO_Port;
}

uint16_t PinForLED(LED_Type led)
{
	switch (led) {
	case StatusLED: return STATUS_LED_Pin;
	case MasterLED: return MASTER_LED_Pin;  // NOT WORKING
	case PunchLED:  return TX_LED_Pin;      // NOT WORKING
	case Rssi1LED:  return RSSI_1_LED_Pin;
	case Rssi2LED:  return RSSI_2_LED_Pin;
	case Rssi3LED:  return RSSI_3_LED_Pin;
	}
	// just to stop the warning...
	return STATUS_LED_Pin;
}

uint32_t updateLED(LED_Type led, LED_State state)
{
	HAL_GPIO_WritePin(GPIOPortForLEDPin(led), PinForLED(led), state == LED_ON ? GPIO_PIN_SET : GPIO_PIN_RESET);
	return HAL_GetTick();
}

uint32_t ToggleLED(LED_Type led)
{
	HAL_GPIO_TogglePin(GPIOPortForLEDPin(led), PinForLED(led));
	return HAL_GetTick();
}

void BlockingErrorAlert(int flashCount)
{
	startBeep();
	for(int i = 0; i < flashCount; i++) {
		updateLED(Rssi1LED, LED_ON);
		updateLED(Rssi2LED, LED_ON);
		updateLED(Rssi3LED, LED_ON);
		HAL_Delay(200);
		updateLED(Rssi1LED, LED_OFF);
		updateLED(Rssi2LED, LED_OFF);
		updateLED(Rssi3LED, LED_OFF);
		HAL_Delay(200);
	}
	endBeep();
}

// The searching LED sequence scans the leds up and down to provide a visually obvious indicator that we're searching
void startLEDSequence(LED_Sequence sequence) {
    if (ledSequence == sequence) {
        return;
    }

    stopLEDSequence();
    if (HAL_TIM_Base_GetState(&htim6) == HAL_TIM_STATE_READY) {
        HAL_TIM_Base_Start_IT(&htim6);
        ledSequence = sequence;
    }
}

void stopLEDSequence() {
    HAL_TIM_Base_Stop_IT(&htim6);

    switch (ledSequence) {
    case LED_SEQUENCE_OFF:
        break;
    case LED_SEQUENCE_LOOKINGFORXBEE:
        updateLED(StatusLED, LED_OFF);
        break;
    case LED_SEQUENCE_CONFIGURING:
        updateLED(StatusLED, LED_OFF);
        break;
    case LED_SEQUENCE_SEARCHING:
        ledSearchingSequenceReset();
        break;
    case LED_SEQUENCE_RUNNING:
        updateLED(StatusLED, LED_OFF);
        break;
    }

    ledSequence = LED_SEQUENCE_OFF;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM6) {
	    switch (ledSequence) {
	    case LED_SEQUENCE_OFF:
	        break;
	    case LED_SEQUENCE_LOOKINGFORXBEE:
	        toggleLED(MasterLED, 10);
	        break;
	    case LED_SEQUENCE_CONFIGURING:
            toggleLED(StatusLED, 10);
	        break;
	    case LED_SEQUENCE_SEARCHING:
	        doLedSearchingSequence();
	        break;
	    case LED_SEQUENCE_RUNNING:
            toggleLED(StatusLED, 60);
	        break;
	    }
	}
}

int blinkCount = 0;
void toggleLED(LED_Type led, int period) {
    blinkCount++;
    if (blinkCount > period) {
        blinkCount = 0;
        ToggleLED(led);
    }
}
