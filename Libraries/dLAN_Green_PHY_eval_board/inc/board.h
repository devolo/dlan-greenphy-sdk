/*
 * Copyright (c) 2017, devolo AG, Aachen, Germany.
 * All rights reserved.
 *
 * This Software is part of the devolo GreenPHY-SDK.
 *
 * Usage in source form and redistribution in binary form, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Usage in source form is subject to a current end user license agreement
 *    with the devolo AG.
 * 2. Neither the name of the devolo AG nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 3. Redistribution in binary form is limited to the usage on the GreenPHY
 *    module of the devolo AG.
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

#ifndef __BOARD_H_
#define __BOARD_H_

#include "chip.h"

/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup BOARD_dLAN_Green_PHY_Module devolo LPC1758 board software API functions
 * @ingroup BOARD_dLAN_Green_PHY_Module
 * The board support software API functions provide some simple abstracted
 * functions used across multiple LPCOpen board examples. See @ref BOARD_COMMON_API
 * for the functions defined by this board support layer.<br>
 * @{
 */

/** @defgroup BOARD_dLAN_Green_PHY_Module BOARD: devolo LPC1758 board build options
 * This board has options that configure its operation at build-time.<br>
 * @{
 */

/** Define DEBUG_ENABLE to enable IO via the DEBUGSTR, DEBUGOUT, and
    DEBUGIN macros. If not defined, DEBUG* functions will be optimized
    out of the code at build time.
 */
#ifdef DEBUG
	#define DEBUG_ENABLE
#endif

/** Define DEBUG_SEMIHOSTING along with DEBUG_ENABLE to enable IO support
    via semihosting. You may need to use a C library that supports
    semihosting with this option.
 */
// #define DEBUG_SEMIHOSTING

/** Board UART used for debug output and input using the DEBUG* macros. This
    is also the port used for Board_UARTPutChar, Board_UARTGetChar, and
    Board_UARTPutSTR functions.
 */
#define DEBUG_UART LPC_UART0

/**
 * @}
 */

/* Board name */
#define BOARD_dLAN_Green_PHY_Module

#define USE_RMII

/**
 * LED defines
 */
#define USR_LED_ON false
#define USR_LED_OFF true
#define LEDS_LED0           0x01
#define LEDS_NO_LEDS        0x00

/**
 * Button defines
 */
#define BUTTONS_BUTTON1     0x01
#define NO_BUTTON_PRESSED   0x00

/**
 * Clickboard defines
 */
typedef enum ClickboardPorts {
	eClickboardInactive = 0x00,
	eClickboardPort1    = 0x01,
	eClickboardPort2    = 0x02,
	eClickboardAllPorts = ( eClickboardPort1 | eClickboardPort2 )
} ClickboardPorts_t;

#define BOARD_UNUSED 0
#define SLOT1 1
#define SLOT2 2

#define LED0_GPIO_PORT_NUM                        0
#define LED0_GPIO_BIT_NUM                         6

#define BUTTONS_BUTTON1_GPIO_PORT_NUM             2
#define BUTTONS_BUTTON1_GPIO_BIT_NUM              10

#define CLICKBOARD1_CS_GPIO_PORT_NUM              2
#define CLICKBOARD1_CS_GPIO_BIT_NUM               2

#define CLICKBOARD1_INT_GPIO_PORT_NUM             2
#define CLICKBOARD1_INT_GPIO_BIT_NUM              3

#define CLICKBOARD1_PWM_GPIO_PORT_NUM             2
#define CLICKBOARD1_PWM_GPIO_BIT_NUM              4

#define CLICKBOARD1_RST_GPIO_PORT_NUM             1
#define CLICKBOARD1_RST_GPIO_BIT_NUM              26

#define CLICKBOARD1_TX_GPIO_PORT_NUM              2
#define CLICKBOARD1_TX_GPIO_BIT_NUM               0

#define CLICKBOARD1_RX_GPIO_PORT_NUM              2
#define CLICKBOARD1_RX_GPIO_BIT_NUM               1

#define CLICKBOARD1_AN_GPIO_PORT_NUM              1
#define CLICKBOARD1_AN_GPIO_BIT_NUM               31

#define CLICKBOARD2_CS_GPIO_PORT_NUM              2
#define CLICKBOARD2_CS_GPIO_BIT_NUM               7

#define CLICKBOARD2_INT_GPIO_PORT_NUM             2
#define CLICKBOARD2_INT_GPIO_BIT_NUM              6

#define CLICKBOARD2_PWM_GPIO_PORT_NUM             2
#define CLICKBOARD2_PWM_GPIO_BIT_NUM              5

#define CLICKBOARD2_RST_GPIO_PORT_NUM             1
#define CLICKBOARD2_RST_GPIO_BIT_NUM              28

#define CLICKBOARD2_TX_GPIO_PORT_NUM              0
#define CLICKBOARD2_TX_GPIO_BIT_NUM               10

#define CLICKBOARD2_RX_GPIO_PORT_NUM              0
#define CLICKBOARD2_RX_GPIO_BIT_NUM               11

#define CLICKBOARD2_AN_GPIO_PORT_NUM              0
#define CLICKBOARD2_AN_GPIO_BIT_NUM               25

#define CLICKBOARD_SCK_SPI1_PORT_NUM              0
#define CLICKBOARD_SCK_SPI1_BIT_NUM               7

#define CLICKBOARD_MISO_SPI1_PORT_NUM             0
#define CLICKBOARD_MISO_SPI1_BIT_NUM              8

#define CLICKBOARD_MOSI_SPI1_PORT_NUM             0
#define CLICKBOARD_MOSI_SPI1_BIT_NUM              9

#define CLICKBOARD1_CS_SPI1_PORT_NUM              2
#define CLICKBOARD1_CS_SPI1_BIT_NUM               2

#define CLICKBOARD2_CS_SPI1_PORT_NUM              2
#define CLICKBOARD2_CS_SPI1_BIT_NUM               7

#define CLICKBOARD_SDA_I2C1_PORT_NUM              0
#define CLICKBOARD_SDA_I2C1_BIT_NUM               0

#define CLICKBOARD_SCL_I2C1_PORT_NUM              0
#define CLICKBOARD_SCL_I2C1_BIT_NUM               1

#define CLICKBOARD2_SDA_I2C2_PORT_NUM             0
#define CLICKBOARD2_SDA_I2C2_BIT_NUM              10

#define CLICKBOARD2_SCL_I2C2_PORT_NUM             0
#define CLICKBOARD2_SCL_I2C2_BIT_NUM              11

/*Peripherals*/
#define J12_GPP_GPIO_PORT_NUM                     0
#define J12_GPP_GPIO_BIT_NUM                      26

#define J10_PIN4_TX_GPIO_PORT_NUM                 0
#define J10_PIN4_TX_GPIO_BIT_NUM                  3

#define J10_PIN5_RX_GPIO_PORT_NUM                 0
#define J10_PIN5_RX_GPIO_BIT_NUM                  2

#define USB_PLUS_RX_GPIO_PORT_NUM                 0
#define USB_PLUS_RX_GPIO_BIT_NUM                  29

#define USB_MINUS_RX_GPIO_PORT_NUM                0
#define USB_MINUS_RX_GPIO_BIT_NUM                 30

#define POSITIVE_USB_PORT_NUM                     0
#define POSITIVE_USB_BIT_NUM                      30

#define NEGATIVE_USB_PORT_NUM                     0
#define NEGATIVE_USB_BIT_NUM                      29

#define VBUS_USB_PORT_NUM                         1
#define VBUS_USB_BIT_NUM                          30

/**
 * SSP defines
 */
#define SSP0_SCK_PORT                             0
#define SSP0_SCK_PIN                              15

#define SSP0_SSEL_PORT                            0
#define SSP0_SSEL_PIN                             16

#define SSP0_MISO_PORT                            0
#define SSP0_MISO_PIN                             17

#define SSP0_MOSI_PORT                            0
#define SSP0_MOSI_PIN                             18

#define SSP0_INT_PORT                             0
#define SSP0_INT_PIN                              22

/**
 * GreenPHY defines
 */
#define GREENPHY_SCK_PORT                         SSP0_SCK_PORT
#define GREENPHY_SCK_PIN                          SSP0_SCK_PIN

#define GREENPHY_SSEL_PORT                        SSP0_SSEL_PORT
#define GREENPHY_SSEL_PIN                         SSP0_SSEL_PIN

#define GREENPHY_MISO_PORT                        SSP0_MISO_PORT
#define GREENPHY_MISO_PIN                         SSP0_MISO_PIN

#define GREENPHY_MOSI_PORT                        SSP0_MOSI_PORT
#define GREENPHY_MOSI_PIN                         SSP0_MOSI_PIN

#define GREENPHY_INT_PORT                         SSP0_INT_PORT
#define GREENPHY_INT_PIN                          SSP0_INT_PIN

#define GREENPHY_RESET_GPIO_PORT                  1
#define GREENPHY_RESET_GPIO_PIN                   29

/*onboard Ethernet interface*/
#define MDC_GPIO_PORT_NUM                         1
#define MDC_GPIO_BIT_NUM                          20

#define MDIO_GPIO_PORT_NUM                        1
#define MDIO_GPIO_BIT_NUM                         23

/* Mutex to secure access to I2C1 interface. */
extern SemaphoreHandle_t xI2C1_Mutex;

/**
 * @brief	Initialize pin muxing for a UART
 * @param	pUART	: Pointer to UART register block for UART pins to init
 * @return	Nothing
 */
void Board_UART_Init(LPC_USART_T *pUART);

/**
 * @brief	Returns the MAC address assigned to this board
 * @param	mcaddr : Pointer to 6-byte character array to populate with MAC address
 * @return	Nothing
 * @note    Returns the MAC address used by Ethernet
 */
void Board_ENET_GetMacADDR(uint8_t *mcaddr);

/**
 * @brief	Initialize pin muxing for SSP interface
 * @param	pSSP	: Pointer to SSP interface to initialize
 * @return	Nothing
 */
void Board_SSP_Init(LPC_SSP_T *pSSP, bool isMaster);

/**
 * @brief	Assert SSEL pin
 * @param	pSSP	: Pointer to SSP interface to assert
 * @return	previous SSEL state
 */
bool Board_SSP_AssertSSEL(LPC_SSP_T *pSSP);

/**
 * @brief	De-assert SSEL pin
 * @param	pSSP	: Pointer to SSP interface to deassert
 * @return	previous SSEL state
 */
bool Board_SSP_DeassertSSEL(LPC_SSP_T *pSSP);

/**
 * @brief	Sets up board specific I2C interface
 * @param	id	: ID of I2C peripheral
 * @return	Nothing
 */
void Board_I2C_Init(I2C_ID_T id);

/**
 * @brief	Sets up I2C Fast Plus mode
 * @param	id	: Must always be I2C0
 * @return	Nothing
 * @note	This function must be called before calling
 *          Chip_I2C_SetClockRate() to set clock rates above
 *          normal range 100KHz to 400KHz. Only I2C0 supports
 *          this mode.
 */
STATIC INLINE void Board_I2C_EnableFastPlus(I2C_ID_T id)
{
	Chip_IOCON_SetI2CPad(LPC_IOCON, I2CPADCFG_FAST_MODE_PLUS);
}

/**
 * @brief	Disables I2C Fast plus mode and enable normal mode
 * @param	id	: Must always be I2C0
 * @return	Nothing
 */
STATIC INLINE void Board_I2C_DisableFastPlus(I2C_ID_T id)
{
	Chip_IOCON_SetI2CPad(LPC_IOCON, I2CPADCFG_STD_MODE);
}

/**
 * @brief	Initialize buttons on the board
 * @return	Nothing
 */
void Board_Buttons_Init(void);

/**
 * @brief	Get button status
 * @return	status of button
 */
uint32_t Buttons_GetStatus(void);

/**
 * @brief	Initializes USB device mode pins per board design
 * @param	port	: USB port to be enabled 
 * @return	Nothing
 * @note	Only one of the USB port can be enabled at a given time.
 */
void Board_USBD_Init(uint32_t port);

/**
 * @}
 */

/* GreenPHY Board Support includes */
#include "board_api.h"
#include "lpc_phy.h"
#include "lpc_dma.h"
#include "lpc_gpio_interrupt.h"
#include "byteorder.h"

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H_ */
