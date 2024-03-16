/*
 * xbeeATCommand.h
 *
 *  Created on: Mar 6, 2024
 *      Author: tateneedham
 */

#ifndef INC_XBEEATCOMMAND_H_
#define INC_XBEEATCOMMAND_H_

#include "stm32l0xx_hal.h"
#include "stdbool.h"

bool xbeeSendATCommand(char *atCommand, uint8_t *parameterValue, int parameterLength, char *error);
bool xbeeATCommandGetValue_blocking(char *atCommand, uint8_t *result, int *resultLength, char *error);
bool xbeeATCommandSetValue_blocking(char *atCommand, uint8_t *parameterValue, int parameterLength, char *error);
bool xbeeSendATCommand_blocking(char *atCommand, bool isGet, uint8_t *parameterValue, int parameterLength, uint8_t *result, int *resultLength, char *error);

#endif /* INC_XBEEATCOMMAND_H_ */
