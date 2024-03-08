/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

#include "main.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"
#include "leds.h"
#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"
#include "string.h"
#include "beep.h"
#include "xbeeChecksum.h"
#include "xbeeConstants.h"
#include "xbeeConfiguration.h"

/* Orienteering Device definitions */
#define SLAVE		0
#define MASTER		1
#define OD_ID		0x43

/* SRR definitions */
#define ETX			0x03
#define NAK			0x15

/* Tracker variable for which mode we are in */
bool mode;

/* Incoming channel buffers */
volatile uint8_t radioRedBuffer  	[100];
volatile uint8_t radioBlueBuffer 	[100];
volatile uint8_t radioAuxBuffer		[100];

volatile uint8_t PCBuffer  			[100];
volatile uint8_t xbeeRxBuffer 		[100];

uint8_t xbeeTXBuffer	[XBEE_MAX_PACKET_SIZE];
unsigned char resetSequence[] = {SOF, 0x00, 0x02, 0x8A, 0x00, 0x75};

/* Incoming single byte (we only handle 1 byte at a time) */
uint8_t radioRedIn = 0;
uint8_t radioBlueIn = 0;
uint8_t radioAuxIn = 0;

uint8_t PCIn = 0;
uint8_t xbeeIn = 0;

/* Tracker to keep track of where in the buffer we are */
uint8_t radioRedTracker = 0;
uint8_t radioBlueTracker = 0;
uint8_t radioAuxTracker = 0;

uint8_t PCTracker = 0;
uint8_t xbeeTracker = 0;

/* Boolean flag for when a packet is complete and ready to be interpreted */
bool radioRedPacketComplete = false;
bool radioBluePacketComplete = false;
bool radioAuxPacketComplete = false;

bool PCPacketComplete = false;
bool xbeePacketComplete = false;
uint32_t xbeeReinitTimeout = 0;

/* Timeout counters */
uint32_t xbeeTimeout = 0;

/* Variable for which stage of the packet we are at */
volatile uint8_t xbeeStep = 1;
volatile uint16_t xbeeSize = 0;

/* Outgoing buffer */
uint8_t transmitBuffer	[100];

/* Tracker variable for last time LEDs blinked */
uint32_t timeSinceLastPowerLEDBlink = 0;
uint32_t timeSinceLastPunchLEDBlink = 0;

void SystemClock_Config(void);
static void Boot_Sequence(bool mode);
bool XBee_Transmit(uint8_t* txBuffer, uint8_t txBufferSize);
void BlinkAndBeepForPunch(void);
void ResetBlinkAndBeepForPunch(void);
void SlaveModeLoop(void);
void MasterModeLoop(void);
void ToggleStatusLED(uint16_t);
void CheckForXBeeTimeout(void);
void ResetXBeeIfRequired(void);
void InitialiseHardware();

typedef enum {
  LookingForXBee,
  ConfigureXBee,
  MasterLoop,
  SlaveLoop
} Machine_State;

Machine_State machineState = ConfigureXBee;

int main(void)
{
	InitialiseHardware();

	while (true) {
		switch (machineState) {
		case LookingForXBee:
			ToggleStatusLED(100);
			ResetXBeeIfRequired();
			break;
		case ConfigureXBee:
			BlinkLED(StatusLED, ON);
			if (mode == MASTER) {
				if (xbeeConfigMaster()) {
					machineState = MasterLoop;
				} else {
					machineState = LookingForXBee;
				}
			} else {
				if (xbeeConfigSlave()) {
					machineState = SlaveLoop;
				} else {
					machineState = LookingForXBee;
				}
				StartBeep();
			}
			break;
		case MasterLoop:
			ToggleStatusLED(500);
			MasterModeLoop();
			break;
		case SlaveLoop:
			ToggleStatusLED(500);
			SlaveModeLoop();
			break;
		}
	}
}

void ResetXBeeIfRequired(void) {
	if (HAL_GetTick() - xbeeReinitTimeout > 500) {
		xbeeReinitTimeout = HAL_GetTick();
		HAL_NVIC_SystemReset();
	}
}

void BlinkAndBeepForPunch(void)
{
	timeSinceLastPunchLEDBlink = BlinkLED(Rssi1LED, ON);  // PunchLED is not working on 1 board, so use RSSI1 for now
	StartBeep();
}

void ResetBlinkAndBeepForPunch(void)
{
	if (HAL_GetTick() - timeSinceLastPunchLEDBlink > 300)
	{
		BlinkLED(Rssi1LED, OFF);  // PunchLED is not working on 1 board, so use RSSI1 for now
		EndBeep();
	}
}

void ToggleStatusLED(uint16_t blinkRate)
{
	if (HAL_GetTick() - timeSinceLastPowerLEDBlink > blinkRate)
	{
		timeSinceLastPowerLEDBlink = ToggleLED(StatusLED);
	}
}

void SlaveModeLoop(void)
{
	bool success = false;
	/* Handle the slave mode */
	/* Here, 4 things could happen:	 */
	/* - A packet from the red radio comes in. We send this out via the XBee */
	/* - A packet from the blue radio comes in. We send this out via the XBee */
	/* - A packet from the aux radio comes in. We send this out via the XBee */
	/* - A packet from the XBee comes in. We handle this command */

	if (radioRedPacketComplete == true)
	{
		success = XBee_Transmit(radioRedBuffer, radioRedTracker);

		memset(radioRedBuffer, 0, 100);
		radioRedTracker = 0;

		radioRedPacketComplete = false;
	}

	if (radioBluePacketComplete == true)
	{
		success = XBee_Transmit(radioBlueBuffer, radioBlueTracker);

		memset(radioBlueBuffer, 0, 100);
		radioBlueTracker = 0;

		radioBluePacketComplete = false;
	}

	if (radioAuxPacketComplete == true)
	{
		success = XBee_Transmit(radioAuxBuffer, radioAuxTracker);

		memset(radioAuxBuffer, 0, 100);
		radioAuxTracker = 0;

		radioAuxPacketComplete = false;
	}

	if (success) {
		BlinkAndBeepForPunch();
	}

	ResetBlinkAndBeepForPunch();
}

void MasterModeLoop(void)
{
	/* Handle the master mode */
	/* Here, 3 things could happen:	 */
	/* - A packet from the Xbee comes in. We handle this data */
	/* - A packet from the PC comes in. We handle this command (TODO) */
	/* - The heartbeat button is pushed. We send out a signal to make the slaves blink (TODO) */

	if (xbeePacketComplete == true)
	{

		/* We need to find where the packet starts. Sometimes this is 14, 15, 16 etc. indexes into the buffer */
		/* There's also a chance both the XBee AND the radio packet are atypical sizes, and with both having their own overhead/structure */
		/* You can see, it's actually proving quite hard to work out where one starts and the other ends because we have embedded radio packets */
		/* We also can't just look for the radio EOF indicator (0x03) since the XBee packet might have this inside of it, too! */
		/* Temporary solution is to look for what seems to be a unique key at the start of the punch radio message: */
		/* - 0xFF (alert, packet incoming) */
		/* - 0x02 (STX, start of text) */
		/* - 0xD3 (transmit punch data) */
		/* This is taken from Page 5 of the SportIdent PC programmers guide */
		/* We can then use the next byte to work out the length. This is similar to the XBee radio structure but is more appropriate to use here */
		/* If this exact key isn't received, then we can discard it and it must be noise or some message we aren't interested in */
		/* This is STILL flawed though! THere's a chance the XBee might have that exact ^ structure embedded in its header */
		uint8_t index;
		uint8_t radioPacketLength;

		for (index = 0; index <= 100; index++)
		{

			/* See if this matches the start of a valid punch data radio packet */
			if (xbeeRxBuffer[index] == 0xFF && xbeeRxBuffer[index + 1] == 0x02 && xbeeRxBuffer[index + 2] == 0xD3)
			{

				/* Grab the length - this is the next byte, but also add back in the overhead (4 bytes) and checksum (3 bytes) */
				/* This means rather than the interesting data, we are transmitting exactly as if the SRR was plugged into the PC */
				/* This can be easily removed later, if we don't want to worry about the checksum and overhead and just want the pure punch data */
				radioPacketLength = xbeeRxBuffer[index + 3] + 4 + 3;

				memcpy(transmitBuffer, &xbeeRxBuffer[index], radioPacketLength);
				HAL_UART_Transmit(&huart5, transmitBuffer, 20, 100);
				break;
			}

		}

		/* If the for loop ends normally, then nothing was transmitted and the message was discarded */
		memset(xbeeRxBuffer, 0, 100);
		xbeePacketComplete = false;

		HAL_UART_Receive_IT(&huart1, xbeeRxBuffer, 1);

	}

	if (PCPacketComplete == true)
	{
		/* Do something with the PC Packet */

		memset(PCBuffer, 0, 100);
		PCTracker = 0;

		PCPacketComplete = false;
	}

}

//void CheckForXBeeTimeout(void)
//{
//	/* If it's taken 100ms to get through the process of receiving a whole Xbee packet, it must have failed */
//	/* So, we reset the steps */
//	if ((HAL_GetTick() - xbeeTimeout) > 100)
//	{
//		xbeeStep = 1;
//		HAL_UART_Receive_IT(&huart1, &xbeeBuffer[1], 1);
//	}
//}

static void Boot_Sequence(bool mode)
{

	if (mode == SLAVE)
	{
		/* Turn LEDS ON and OFF and make buzzer beep once and longer */

		StartBeep();

		BlinkLED(StatusLED, ON);
		HAL_Delay(200);
		BlinkLED(StatusLED, OFF);

		BlinkLED(MasterLED, ON);
		HAL_Delay(200);
		EndBeep();
		BlinkLED(MasterLED, OFF);

		BlinkLED(PunchLED, ON);
		HAL_Delay(200);
		BlinkLED(PunchLED, OFF);

		BlinkLED(Rssi1LED, ON);
		HAL_Delay(200);
		BlinkLED(Rssi1LED, OFF);

		BlinkLED(Rssi2LED, ON);
		HAL_Delay(200);
		BlinkLED(Rssi2LED, OFF);

		BlinkLED(Rssi3LED, ON);
		HAL_Delay(200);
		BlinkLED(Rssi3LED, OFF);

	}
	else if (mode == MASTER)
	{

		/* Turn LEDS ON and OFF and make buzzer beep twice */
		StartBeep();
		BlinkLED(StatusLED, ON);
		HAL_Delay(100);
		EndBeep();

		HAL_Delay(100);
		BlinkLED(StatusLED, OFF);

		BlinkLED(MasterLED, ON);
		StartBeep();
		HAL_Delay(100);
		EndBeep();

		HAL_Delay(100);
		BlinkLED(MasterLED, OFF);

		BlinkLED(PunchLED, ON);
		HAL_Delay(200);
		BlinkLED(PunchLED, OFF);

		BlinkLED(Rssi1LED, ON);
		HAL_Delay(200);
		BlinkLED(Rssi1LED, OFF);

		BlinkLED(Rssi2LED, ON);
		HAL_Delay(200);
		BlinkLED(Rssi2LED, OFF);

		BlinkLED(Rssi3LED, ON);
		HAL_Delay(200);
		BlinkLED(Rssi3LED, OFF);


		StartBeep();
		HAL_Delay(500);
		EndBeep();

		/* Lastly, turn the MASTER LED on, and don't change it */
		BlinkLED(MasterLED, ON);
	}
}

bool XBee_Transmit(uint8_t* txBuffer, uint8_t txBufferSize)
{

	uint16_t packetLength;
	uint8_t checksum;

	/* Let's build the packet */
	/* First, it's the Start of File indicator byte */
	xbeeTXBuffer[0] = SOF;

	/* Second, the Packet Length. This equals Payload Length + 14 Bytes (CMD-ID(1B) + frame id(1B) +  64-bit addr + 16 bit-addr + 2B) */
	packetLength = txBufferSize + 14;

	xbeeTXBuffer[1] = (uint8_t) (packetLength >> 8);
	xbeeTXBuffer[2] = (uint8_t) (packetLength);

	/* Command ID */
	xbeeTXBuffer[3] = CMD_TX_REQUEST;

	/* Frame ID - 0x00 = disables response frame */
	xbeeTXBuffer[4] = 0x01;

	/* 64-Bit Destination Address. In broadcast mode, this is 0x000000000000FFFF */
	xbeeTXBuffer[5] = 0x00;
	xbeeTXBuffer[6] = 0x00;
	xbeeTXBuffer[7] = 0x00;
	xbeeTXBuffer[8] = 0x00;
	xbeeTXBuffer[9] = 0x00;
	xbeeTXBuffer[10] = 0x00;
	xbeeTXBuffer[11] = 0xFF;
	xbeeTXBuffer[12] = 0xFF;

	/* 16-Bit Address. In broadcast mode, this is 0xFFFE */
	xbeeTXBuffer[13] = 0xFF;
	xbeeTXBuffer[14] = 0xFE;

	/* Broadcast Radius - maximum number of hops. 0 = max */
	xbeeTXBuffer[15] = 0x00;

	/* Transmit Options. We won't use any here */
	xbeeTXBuffer[16] = 0x00;

	/* Paste the payload in */
	memcpy(&xbeeTXBuffer[17], txBuffer, txBufferSize);

	/* Calculate the checksum */
	checksum = xbeeChecksum(&xbeeTXBuffer[3], packetLength);

	/* Insert the checksum */
	xbeeTXBuffer[17 + txBufferSize] = checksum;

	/* Transmit the packet to the XBee */
	HAL_StatusTypeDef transmitStatus = HAL_UART_Transmit(&huart1, xbeeTXBuffer, packetLength + 4, 100);

	/* Flash a code to indicate the error state
	 * The Beeps and flashes are blocking, so remove for release build */
	switch (transmitStatus) {
	case HAL_OK:
		return true;
	case HAL_ERROR:
		BlockingErrorAlert(2);
		break;
	case HAL_BUSY:
		BlockingErrorAlert(3);
		break;
	case HAL_TIMEOUT:
		BlockingErrorAlert(4);
		break;
	}
	return false;
}

void lookForXBee(void) {
	// Look for the XBee Reset Frame - this will make sure we have an XBee
	if (memcmp((uint8_t *)xbeeRxBuffer, resetSequence, sizeof(resetSequence)) == 0) {
		BlinkLED(Rssi1LED, OFF);
		BlinkLED(Rssi2LED, OFF);
		BlinkLED(Rssi3LED, OFF);
		EndBeep();

		machineState = ConfigureXBee;
		return;
	} else {
		// If we don't get the reset sequence we don't know if an XBee is attached.
		BlinkLED(Rssi1LED, ON);
		BlinkLED(Rssi2LED, ON);
		BlinkLED(Rssi3LED, ON);
		StartBeep();

		// Main loop does a retry after a timeout
		xbeeReinitTimeout = HAL_GetTick();
		return;
	}
}
uint8_t bytesLeft = 0;
uint8_t byteLocation = 0;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	/* Handle XBee input - this happens independent of slave or master mode */

	/* Unlike PC where we just look for newline ('\n'), or the radios where we look for NAK/ETX byte, this isn't trivial */
	/* First, we look for the SOF (start of file) byte. When we get this, we know the next 2 bytes will be the length */
	/* Second, we read the length which is 2 bytes. Then, we use this information to wait for the final amount of data incoming */
	/* Third and final, we read the rest of the data */
	if (huart->Instance == USART1)
	{

		if (machineState == LookingForXBee) {
			lookForXBee();
			return;
		}

		// All other commands start with 3 bytes: SOF, Length MSB and Length LSB
		if (xbeeStep == 1) {
			// check for SOF and calculate the size of the message
			if (xbeeRxBuffer[0] == SOF) {
				xbeeSize = (xbeeRxBuffer[1] << 8) | (xbeeRxBuffer[2] << 0);
				xbeeSize += 1;

				// save the first part of the message in the buffer, so that next time around the loop we know how big the payload is
				HAL_UART_Receive_IT(&huart1, &xbeeRxBuffer[3], 1);
				bytesLeft = xbeeSize - 1;
				byteLocation = 4;
				xbeeStep = 2;
				return;
			}
		}

		if (xbeeStep == 2) {
			HAL_UART_Receive_IT(&huart1, &xbeeRxBuffer[byteLocation], 1);
			byteLocation += 1;
			bytesLeft -= 1;
			if (bytesLeft == 0) {
				xbeeStep = 3;
			}
			return;
		}

		// This is receiving _most_ of the XBee reset sequence: 00 02 8A 00 75. It's missing the 7E at the start
		// https://www.digi.com/resources/documentation/Digidocs/90001500/Reference/r_frame_0x8A.htm
		// Changing it to interrupt doesn't work (ie. HAL_UART_Receive_IT)
//		memset(xbeeBuffer,'\0',10);
//		HAL_UART_Receive(&huart1, (uint8_t *)xbeeBuffer, 10, 100);

		xbeeTimeout = HAL_GetTick();

		if (xbeeStep == 1)
		{

			/* This is a valid XBee packet */
			/* Proceed to the next step, shift the buffer pointer up 1, and wait for 2 bytes */
			/* These 2 bytes will be the length */
			if (xbeeRxBuffer[0] == SOF)
			{

				xbeeStep = 2;
				xbeeSize = 2;
				HAL_UART_Receive_IT(&huart1, &xbeeRxBuffer[1], xbeeSize);

			}
			/* Otherwise, this is an invalid packet/we got out of sync */
			/* Restart and continue the hunt for a valid packet */
			else
			{
				xbeeStep = 1;
				HAL_UART_Receive_IT(&huart1, xbeeRxBuffer, 1);
			}

		}
		else if (xbeeStep == 2)
		{

			/* Here, we calculate the length of the packet incoming */
			/* This is then used for the final step where we grab the data we want */
			xbeeSize = (xbeeRxBuffer[1] << 8) | (xbeeRxBuffer[2] << 0);

			/* The real size is an additional 1 bytes on top of this for the checksum */
			xbeeSize += 1;

			/* We add a check here to make sure the length is sensible */
			/* If it is sensible, we move up the buffer a few positions and look for the remaining packet size */
			if (xbeeSize <= XBEE_MAX_PACKET_SIZE)
			{
				xbeeStep = 3;
				HAL_UART_Receive_IT(&huart1, &xbeeRxBuffer[3], xbeeSize);
			}
			/* Otherwise, this is an invalid packet/we got out of sync */
			/* Restart and continue the hunt for a valid packet */
			else
			{
				xbeeStep = 1;
				HAL_UART_Receive_IT(&huart1, xbeeRxBuffer, 1);
			}
		}
		else if (xbeeStep == 3)
		{

			/* Here, we have received the packet of data */
			/* There are several checks we need to make here */
			/* First, check that it's a valid Orienteering Device packet */
//			if (xbeeBuffer[0] == OD_ID
//			 && xbeeBuffer[1] == currentChannel)
//			{

				xbeePacketComplete = true;

//			}
			/* Whether it was a valid packet or not, we begin scanning again for new data */
			xbeeStep = 1;
			HAL_UART_Receive_IT(&huart1, xbeeRxBuffer, 1);

		}

	}

	/* Manage data coming in from the radios */
	if (mode == SLAVE)
	{

		/* Handle Red Radio input */
		if (huart->Instance == USART4)
		{
			radioRedBuffer[radioRedTracker] = radioRedIn;
			radioRedTracker++;

			if (radioRedIn == NAK || radioRedIn == ETX)
			{
				radioRedPacketComplete = true;
			}

			HAL_UART_Receive_IT(&huart4, &radioRedIn, 1);

		}

		/* Handle Blue Radio input */
		if (huart->Instance == USART2)
		{
			radioBlueBuffer[radioBlueTracker] = radioBlueIn;
			radioBlueTracker++;

			if (radioBlueIn == NAK || radioBlueIn == ETX)
			{
				radioBluePacketComplete = true;
			}

			HAL_UART_Receive_IT(&huart2, &radioBlueIn, 1);

		}

		/* Handle Auxiliary Radio input */
		if (huart->Instance == USART5)
		{
			radioAuxBuffer[radioAuxTracker] = radioAuxIn;
			radioAuxTracker++;

			if (radioAuxIn == NAK || radioAuxIn == ETX)
			{
				radioAuxPacketComplete = true;
			}

			HAL_UART_Receive_IT(&huart5, &radioAuxIn, 1);

		}

	}

	/* Manage data coming from the PC */
	else if (mode == MASTER)
	{

		/* Handle PC input */
		if (huart->Instance == USART5)
		{
			PCBuffer[PCTracker] = PCIn;
			PCTracker++;

			if (PCIn == '\n')
			{
				PCPacketComplete = true;
			}

			HAL_UART_Receive_IT(&huart5, &PCIn, 1);

		}

	}


}

void InitialiseHardware(void) {
	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_I2C1_Init();
	MX_TIM3_Init();
	MX_USART1_UART_Init();
	MX_USART2_UART_Init();
	MX_USART4_UART_Init();
	MX_USART5_UART_Init();

	/* Read the switch to see if we are a master or slave */
	mode = (bool)(HAL_GPIO_ReadPin(GPIOC, SWITCH_Pin));

	HAL_UART_Receive_IT(&huart1, xbeeRxBuffer, sizeof(resetSequence));

	if (mode == SLAVE)
	{
		/* Enable 3 radio inputs */
		HAL_UART_Receive_IT(&huart4, &radioRedIn, 1);
		HAL_UART_Receive_IT(&huart2, &radioBlueIn, 1);
		HAL_UART_Receive_IT(&huart5, &radioAuxIn, 1);
	}
	else if (mode == MASTER)
	{
		/* Enable PC communications */
		HAL_UART_Receive_IT(&huart5, &PCIn, 1);
	}

	/* Wake up the XBee */
	HAL_GPIO_WritePin(GPIOA, XBEE_RESETn_Pin, GPIO_PIN_SET);
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

	/** Configure the main internal regulator output voltage
	 */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
	/** Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_4;
	RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
			|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
	{
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_USART2
			|RCC_PERIPHCLK_I2C1;
	PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
	PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
	PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
