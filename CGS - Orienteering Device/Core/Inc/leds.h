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

typedef enum
{
  ON,
  OFF
} LED_State;

uint32_t BlinkLED(LED_Type led, LED_State state);
uint32_t ToggleLED(LED_Type led);


#endif /* INC_LEDS_H_ */
