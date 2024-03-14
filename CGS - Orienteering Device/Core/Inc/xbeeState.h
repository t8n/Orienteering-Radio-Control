/*
 * xbeeState.h
 *
 *  Created on: Mar 6, 2024
 *      Author: tateneedham
 */

#ifndef INC_XBEESTATE_H_
#define INC_XBEESTATE_H_

typedef enum {
    Idle,
    ATCommand
} XBee_State;

extern XBee_State xbeeState;

#endif /* INC_XBEESTATE_H_ */
