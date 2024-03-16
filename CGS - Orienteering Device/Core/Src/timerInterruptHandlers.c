/*
 * timerInterruptHandlers.c
 *
 *  Created on: Mar 15, 2024
 *      Author: tateneedham
 */

#include "leds.h"
#include "xbeeMesh.h"
#include "tim.h"
#include "stdio.h"
#include "stdlib.h"
#include "stm32l0xx_hal_tim.h"

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM6) {
        ledTimerCallback();
    } else if (htim->Instance == TIM7) {
        heartbeatTimerCallback();
    }
}
