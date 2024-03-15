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

LED_Sequence ledSequence = OFF;

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

uint32_t BlinkLED(LED_Type led, LED_State state)
{
	HAL_GPIO_WritePin(GPIOPortForLEDPin(led), PinForLED(led), state == ON ? GPIO_PIN_SET : GPIO_PIN_RESET);
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
		BlinkLED(Rssi1LED, ON);
		BlinkLED(Rssi2LED, ON);
		BlinkLED(Rssi3LED, ON);
		HAL_Delay(200);
		BlinkLED(Rssi1LED, OFF);
		BlinkLED(Rssi2LED, OFF);
		BlinkLED(Rssi3LED, OFF);
		HAL_Delay(200);
	}
	endBeep();
}

// The searching LED sequence scans the leds up and down to provide a visually obvious indicator that we're searching
void startSearchingLEDSequence() {
    if (HAL_TIM_Base_GetState(&htim6) == HAL_TIM_STATE_READY) {
        HAL_TIM_Base_Start_IT(&htim6);
        ledSequence = SEARCHING;
    }
}

void stopSearchingLEDSequence() {
    HAL_TIM_Base_Stop_IT(&htim6);
    ledSequence = LED_SEQUENCE_OFF;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM6) {
	    switch (ledSequence) {
	    case LED_SEQUENCE_OFF:
	        break;
	    case SEARCHING:
	        doLedSearchingSequence();
	        break;
	    }
	}
}
