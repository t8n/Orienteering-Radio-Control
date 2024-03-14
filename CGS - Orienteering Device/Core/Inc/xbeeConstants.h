/*
 * xbeeConstants.h
 *
 *  Created on: Mar 6, 2024
 *      Author: tateneedham
 */

#ifndef INC_XBEECONSTANTS_H_
#define INC_XBEECONSTANTS_H_

#define SOF						 0x7E
#define CMD_TX_REQUEST			 0x10

#define XBEE_AT_COMMAND          0x08
#define XBEE_AT_COMMAND_RESPONSE 0x88
#define XBEE_AT_SUCCESS          0x00
#define XBEE_AT_STATUS_OK        0x40

#define RADIO_NAME_MASTER        "MASTER"
#define RADIO_NAME_SLAVE         "SLAVE"

#define XBEE_MAX_PACKET_SIZE	 60

#define XBEE_RESET_SEQUENCE      "\x7e\x00\x02\x8a\x00\x75"

#endif /* INC_XBEECONSTANTS_H_ */
