/*
 * xbeeMesh.h
 *
 *  Created on: Mar 13, 2024
 *      Author: tateneedham
 */

#ifndef INC_XBEEMESH_H_
#define INC_XBEEMESH_H_

#include "stm32l0xx_hal.h"
#include "stdbool.h"

extern uint8_t xbeeMasterAddress [8];

bool meshFindMaster();
void heartbeatTimerCallback();
void startHeartbeatTimer();
void stopHeartbeatTimer();
void processHeartbeatResponse();

#endif /* INC_XBEEMESH_H_ */
