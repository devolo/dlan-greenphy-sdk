/*
 * @brief devolo LPC1769 dlAN Green PHY Sysinit file
 *
 * @note
 * Copyright(C) devolo AG, 2016
 * All rights reserved.
 *
 */

#include "board.h"

/* The System initialization code is called prior to the application and
   initializes the board for run-time operation. Board initialization
   includes clock setup and default pin muxing configuration. */

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/* Pin muxing configuration */
STATIC const PINMUX_GRP_T pinmuxing[] = {
		/*  PORT0   */
    {0,  0,   IOCON_MODE_INACT | IOCON_FUNC3},	/* I2C SDA1 */
	{0,  1,   IOCON_MODE_INACT | IOCON_FUNC3},	/* I2C SCL1 */
	{0,  2,   IOCON_MODE_INACT | IOCON_FUNC1},	/* TXD0 UART pinheader */
	{0,  3,   IOCON_MODE_INACT | IOCON_FUNC1},	/* RXD0 UART pinheader */
	{0,  4,   IOCON_MODE_INACT | IOCON_FUNC2},	/* CAN-RD2 NOT USED */
	{0,  5,   IOCON_MODE_INACT | IOCON_FUNC2},	/* CAN-TD2 NOT USED */
	{0,  6,   IOCON_MODE_INACT | IOCON_FUNC0},	/* on board LED */
	{0,  7,   IOCON_MODE_INACT | IOCON_FUNC2},	/* SPI SCK1 */
	{0,  8,   IOCON_MODE_INACT | IOCON_FUNC2},	/* SPI MISO1 */
	{0,  9,   IOCON_MODE_INACT | IOCON_FUNC2},	/* SPI MOSI1 */
	{0, 10,   IOCON_MODE_INACT | IOCON_FUNC0},	/* GPIO or TXD2 UART mikrobus2 */
	{0, 11,   IOCON_MODE_INACT | IOCON_FUNC0},	/* GPIO or RXD2 UART mikrobus2 */
	{0, 25,   IOCON_MODE_INACT | IOCON_FUNC0},	/* ANA mikrobus2  */
	{0, 26,   IOCON_MODE_INACT | IOCON_FUNC0},	/* GPP PIN2 J12 */
	{0, 29,   IOCON_MODE_INACT | IOCON_FUNC1},	/* USB+ */
	{0, 30,   IOCON_MODE_INACT | IOCON_FUNC1},	/* USB- */

    	/*  PORT1   */
	{0x1, 0,  IOCON_MODE_INACT | IOCON_FUNC1},	/* ENET_TXD0 */
	{0x1, 1,  IOCON_MODE_INACT | IOCON_FUNC1},	/* ENET_TXD1 */
	{0x1, 4,  IOCON_MODE_INACT | IOCON_FUNC1},	/* ENET_TX_EN */
	{0x1, 8,  IOCON_MODE_INACT | IOCON_FUNC1},	/* ENET_CRS */
	{0x1, 9,  IOCON_MODE_INACT | IOCON_FUNC1},	/* ENET_RXD0 */
	{0x1, 10, IOCON_MODE_INACT | IOCON_FUNC1},	/* ENET_RXD1 */
	{0x1, 14, IOCON_MODE_INACT | IOCON_FUNC1},	/* ENET_RX_ER */
	{0x1, 15, IOCON_MODE_INACT | IOCON_FUNC1},	/* ENET_REF_CLK */
	{0x1, 16, IOCON_MODE_INACT | IOCON_FUNC1},	/* ENET_MDC */
	{0x1, 17, IOCON_MODE_INACT | IOCON_FUNC1},	/* ENET_MDIO */
	{0x1, 26, IOCON_MODE_INACT | IOCON_FUNC0},	/* RST mikrobus1 */
	{0x1, 27, IOCON_MODE_INACT | IOCON_FUNC1},	/* CLKOUT*/
	{0x1, 28, IOCON_MODE_INACT | IOCON_FUNC0},	/* RST mikrobus2 */
	{0x1, 31, IOCON_MODE_INACT | IOCON_FUNC0},	/* ANA mikrobus1 */


	   /*  PORT2   */
	{2, 0,  IOCON_MODE_INACT | IOCON_FUNC0},	/* GPIO or TXD2 UART mikrobus1 */
	{2, 1,  IOCON_MODE_INACT | IOCON_FUNC0},	/* GPIO or RXD2 UART mikrobus1 */
	{2, 2,  IOCON_MODE_INACT | IOCON_FUNC0},	/* CS mikrobus1 */
	{2, 3,  IOCON_MODE_INACT | IOCON_FUNC0},	/* INT mikrobus1 */
	{2, 4,  IOCON_MODE_INACT | IOCON_FUNC0},	/* PWM mikrobus1 */
	{2, 5,  IOCON_MODE_INACT | IOCON_FUNC0},	/* PWM mikrobus2 */
	{2, 6,  IOCON_MODE_INACT | IOCON_FUNC0},	/* int mikrobus2 */
	{2, 7,  IOCON_MODE_INACT | IOCON_FUNC0},	/* CS mikrobus2 */
	{2,10,  IOCON_MODE_INACT | IOCON_FUNC0},	/* MINT onboard Button */


};

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Sets up system pin muxing */
void Board_SetupMuxing(void)
{
	Chip_IOCON_SetPinMuxing(LPC_IOCON, pinmuxing, sizeof(pinmuxing) / sizeof(PINMUX_GRP_T));
}

/* Setup system clocking */
void Board_SetupClocking(void)
{
	Chip_SetupXtalClocking();

	/* Setup FLASH access to 4 clocks (100MHz clock) */
	Chip_SYSCTL_SetFLASHAccess(FLASHTIM_100MHZ_CPU);
}

/* Set up and initialize hardware prior to call to main */
void Board_SystemInit(void)
{
	Board_SetupMuxing();
	Board_SetupClocking();
}
