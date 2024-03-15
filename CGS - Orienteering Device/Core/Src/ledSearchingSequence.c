/*
 * ledSearchingSequence.c
 *
 *  Created on: Mar 15, 2024
 *      Author: tateneedham
 */

#include "ledSearchingSequence.h"
#include "tim.h"
#include "stdbool.h"
#include "leds.h"

int ledScanState = 1;
bool goingUp = true;

const int endTime = 15;
const int shoulderTime = 12;
const int midTime = 10;
const int smallOverlap = 2;
const int bigOverlap = 6;

void doLedSearchingSequence() {
    if (goingUp) {
        ledScanState += 1;
    } else {
        ledScanState -= 1;
    }

    bool led1 =  ledScanState < endTime;

    bool led2 = (ledScanState > endTime - bigOverlap) &&
                (ledScanState < endTime + shoulderTime + smallOverlap);

    bool led3 = (ledScanState > endTime + shoulderTime - smallOverlap) &&
                (ledScanState < endTime + shoulderTime + midTime + smallOverlap);

    bool led4 = (ledScanState > endTime + shoulderTime + midTime - smallOverlap) &&
                (ledScanState < endTime + shoulderTime + midTime * 2 + smallOverlap);

    bool led5 = (ledScanState > endTime + shoulderTime + midTime * 2 - smallOverlap) &&
                (ledScanState < endTime + shoulderTime * 2 + midTime * 2 + bigOverlap);

    bool led6 =  ledScanState > endTime + shoulderTime * 2 + midTime * 2 - bigOverlap;

    BlinkLED(StatusLED, led1 ? ON : OFF);
    BlinkLED(MasterLED, led2 ? ON : OFF);
    BlinkLED(PunchLED,  led3 ? ON : OFF);
    BlinkLED(Rssi1LED,  led4 ? ON : OFF);
    BlinkLED(Rssi2LED,  led5 ? ON : OFF);
    BlinkLED(Rssi3LED,  led6 ? ON : OFF);

    // for debugging...
    // char ledStatus[12] = {0};
    // sprintf(ledStatus, "%d %d %d %d %d %d", (int)led1, (int)led2, (int)led3, (int)led4, (int)led5, (int)led6);
    // serialLogMessage(ledStatus, true);

    if (ledScanState > endTime * 2 + shoulderTime * 2 + midTime * 2) {
        goingUp = false;
    }
    if (ledScanState < 0) {
        goingUp = true;
    }
}
