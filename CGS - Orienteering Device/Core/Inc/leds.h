/*
 * leds.h
 *
 *  Created on: Mar 2, 2024
 *      Author: tateneedham
 */

#ifndef INC_LEDS_H_
#define INC_LEDS_H_

#include "stdint.h"

typedef enum {
  StatusLED,  // D8
  MasterLED,  // D4
  PunchLED,   // D3
  Rssi1LED,   // D7
  Rssi2LED,   // D6
  Rssi3LED    // D5
} LED_Type;

typedef enum {
  LED_ON,
  LED_OFF
} LED_State;

typedef enum {
  LED_SEQUENCE_OFF,
  LED_SEQUENCE_LOOKINGFORXBEE,
  LED_SEQUENCE_CONFIGURING,
  LED_SEQUENCE_SEARCHING,
  LED_SEQUENCE_RUNNING
} LED_Sequence;

uint32_t updateLED(LED_Type led, LED_State state);
uint32_t ToggleLED(LED_Type led);
void BlockingErrorAlert(int flashCount);

void configLEDFlashTimer();
void startLEDSequence(LED_Sequence sequence);
void stopLEDSequence();
void ledTimerCallback();

#endif /* INC_LEDS_H_ */
