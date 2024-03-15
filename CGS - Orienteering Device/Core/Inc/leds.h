/*
 * leds.h
 *
 *  Created on: Mar 2, 2024
 *      Author: tateneedham
 */

#ifndef INC_LEDS_H_
#define INC_LEDS_H_

typedef enum {
  StatusLED,  // D8
  MasterLED,  // D4
  PunchLED,   // D3
  Rssi1LED,   // D7
  Rssi2LED,   // D6
  Rssi3LED    // D5
} LED_Type;

typedef enum {
  ON,
  OFF
} LED_State;

typedef enum {
  LED_SEQUENCE_OFF,
  SEARCHING
} LED_Sequence;

uint32_t BlinkLED(LED_Type led, LED_State state);
uint32_t ToggleLED(LED_Type led);
void BlockingErrorAlert(int flashCount);

void configLEDFlashTimer();
void startSearchingLEDSequence();
void stopSearchingLEDSequence();

#endif /* INC_LEDS_H_ */
