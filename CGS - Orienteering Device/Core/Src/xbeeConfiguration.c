/*
 * xbeeConfiguration.c
 *
 *  Created on: Mar 6, 2024
 *      Author: tateneedham
 */

#include "xbeeATCommand.h"
#include "string.h"
#include "stdbool.h"
#include "stm32l0xx_hal.h"
#include "xbeeConstants.h"

uint8_t resultBuffer[100] = {0};
int resultLength;
char error[100] = {0};
uint8_t parameter[10] = {0};

#define PANID_Byte1 0x01
#define PANID_Byte2 0x23

// This may not be right or necessary.
// See page 84 of the XBee®-PRO 900HP/XSC RF Modules.pdf
// Bit 0 - Indirect Messaging Coordinator enable. All point-to-multipoint unicasts will be held until requested by a polling end device.
// Bit 1 - Disable routing on this node. When set, this node will not propagate broadcasts or become an intermediate node in a DigiMesh route. This node will not function as a repeater.
// Bit 2 - Indirect Messaging Polling enable. Periodically send requests for messages held by the node’s coordinator.
bool setCoordinator(bool enabled) {
    // Clear result buffer
    memset(resultBuffer, '\0', 100);

    // Get Coordinator Enable
    if (!xbeeATCommandGetValue("CE", resultBuffer, &resultLength, error)) {
        return false;
    }

    // Set Coordinator Enable if required
    if (resultBuffer[0] != (enabled ? 0x02 : 0x00)) {
    	parameter[0] = (enabled ? 0x02 : 0x00);
        if (!xbeeATCommandSetValue("CE", parameter, 1, error)) {
            return false;
        }
    }
    return true;
}

bool setPANID(void) {
    // Clear result buffer
    memset(resultBuffer, '\0', 100);

    // Get PANID
    if (!xbeeATCommandGetValue("ID", resultBuffer, &resultLength, error)) {
        return false;
    }

    // Set PANID if required
    if (!((resultBuffer[0] == PANID_Byte1) && (resultBuffer[1] == PANID_Byte2))) {
    	parameter[0] = PANID_Byte1;
    	parameter[1] = PANID_Byte2;
        if (!xbeeATCommandSetValue("ID", parameter, 2, error)) {
            return false;
        }
    }

    return true;
}

bool setNodeID(char *nodeID) {
    // Clear result buffer
    memset(resultBuffer, '\0', 100);

    // Get Node ID
    if (!xbeeATCommandGetValue("NI", resultBuffer, &resultLength, error)) {
        return false;
    }

    bool same = true;
	for (int i = 0; nodeID[i] != '\0' || i < sizeof(resultBuffer); ++i) {
		if (nodeID[i] != resultBuffer[i]) {
			same = false;
			break;
		}
	}

    // Set Node ID if required
    if (!same) {
    	for(int i = 0; i < strlen(nodeID); i++) {
    		parameter[i] = nodeID[i];
    	}
    	parameter[strlen(nodeID)] = 0;
        if (!xbeeATCommandSetValue("NI", parameter, strlen(nodeID) + 1, error)) {
            return false;
        }
    }

    return true;
}

bool setAPIMode(void) {
    // Clear result buffer
    memset(resultBuffer, '\0', 100);

    // Get AP Mode
    if (!xbeeATCommandGetValue("AP", resultBuffer, &resultLength, error)) {
        return false;
    }

    // Set AP Mode if required
    if (resultBuffer[0] != 1) {
    	parameter[0] = 1;
        if (!xbeeATCommandSetValue("AP", parameter, 1, error)) {
            return false;
        }
    }

    return true;
}

bool setTransmitPowerLevel(void) {
    // Clear result buffer
    memset(resultBuffer, '\0', 100);

    if (!xbeeATCommandGetValue("PL", resultBuffer, &resultLength, error)) {
        return false;
    }

    // Set Power Level if required
    if (resultBuffer[0] != 4) {
    	parameter[0] = 4;
        if (!xbeeATCommandSetValue("PL", parameter, 1, error)) {
            return false;
        }
    }

    return true;
}

bool energyDetect(void) {
    memset(resultBuffer, '\0', 100);

    if (!xbeeATCommandGetValue("ED", resultBuffer, &resultLength, error)) {
        return false;
    }
    return true;
}

bool applyChanges(void) {
    memset(resultBuffer, '\0', 100);
    if (!xbeeATCommandSetValue("AC", parameter, 0, error)) {
        return false;
    }
    return true;
}
/// Configure the local XBee as the Coordinator
/// There must be a coordinator to form a network.
///
/// The following need to be set:
///  1. API mode (not transparent)
///  2. Coordinator Enable (CE) - Disable routing on this node.
///  3. PAN ID (ID) - every node in network needs to have the same ID
///  4. Maximum transmit power
///  5. Node identifier (NI) - nice name for the node
///
///  - Returns: true for success
///
bool xbeeConfigMaster(void) {

	if (!setAPIMode()) { return false; }
    if (!setCoordinator(true)) { return false; }
    if (!setPANID()) { return false; }
    if (!setTransmitPowerLevel()) { return false; }
    if (!setNodeID("MASTER")) { return false; }
    if (!applyChanges()) { return false; }
    // (WR) Write

    energyDetect();


    return true;
}

/// Configure the local XBee as a Slave
///
/// The following need to be set:
///  1. API mode (not transparent)
///  2. Indirect Messaging Coordinator enable (allows routing)
///  3. PAN ID (ID) - every node in network needs to have the same ID
///  4. Maximum transmit power
///  5. Node identifier (NI) - nice name for the node
///
///  - Returns: true for success
///
bool xbeeConfigSlave(void) {

	if (!setAPIMode()) { return false; }
    if (!setCoordinator(false)) { return false; }
    if (!setPANID()) { return false; }
    if (!setTransmitPowerLevel()) { return false; }
    if (!setNodeID("SLAVE")) { return false; }
    if (!applyChanges()) { return false; }
    // (WR) Write

    return true;
}



// AP (API Mode)          : 0 Transparent, 1 API, 1 API escaped
// SM (Sleep Mode)        : 0 disabled
// WR (Write)             : writes values to non-volatile memory. Wait for OK
// FR (Firmware reset)    : reboot. Immediately returns OK then resets
// RE (Restore)           : return to factory defaults
// CE (Coordinator enable): 1 enable
// RR (Retry multiplier)  : 0 - 6, default 0. (3 retries if necessary, multiplier can do more)
// ID (PAN ID)            : read or set PAN. Default 0x3332. 16 bit address
// ED (Energy scan)       : energy scan for each channel
// SC (Scan channels)     : which channels can be used
// PL (Power Level)       : 4 is max
// AC (Apply changes)     : after all properties have been set

// Other notes:
//  - unicast supports retries, broadcast doesn't
