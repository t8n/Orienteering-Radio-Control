/*
 * stmSerial.h
 *
 *  Created on: Mar 13, 2024
 *      Author: tateneedham
 */

#ifndef INC_STMSERIAL_H_
#define INC_STMSERIAL_H_

#include "stm32l0xx_hal.h"
#include "stdbool.h"

typedef enum {
  GetFrame,
  SetFrame,
  ResponseFrame
} Log_Frame_Type;

void serialLogInitialise();
void serialLogSendNextMessage();
void serialLogSendAllMessages();
void serialLogClearScreen();
void serialLogMessage(char *message, bool addNewLine);
void serialLogBuffer(uint8_t *buffer, int length, bool convertToHex, bool addNewLine);
void serialLogXBeeFrame(char *atCommand, Log_Frame_Type frameType, uint8_t *frame, int length);

#endif /* INC_STMSERIAL_H_ */
