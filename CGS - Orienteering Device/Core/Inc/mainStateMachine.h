/*
 * mainStateMachine.h
 *
 *  Created on: Mar 14, 2024
 *      Author: tateneedham
 */

#ifndef INC_MAINSTATEMACHINE_H_
#define INC_MAINSTATEMACHINE_H_

typedef enum {
  LookingForXBee,
  ConfigureXBee,
  FindMaster,
  MasterLoop,
  SlaveLoop
} Machine_State;

void stateMachineLoop();
void xbeeFound();
void xbeeNotFound();

extern Machine_State machineState;

#endif /* INC_MAINSTATEMACHINE_H_ */
