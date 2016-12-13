/*
 * @brief devolo LPC1758 dlAN Green PHY Sysinit file
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
		/* default initialization with basic GreenPHY evalboard functions*/

		/*  PORT0   */
//    {0,  0,   IOCON_MODE_INACT | IOCON_FUNC0},	/* default GPIO; click I2C SDA1; P0.0 GPIO Port 0.0 RD1 TXD3 SDA1 */
//	{0,  1,   IOCON_MODE_INACT | IOCON_FUNC0},	/* default GPIO; click I2C SCL1; P0.1 GPIO Port 0.1 TD1 RXD3 SCL1 */
	{0,  2,   IOCON_MODE_INACT | IOCON_FUNC1},	/* default TXD0 UART pinheader;  P0.2 GPIO Port 0.2 TXD0 AD0.7 Reserved  */
	{0,  3,   IOCON_MODE_INACT | IOCON_FUNC1},	/* default RXD0 UART pinheader;  P0.3 GPIO Port 0.3 RXD0 AD0.6 Reserved */
//	{0,  4,   IOCON_MODE_INACT | IOCON_FUNC0},	/* Not available on 80-pin package. */
//	{0,  5,   IOCON_MODE_INACT | IOCON_FUNC0},	/* Not available on 80-pin package. */
//	{0,  6,   IOCON_MODE_INACT | IOCON_FUNC0},	/* default on board LED;         P0.6 GPIO Port 0.6 I2SRX_SDA SSEL1 MAT2.0 */
//	{0,  7,   IOCON_MODE_INACT | IOCON_FUNC0},	/* default GPIO; click SPI SCK1; P0.7 GPIO Port 0.7 I2STX_CLK SCK1 MAT2.1 */
//	{0,  8,   IOCON_MODE_INACT | IOCON_FUNC0},	/* default GPIO; click SPI MISO1;P0.8 GPIO Port 0.8 I2STX_WS MISO1 MAT2.2 */
//	{0,  9,   IOCON_MODE_INACT | IOCON_FUNC0},	/* default GPIO; click SPI MOSI1;P0.9 GPIO Port 0.9 I2STX_SDA MOSI1 MAT2.3 */
//	{0, 10,   IOCON_MODE_INACT | IOCON_FUNC0},	/* default GPIO; click TXD2 UART mikrobus2; P0.10 GPIO Port 0.10 TXD2 SDA2 MAT3.0*/
//	{0, 11,   IOCON_MODE_INACT | IOCON_FUNC0},	/* default GPIO; click RXD2 UART mikrobus2; P0.11 GPIO Port 0.11 RXD2 SCL2 MAT3.1 */
	{0, 15,   IOCON_MODE_INACT | IOCON_FUNC2},	/* default: greenPHY SCK                    P0.15 GPIO Port 0.15 TXD1 SCK0 SCK */
	{0, 16,   IOCON_MODE_INACT | IOCON_FUNC2},	/* default: greenPHY SSEL                   P0.16 GPIO Port 0.16 RXD1 SSEL0 SSEL*/
	{0, 17,   IOCON_MODE_INACT | IOCON_FUNC2},	/* default: greenPHY MISO                   P0.17 GPIO Port 0.17 CTS1 MISO0 MISO*/
	{0, 18,   IOCON_MODE_INACT | IOCON_FUNC2},	/* default: greenPHY MOSI                   P0.18 GPIO Port 0.18 DCD1 MOSI0 MOSI*/
//	{0, 19,   IOCON_MODE_INACT | IOCON_FUNC0},	/* Not available on 80-pin package. */
//	{0, 20,   IOCON_MODE_INACT | IOCON_FUNC0},	/* Not available on 80-pin package. */
//	{0, 21,   IOCON_MODE_INACT | IOCON_FUNC0},	/* Not available on 80-pin package. */
//	{0, 22,   IOCON_MODE_INACT | IOCON_FUNC0},	/* default: greenPHY int-GPIO               P0.22 GPIO Port 0.22 RTS1 Reserved TD1*/
//	{0, 23,   IOCON_MODE_INACT | IOCON_FUNC0},	/* Not available on 80-pin package. */
//	{0, 24,   IOCON_MODE_INACT | IOCON_FUNC0},	/* Not available on 80-pin package. */
//	{0, 25,   IOCON_MODE_INACT | IOCON_FUNC0},	/* default GPIO; click ANA mikrobus2;       P0.25 GPIO Port 0.25 AD0.2 I2SRX_SDA TXD3  */
//	{0, 26,   IOCON_MODE_INACT | IOCON_FUNC0},	/* default GPIO-GPP PIN2 J12;               P0.26 GPIO Port 0.26 AD0.3 AOUT RXD3 */
//	{0, 27,   IOCON_MODE_INACT | IOCON_FUNC0},	/* Not available on 80-pin package. */
//	{0, 28,   IOCON_MODE_INACT | IOCON_FUNC0},	/* Not available on 80-pin package. */
//	{0, 29,   IOCON_MODE_INACT | IOCON_FUNC0},	/* default GPIO; evalboard USB+             P0.29 GPIO Port 0.29 USB_D Reserved Reserved */
//	{0, 30,   IOCON_MODE_INACT | IOCON_FUNC0},	/* default GPIO; evalboard USB-             P0.30 GPIO Port 0.30 USB_D Reserved Reserved */

    	/*  PORT1   */
	{0x1, 0,  IOCON_MODE_INACT | IOCON_FUNC1},	/* default: ENET_TXD0                       P1.00 GPIO Port 1.00 ENET_TXD0 Reserved Reserved    */
	{0x1, 1,  IOCON_MODE_INACT | IOCON_FUNC1},	/* default: ENET_TXD1                       P1.01 GPIO Port 1.01 ENET_TXD1 Reserved Reserved    */
	{0x1, 4,  IOCON_MODE_INACT | IOCON_FUNC1},	/* default: ENET_TX_EN                      P1.04 GPIO Port 1.04 ENET_TX_EN Reserved Reserved   */
	{0x1, 8,  IOCON_MODE_INACT | IOCON_FUNC1},	/* default: ENET_CRS                        P1.08 GPIO Port 1.08 ENET_CRS Reserved Reserved     */
	{0x1, 9,  IOCON_MODE_INACT | IOCON_FUNC1},	/* default: ENET_RXD0                       P1.09 GPIO Port 1.09 ENET_RXD0 Reserved Reserved    */
	{0x1, 10, IOCON_MODE_INACT | IOCON_FUNC1},	/* default: ENET_RXD1                       P1.10 GPIO Port 1.10 ENET_RXD1 Reserved Reserved    */
	{0x1, 14, IOCON_MODE_INACT | IOCON_FUNC1},	/* default: ENET_RX_ER                      P1.14 GPIO Port 1.14 ENET_RX_ER Reserved Reserved   */
	{0x1, 15, IOCON_MODE_INACT | IOCON_FUNC1},	/* default: ENET_REF_CLK                    P1.15 GPIO Port 1.15 ENET_REF_CLK Reserved Reserved */
//	{0x1, 16, IOCON_MODE_INACT | IOCON_FUNC0},	/* Not available on 80-pin package. */
//	{0x1, 17, IOCON_MODE_INACT | IOCON_FUNC0},	/* Not available on 80-pin package. */
//	{0x1, 18, IOCON_MODE_INACT | IOCON_FUNC0},	/* default: GPIO not used                   P1.18 GPIO Port 1.18 USB_UP_LED PWM1.1 CAP1.0 */
//	{0x1, 19, IOCON_MODE_INACT | IOCON_FUNC0},	/* default: GPIO not used                   P1.19 GPIO Port 1.19 MCOA0 USB_PPWR CAP1.1 */
//	{0x1, 20, IOCON_MODE_INACT | IOCON_FUNC0},	/* default: ENET_RMII_MDC on GPIO           P1.20 GPIO Port 1.20 MCI0 PWM1.2 SCK0 */
//	{0x1, 21, IOCON_MODE_INACT | IOCON_FUNC0},	/* Not available on 80-pin package. */
//	{0x1, 22, IOCON_MODE_INACT | IOCON_FUNC0},	/* default: GPIO not used                   P1.22 GPIO Port 1.22 MCOB0 USB_PWRD MAT1.0 */
//	{0x1, 23, IOCON_MODE_INACT | IOCON_FUNC0},	/* default: ENET_RMII_MDIO on GPIO          P1.23 GPIO Port 1.23 MCI1 PWM1.4 MISO0 */
//	{0x1, 24, IOCON_MODE_INACT | IOCON_FUNC0},	/* default: ENET_RMII_RESET on GPIO         P1.24 GPIO Port 1.24 MCI2 PWM1.5 MOSI0 */
//	{0x1, 25, IOCON_MODE_INACT | IOCON_FUNC0},	/* default: GPIO not used                   P1.25 GPIO Port 1.25 MCOA1 Reserved MAT1.1 */
//	{0x1, 26, IOCON_MODE_INACT | IOCON_FUNC0},	/* default: RST mikrobus1 on GPIO           P1.26 GPIO Port 1.26 MCOB1 PWM1.6 CAP0.0 */
//	{0x1, 27, IOCON_MODE_INACT | IOCON_FUNC0},	/* Not available on 80-pin package. */
//	{0x1, 28, IOCON_MODE_INACT | IOCON_FUNC0},	/* default: RST mikrobus2 on GPIO           P1.28 GPIO Port 1.28 MCOA2 PCAP1.0 MAT0.0 */
//	{0x1, 29, IOCON_MODE_INACT | IOCON_FUNC0},	/* default: GPIO not used                   P1.29 GPIO Port 1.29 MCOB2 PCAP1.1 MAT0.1 */
//	{0x1, 30, IOCON_MODE_INACT | IOCON_FUNC0},	/* default: GPIO; USB VBUS on evalboard     P1.30 GPIO Port 1.30 Reserved VBUS AD0.4 */
//	{0x1, 31, IOCON_MODE_INACT | IOCON_FUNC0},	/* default: ANA mikrobus1 on GPIO           P1.31 GPIO Port 1.31 Reserved SCK1 AD0.5 */


	   /*  PORT2   */
//	{2, 0,  IOCON_MODE_INACT | IOCON_FUNC0},	/* default: GPIO; evalb.TXD2 UART mikrobus1 P2.0 GPIO Port 2.0 PWM1.1 TXD1 Reserved*/
//	{2, 1,  IOCON_MODE_INACT | IOCON_FUNC0},	/* default: GPIO; evalb.RXD2 UART mikrobus1 P2.1 GPIO Port 2.1 PWM1.2 RXD1 Reserved*/
//	{2, 2,  IOCON_MODE_INACT | IOCON_FUNC0},	/* default: CS on mikrobus1 GPIO            P2.2 GPIO Port 2.2 PWM1.3 CTS1 Reserved mikrobus1 */
//	{2, 3,  IOCON_MODE_INACT | IOCON_FUNC0},	/* default: INT mikrobus1 on GPIO           P2.3 GPIO Port 2.3 PWM1.4 DCD1 Reserved*/
//	{2, 4,  IOCON_MODE_INACT | IOCON_FUNC0},	/* default: PWM mikrobus1 on GPIO           P2.4 GPIO Port 2.4 PWM1.5 DSR1 Reserved*/
//	{2, 5,  IOCON_MODE_INACT | IOCON_FUNC0},	/* default: PWM mikrobus2 on GPIO           P2.5 GPIO Port 2.5 PWM1.6 DTR1 Reserved*/
//	{2, 6,  IOCON_MODE_INACT | IOCON_FUNC0},	/* default: int mikrobus2 on GPIO           P2.6 GPIO Port 2.6 PCAP1.0 RI1 Reserved*/
//	{2, 7,  IOCON_MODE_INACT | IOCON_FUNC0},	/* default: CS mikrobus2 on GPIO            P2.7 GPIO Port 2.7 RD2 RTS1 Reserved*/
//	{2, 8,  IOCON_MODE_INACT | IOCON_FUNC0},	/* default: GPIO not used                   P2.8 GPIO Port 2.8 TD2 TXD2 ENET_MDC */
//	{2, 9,  IOCON_MODE_INACT | IOCON_FUNC0},	/* default: GPIO not used                   P2.9 GPIO Port 2.9 USB_CONNECT RXD2 ENET_MDIO */
//	{2,10,  IOCON_MODE_INACT | IOCON_FUNC0},	/* default: MINT onboard Button on GPIO     P2.10 GPIO Port 2.10 EINT0 NMI Reserved */
//	{2,11,  IOCON_MODE_INACT | IOCON_FUNC0},	/* Not available on 80-pin package. */
//	{2,12,  IOCON_MODE_INACT | IOCON_FUNC0},	/* Not available on 80-pin package. */
//	{2,13,  IOCON_MODE_INACT | IOCON_FUNC0},	/* Not available on 80-pin package. */


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
