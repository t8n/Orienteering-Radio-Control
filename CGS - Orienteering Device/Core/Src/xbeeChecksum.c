/*
 * xbeeChecksum.c
 *
 *  Created on: Mar 6, 2024
 *      Author: tateneedham
 */

#include "xbeeChecksum.h"

uint8_t xbeeChecksum(uint8_t *buffer, uint16_t length) {
	uint16_t add = 0;
	uint16_t i;
	uint8_t checksum;

	for (i = 0; i < length; ++i) {
		add += buffer[i];
	}

	// Keep only the lowest 8 bits of the result
	add = add & 0x00FF;

	checksum = 0xFF - (uint8_t)add;

	return checksum;
}
