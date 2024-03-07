/*
 * xbeeChecksum.h
 *
 *  Created on: Mar 6, 2024
 *      Author: tateneedham
 */

#ifndef INC_XBEECHECKSUM_H_
#define INC_XBEECHECKSUM_H_

#include "stdbool.h"
#include "stm32l0xx_hal.h"

uint8_t xbeeChecksum(uint8_t *buffer, uint16_t length);

#endif /* INC_XBEECHECKSUM_H_ */
