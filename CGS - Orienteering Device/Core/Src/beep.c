/*
 * beep.c
 *
 *  Created on: Mar 2, 2024
 *      Author: tateneedham
 */

#include "beep.h"
#include "tim.h"

void startBeep() {
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
}

void endBeep() {
	HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2);
}
