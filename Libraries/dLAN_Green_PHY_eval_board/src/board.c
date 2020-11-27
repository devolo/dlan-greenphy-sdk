/*
 * @brief NXP LPC1769 LPCXpresso board file
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2012
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

/* Standard includes. */
#include "string.h"

/* LPCOpen includes. */
#include "board.h"
#include "retarget.h"

/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include "semphr.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/* System oscillator rate and RTC oscillator rate */
const uint32_t OscRateIn = 12000000;
const uint32_t RTCOscRateIn = 32768;

SemaphoreHandle_t xI2C1_Mutex = NULL;

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* Initializes board LED(s) */
static void Board_LED_Init(void)
{
	/* LED0 pin is configured as GPIO pin during SystemInit */
	/* Set the LED0 pin as output */
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, LED0_GPIO_PORT_NUM, LED0_GPIO_BIT_NUM);
	Board_LED_Set(LEDS_LED0, USR_LED_ON);
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Initialize UART pins */
void Board_UART_Init(LPC_USART_T *pUART)
{
	/* Pin Muxing has already been done during SystemInit */
}

/* Initialize debug output via UART for board */
void Board_Debug_Init(void)
{
#if defined(DEBUG_ENABLE)
	Board_UART_Init(DEBUG_UART);

	Chip_UART_Init(DEBUG_UART);
	Chip_UART_SetBaud(DEBUG_UART, 115200);
	Chip_UART_ConfigData(DEBUG_UART, UART_LCR_WLEN8 | UART_LCR_SBS_1BIT | UART_LCR_PARITY_DIS);

	/* Enable UART Transmit */
	Chip_UART_TXEnable(DEBUG_UART);
#endif
}

/* Sends a character on the UART */
void Board_UARTPutChar(char ch)
{
#if defined(DEBUG_ENABLE)
	while ((Chip_UART_ReadLineStatus(DEBUG_UART) & UART_LSR_THRE) == 0) {}
	Chip_UART_SendByte(DEBUG_UART, (uint8_t) ch);
#endif
}

/* Gets a character from the UART, returns EOF if no character is ready */
int Board_UARTGetChar(void)
{
#if defined(DEBUG_ENABLE)
	if (Chip_UART_ReadLineStatus(DEBUG_UART) & UART_LSR_RDR) {
		return (int) Chip_UART_ReadByte(DEBUG_UART);
	}
#endif
	return EOF;
}

/* Outputs a string on the debug UART */
void Board_UARTPutSTR(char *str)
{
#if defined(DEBUG_ENABLE)
	while (*str != '\0') {
		Board_UARTPutChar(*str++);
	}
#endif
}

/* Sets the state of a board LED to on or off */
void Board_LED_Set(uint8_t LEDNumber, bool Status)
{
	/* There is only one LED */
	if (LEDNumber == LEDS_LED0) {
		/* LED0 is ON if GPIO is LOW. */
		Chip_GPIO_SetPinState(LPC_GPIO, LED0_GPIO_PORT_NUM, LED0_GPIO_BIT_NUM, !Status);
	}
}

/* Returns the current state of a board LED */
bool Board_LED_Test(uint8_t LEDNumber)
{
	bool state = false;

	if (LEDNumber == LEDS_LED0) {
		/* LED0 is ON if GPIO is LOW. */
		state = !Chip_GPIO_GetPinState(LPC_GPIO, LED0_GPIO_PORT_NUM, LED0_GPIO_BIT_NUM);
	}

	return state;
}

void Board_LED_Toggle(uint8_t LEDNumber)
{
	if (LEDNumber == LEDS_LED0) {
		Board_LED_Set(LEDNumber, !Board_LED_Test(LEDNumber));
	}
}

/* Set up and initialize all required blocks and functions related to the
   board hardware */
void Board_Init(void)
{
	/* Sets up DEBUG UART */
	DEBUGINIT();

	/* Initializes GPIO */
	Chip_GPIO_Init(LPC_GPIO);
	Chip_IOCON_Init(LPC_IOCON);

	/* Initialize LEDs */
	Board_LED_Init();
}

extern const uint8_t ucMACAddress[ 6 ];

/* Returns the MAC address assigned to this board */
void Board_ENET_GetMacADDR(uint8_t *mcaddr)
{
	memcpy(mcaddr, ucMACAddress, 6);
}

/* Initialize SSP interface */
void Board_SSP_Init(LPC_SSP_T *pSSP, bool isMaster)
{
	/* Set up clock and muxing for SPI0 interface */

	/*
	 * For SSP0:
	 * P0.15: SCK0
	 * P0.16: SSEL0
	 * P0.17: MISO0
	 * P0.18: MOSI0
	 *
	 * For SSP1:
	 * P0.6: SSEL1 on evalboard connected to USR LED
	 * P0.7: SCK1
	 * P0.8: MISO1
	 * P0.9: MOSI1
	 */

	if (pSSP == LPC_SSP0)
	{
		Chip_IOCON_PinMux(LPC_IOCON, SSP0_SCK_PORT, SSP0_SCK_PIN, IOCON_MODE_PULLDOWN, IOCON_FUNC2);
		if (isMaster)
		{
			Chip_IOCON_PinMux(LPC_IOCON, SSP0_SSEL_PORT, SSP0_SSEL_PIN, IOCON_MODE_PULLUP, IOCON_FUNC0);
			Chip_GPIO_SetPinDIROutput(LPC_GPIO, SSP0_SSEL_PORT, SSP0_SSEL_PIN);
			Board_SSP_DeassertSSEL(pSSP);
		}
		else
		{
			Chip_IOCON_PinMux(LPC_IOCON, SSP0_SSEL_PORT, SSP0_SSEL_PIN, IOCON_MODE_PULLUP, IOCON_FUNC2);
		}
		Chip_IOCON_PinMux(LPC_IOCON, SSP0_MISO_PORT, SSP0_MISO_PIN, IOCON_MODE_INACT, IOCON_FUNC2);
		Chip_IOCON_PinMux(LPC_IOCON, SSP0_MOSI_PORT, SSP0_MOSI_PIN, IOCON_MODE_INACT, IOCON_FUNC2);

		Chip_SSP_Init(pSSP);

		/* QCA7000 specific configuration */
		Chip_SSP_Set_Mode(pSSP, SSP_MODE_MASTER);
		Chip_SSP_SetFormat(pSSP, SSP_BITS_8, SSP_FRAMEFORMAT_SPI, SSP_CLOCK_CPHA1_CPOL1);
		Chip_SSP_SetBitRate(pSSP, 12000000);
	}
	else if (pSSP == LPC_SSP1)
	{
		Chip_IOCON_PinMux(LPC_IOCON, 0, 7, IOCON_MODE_PULLDOWN, IOCON_FUNC2);
		if (isMaster)
		{
			Chip_IOCON_PinMux(LPC_IOCON, 0, 6, IOCON_MODE_PULLUP, IOCON_FUNC0);
			Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 6);
			Board_SSP_DeassertSSEL(pSSP);
		}
		else
		{
			Chip_IOCON_PinMux(LPC_IOCON, 0, 6, IOCON_MODE_PULLUP, IOCON_FUNC2);
		}
		Chip_IOCON_PinMux(LPC_IOCON, 0, 8, IOCON_MODE_INACT, IOCON_FUNC2);
		Chip_IOCON_PinMux(LPC_IOCON, 0, 9, IOCON_MODE_INACT, IOCON_FUNC2);

		Chip_SSP_Init(pSSP);
	}
	Chip_SSP_Enable(pSSP);
}

/* Assert SSEL pin */
bool Board_SSP_AssertSSEL(LPC_SSP_T *pSSP)
{
	int rv = 0;
	if (pSSP == LPC_SSP0)
	{
		rv = Chip_GPIO_GetPinState(LPC_GPIO, SSP0_SSEL_PORT, SSP0_SSEL_PIN);
		Chip_GPIO_SetPinOutLow(LPC_GPIO, SSP0_SSEL_PORT, SSP0_SSEL_PIN);
	}
	else if (pSSP == LPC_SSP1)
	{
		rv = Chip_GPIO_GetPinState(LPC_GPIO, 0, 6);
		Chip_GPIO_SetPinOutLow(LPC_GPIO, 0, 6);
	}

	return rv;
}

/* De-Assert SSEL pin */
bool Board_SSP_DeassertSSEL(LPC_SSP_T *pSSP)
{
	int rv = 0;
	if (pSSP == LPC_SSP0)
	{
		rv = Chip_GPIO_GetPinState(LPC_GPIO, SSP0_SSEL_PORT, SSP0_SSEL_PIN);
		Chip_GPIO_SetPinOutHigh(LPC_GPIO, SSP0_SSEL_PORT, SSP0_SSEL_PIN);
	}
	else if (pSSP == LPC_SSP1)
	{
		rv = Chip_GPIO_GetPinState(LPC_GPIO, 0, 6);
		Chip_GPIO_SetPinOutHigh(LPC_GPIO, 0, 6);
	}

	return rv;
}

/* Sets up board specific I2C interface */
void Board_I2C_Init(I2C_ID_T id)
{
	switch (id) {
	case I2C0:
		/*not available on devolo evaluation board*/
		break;

	case I2C1:
		if( xI2C1_Mutex == NULL )
		{
			/*official clickboard I2C interface*/
			Chip_IOCON_PinMux(LPC_IOCON, CLICKBOARD_SDA_I2C1_PORT_NUM, CLICKBOARD_SDA_I2C1_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC3);
			Chip_IOCON_PinMux(LPC_IOCON, CLICKBOARD_SCL_I2C1_PORT_NUM, CLICKBOARD_SCL_I2C1_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC3);
			Chip_IOCON_EnableOD(LPC_IOCON, CLICKBOARD_SDA_I2C1_PORT_NUM, CLICKBOARD_SDA_I2C1_BIT_NUM);
			Chip_IOCON_EnableOD(LPC_IOCON, CLICKBOARD_SCL_I2C1_PORT_NUM, CLICKBOARD_SCL_I2C1_BIT_NUM);

			Chip_I2C_Init(I2C1);
			#define SPEED_400KHZ 400000
			Chip_I2C_SetClockRate(I2C1, SPEED_400KHZ);
			Chip_I2C_SetMasterEventHandler(I2C1, Chip_I2C_EventHandlerPolling);

			xI2C1_Mutex = xSemaphoreCreateMutex();
		}
		break;

	case I2C2:
		/*RX and TX pin on clickboard2 I2C interface, normally used by UART*/
		Chip_IOCON_PinMux(LPC_IOCON, CLICKBOARD2_SDA_I2C2_PORT_NUM, CLICKBOARD2_SDA_I2C2_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC2);
		Chip_IOCON_PinMux(LPC_IOCON, CLICKBOARD2_SCL_I2C2_PORT_NUM, CLICKBOARD2_SCL_I2C2_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC2);
		Chip_IOCON_EnableOD(LPC_IOCON, CLICKBOARD2_SDA_I2C2_PORT_NUM, CLICKBOARD2_SDA_I2C2_BIT_NUM);
		Chip_IOCON_EnableOD(LPC_IOCON, CLICKBOARD2_SCL_I2C2_PORT_NUM, CLICKBOARD2_SCL_I2C2_BIT_NUM);
		break;
	case I2C_NUM_INTERFACE:
		break;
	}
}

void Board_Buttons_Init(void)
{
	Chip_GPIO_WriteDirBit(LPC_GPIO, BUTTONS_BUTTON1_GPIO_PORT_NUM, BUTTONS_BUTTON1_GPIO_BIT_NUM, false);
}

uint32_t Buttons_GetStatus(void)
{
	uint8_t ret = NO_BUTTON_PRESSED;
	if (Chip_GPIO_ReadPortBit(LPC_GPIO, BUTTONS_BUTTON1_GPIO_PORT_NUM, BUTTONS_BUTTON1_GPIO_BIT_NUM) == 0x00) {
		ret |= BUTTONS_BUTTON1;
	}
	return ret;
}

void Board_USBD_Init(uint32_t port)
{
	/* Not tested */
	Chip_IOCON_PinMux(LPC_IOCON, VBUS_USB_PORT_NUM, VBUS_USB_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC2);/* USB VBUS */
	
	Chip_IOCON_PinMux(LPC_IOCON, NEGATIVE_USB_PORT_NUM, NEGATIVE_USB_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC1);	//D1+
	Chip_IOCON_PinMux(LPC_IOCON, POSITIVE_USB_PORT_NUM, POSITIVE_USB_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC1);   //D1-

	LPC_USB->USBClkCtrl = 0x12;                /* Dev, AHB clock enable */
	while ((LPC_USB->USBClkSt & 0x12) != 0x12); 
}

