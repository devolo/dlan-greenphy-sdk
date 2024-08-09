/*
 * Copyright (c) 2016-2024, devolo solutions GmbH, Aachen, Germany.
 * All rights reserved.
 *
 * This Software is part of the devolo GreenPHY-SDK.
 *
 * Usage in source form and redistribution in binary form, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Usage in source form is subject to a current end user license agreement
 *    with the devolo solutions GmbH.
 * 2. Neither the name of the devolo solutions GmbH nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 3. Redistribution in binary form is limited to the usage on the GreenPHY
 *    module of the devolo solutions GmbH.
 * 4. Redistribution in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
//	{0,  0,   IOCON_MODE_INACT    | IOCON_FUNC0},  /* click I2C SDA1;            P0.0  GPIO RD1       TXD3      SDA1     */
//	{0,  1,   IOCON_MODE_INACT    | IOCON_FUNC0},  /* click I2C SCL1;            P0.1  GPIO TD1       RXD3      SCL1     */
	{0,  2,   IOCON_MODE_INACT    | IOCON_FUNC1},  /* TXD0 UART pinheader;       P0.2  GPIO TXD0      AD0.7     Reserved */
	{0,  3,   IOCON_MODE_INACT    | IOCON_FUNC1},  /* RXD0 UART pinheader;       P0.3  GPIO RXD0      AD0.6     Reserved */
//	{0,  4,   IOCON_MODE_INACT    | IOCON_FUNC0},  /*                Not available on 80-pin package.                    */
//	{0,  5,   IOCON_MODE_INACT    | IOCON_FUNC0},  /*                Not available on 80-pin package.                    */
//	{0,  6,   IOCON_MODE_INACT    | IOCON_FUNC0},  /* on board USR LED;          P0.6  GPIO I2SRX_SDA SSEL1     MAT2.0   */
//	{0,  7,   IOCON_MODE_INACT    | IOCON_FUNC0},  /* click SPI SCK1;            P0.7  GPIO I2STX_CLK SCK1      MAT2.1   */
//	{0,  8,   IOCON_MODE_INACT    | IOCON_FUNC0},  /* click SPI MISO1;           P0.8  GPIO I2STX_WS  MISO1     MAT2.2   */
//	{0,  9,   IOCON_MODE_INACT    | IOCON_FUNC2},  /* click SPI MOSI1;           P0.9  GPIO I2STX_SDA MOSI1     MAT2.3   */
//	{0, 10,   IOCON_MODE_INACT    | IOCON_FUNC0},  /* click TXD2 UART mikrobus2; P0.10 GPIO TXD2      SDA2      MAT3.0   */
//	{0, 11,   IOCON_MODE_INACT    | IOCON_FUNC0},  /* click RXD2 UART mikrobus2; P0.11 GPIO RXD2      SCL2      MAT3.1   */
	{0, 15,   IOCON_MODE_PULLDOWN | IOCON_FUNC2},  /* greenPHY SCK               P0.15 GPIO TXD1      SCK0      SCK      */
	{0, 16,   IOCON_MODE_PULLUP   | IOCON_FUNC0},  /* greenPHY SSEL as GPIO      P0.16 GPIO RXD1      SSEL0     SSEL     */
	{0, 17,   IOCON_MODE_INACT    | IOCON_FUNC2},  /* greenPHY MISO              P0.17 GPIO CTS1      MISO0     MISO     */
	{0, 18,   IOCON_MODE_INACT    | IOCON_FUNC2},  /* greenPHY MOSI              P0.18 GPIO DCD1      MOSI0     MOSI     */
//	{0, 19,   IOCON_MODE_INACT    | IOCON_FUNC0},  /*                Not available on 80-pin package.                    */
//	{0, 20,   IOCON_MODE_INACT    | IOCON_FUNC0},  /*                Not available on 80-pin package.                    */
//	{0, 21,   IOCON_MODE_INACT    | IOCON_FUNC0},  /*                Not available on 80-pin package.                    */
	{0, 22,   IOCON_MODE_INACT    | IOCON_FUNC0},  /* greenPHY GPIO Interrupt    P0.22 GPIO RTS1      Reserved  TD1      */
//	{0, 23,   IOCON_MODE_INACT    | IOCON_FUNC0},  /*                Not available on 80-pin package.                    */
//	{0, 24,   IOCON_MODE_INACT    | IOCON_FUNC0},  /*                            Not available on 80-pin package.        */
//	{0, 25,   IOCON_MODE_INACT    | IOCON_FUNC0},  /* click ANA mikrobus2;       P0.25 GPIO AD0.2     I2SRX_SDA TXD3     */
//	{0, 26,   IOCON_MODE_INACT    | IOCON_FUNC0},  /* GPP PIN2 J12;              P0.26 GPIO AD0.3     AOUT      RXD3     */
//	{0, 27,   IOCON_MODE_INACT    | IOCON_FUNC0},  /*                Not available on 80-pin package.                    */
//	{0, 28,   IOCON_MODE_INACT    | IOCON_FUNC0},  /*                Not available on 80-pin package.                    */
//	{0, 29,   IOCON_MODE_INACT    | IOCON_FUNC0},  /* evalboard USB+             P0.29 GPIO USB_D     Reserved  Reserved */
//	{0, 30,   IOCON_MODE_INACT    | IOCON_FUNC0},  /* evalboard USB-             P0.30 GPIO USB_D     Reserved  Reserved */

	/*  PORT1   */
	{1, 0,  IOCON_MODE_INACT | IOCON_FUNC1},    /* ENET_TXD0                  P1.00 GPIO ENET_TXD0    Reserved Reserved */
	{1, 1,  IOCON_MODE_INACT | IOCON_FUNC1},    /* ENET_TXD1                  P1.01 GPIO ENET_TXD1    Reserved Reserved */
	{1, 4,  IOCON_MODE_INACT | IOCON_FUNC1},    /* ENET_TX_EN                 P1.04 GPIO ENET_TX_EN   Reserved Reserved */
	{1, 8,  IOCON_MODE_INACT | IOCON_FUNC1},    /* ENET_CRS                   P1.08 GPIO ENET_CRS     Reserved Reserved */
	{1, 9,  IOCON_MODE_INACT | IOCON_FUNC1},    /* ENET_RXD0                  P1.09 GPIO ENET_RXD0    Reserved Reserved */
	{1, 10, IOCON_MODE_INACT | IOCON_FUNC1},    /* ENET_RXD1                  P1.10 GPIO ENET_RXD1    Reserved Reserved */
	{1, 14, IOCON_MODE_INACT | IOCON_FUNC1},    /* ENET_RX_ER                 P1.14 GPIO ENET_RX_ER   Reserved Reserved */
	{1, 15, IOCON_MODE_INACT | IOCON_FUNC1},    /* ENET_REF_CLK               P1.15 GPIO ENET_REF_CLK Reserved Reserved */
//	{1, 16, IOCON_MODE_INACT | IOCON_FUNC0},    /*                Not available on 80-pin package.                    */
//	{1, 17, IOCON_MODE_INACT | IOCON_FUNC0},    /*                Not available on 80-pin package.                    */
//	{1, 18, IOCON_MODE_INACT | IOCON_FUNC0},    /* not used                   P1.18 GPIO USB_UP_LED PWM1.1    CAP1.0  */
//	{1, 19, IOCON_MODE_INACT | IOCON_FUNC0},    /* not used                   P1.19 GPIO MCOA0      USB_PPWR  CAP1.1  */
	{1, 20, IOCON_MODE_INACT | IOCON_FUNC0},    /* ENET_RMII_MDC on GPIO      P1.20 GPIO MCI0       PWM1.2    SCK0    */
//	{1, 21, IOCON_MODE_INACT | IOCON_FUNC0},    /*                Not available on 80-pin package.                    */
//	{1, 22, IOCON_MODE_INACT | IOCON_FUNC0},    /* not used                   P1.22 GPIO MCOB0      USB_PWRD  MAT1.0  */
	{1, 23, IOCON_MODE_INACT | IOCON_FUNC0},    /* ENET_RMII_MDIO on GPIO     P1.23 GPIO MCI1       PWM1.4    MISO0   */
	{1, 24, IOCON_MODE_INACT | IOCON_FUNC0},    /* ENET_RMII_RESET on GPIO    P1.24 GPIO MCI2       PWM1.5    MOSI0   */
//	{1, 25, IOCON_MODE_INACT | IOCON_FUNC0},    /* not used                   P1.25 GPIO MCOA1      Reserved  MAT1.1  */
//	{1, 26, IOCON_MODE_INACT | IOCON_FUNC0},    /* RST mikrobus1 on GPIO      P1.26 GPIO MCOB1      PWM1.6    CAP0.0  */
//	{1, 27, IOCON_MODE_INACT | IOCON_FUNC1},    /*                Not available on 80-pin package.                    */
//	{1, 28, IOCON_MODE_INACT | IOCON_FUNC0},    /* RST mikrobus2 on GPIO      P1.28 GPIO MCOA2      PCAP1.0   MAT0.0  */
//	{1, 29, IOCON_MODE_INACT | IOCON_FUNC0},    /* not used                   P1.29 GPIO MCOB2      PCAP1.1   MAT0.1  */
//	{1, 30, IOCON_MODE_INACT | IOCON_FUNC0},    /* USB VBUS on evalboard      P1.30 GPIO Reserved   VBUS      AD0.4   */
//	{1, 31, IOCON_MODE_INACT | IOCON_FUNC0},    /* ANA mikrobus1 on GPIO      P1.31 GPIO Reserved   SCK1      AD0.5   */

	/*  PORT2   */
//	{2, 0,  IOCON_MODE_INACT | IOCON_FUNC0},    /* evalboard TXD2 UART mikrobus1   P2.00 GPIO PWM1.1      TXD1 Reserved */
//	{2, 1,  IOCON_MODE_INACT | IOCON_FUNC0},    /* evalboard RXD2 UART mikrobus1   P2.01 GPIO PWM1.2      RXD1 Reserved */
//	{2, 2,  IOCON_MODE_INACT | IOCON_FUNC0},    /* CS on mikrobus1 GPIO            P2.02 GPIO PWM1.3      CTS1 Reserved */
//	{2, 3,  IOCON_MODE_INACT | IOCON_FUNC0},    /* INT mikrobus1 on GPIO           P2.03 GPIO PWM1.4      DCD1 Reserved */
//	{2, 4,  IOCON_MODE_INACT | IOCON_FUNC0},    /* PWM mikrobus1 on GPIO           P2.04 GPIO PWM1.5      DSR1 Reserved */
//	{2, 5,  IOCON_MODE_INACT | IOCON_FUNC0},    /* PWM mikrobus2 on GPIO           P2.05 GPIO PWM1.6      DTR1 Reserved */
//	{2, 6,  IOCON_MODE_INACT | IOCON_FUNC0},    /* Int mikrobus2 on GPIO           P2.06 GPIO PCAP1.0     RI1  Reserved */
//	{2, 7,  IOCON_MODE_INACT | IOCON_FUNC0},    /* CS mikrobus2 on GPIO            P2.07 GPIO RD2         RTS1 Reserved */
//	{2, 8,  IOCON_MODE_INACT | IOCON_FUNC0},    /* not used                        P2.08 GPIO TD2         TXD2 ENET_MDC */
//	{2, 9,  IOCON_MODE_INACT | IOCON_FUNC0},    /* not used                        P2.09 GPIO USB_CONNECT RXD2 ENET_MDIO */
//	{2,10,  IOCON_MODE_INACT | IOCON_FUNC0},    /* MINT onboard Button on GPIO     P2.10 GPIO EINT0 NMI Reserved */
//	{2,11,  IOCON_MODE_INACT | IOCON_FUNC0},    /*                Not available on 80-pin package.                    */
//	{2,12,  IOCON_MODE_INACT | IOCON_FUNC0},    /*                Not available on 80-pin package.                    */
//	{2,13,  IOCON_MODE_INACT | IOCON_FUNC0},    /*                Not available on 80-pin package.                    */
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
