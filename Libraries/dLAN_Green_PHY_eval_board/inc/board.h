/*
 * @brief devolo dLAN Green PHY Module eval board
 *
 * @note
 * Copyright(C) devolo AG, 2016
 * All rights reserved.
 *
 *
 *
 */

#ifndef __BOARD_H_
#define __BOARD_H_

#include "chip.h"

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
#define DEBUG_ENABLE

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
#define LEDS_LED1           0x01
//#define LEDS_LED2           0x02 //not used on devolo evaluation board
//#define LEDS_LED3           0x04 //not used on devolo evaluation board
//#define LEDS_LED4           0x08 //not used on devolo evaluation board
#define LEDS_NO_LEDS        0x00

/**
 * Button defines
 */
#define BUTTONS_BUTTON1     0x01
#define NO_BUTTON_PRESSED   0x00

/**
 * Joystick defines
 */
//#define JOY_UP              0x01 //not used on devolo evaluation board
//#define JOY_DOWN            0x02 //not used on devolo evaluation board
//#define JOY_LEFT            0x04 //not used on devolo evaluation board
//#define JOY_RIGHT           0x08 //not used on devolo evaluation board
//#define JOY_PRESS           0x10 //not used on devolo evaluation board

/**
 * Dummy audio input selection values enum
 */
//#define MCB_17XX_AUDIO_MIC_SELECT       0x00 //not used on devolo evaluation board
//#define MCB_17XX_AUDIO_LINE_IN_SELECT   0x00 //not used on devolo evaluation board

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
 * @brief	Sets up board specific I2S interface and UDA1380 CODEC
 * @param	pI2S	: I2S peripheral to use (Must be LPC_I2S)
 * @param	micIn	: If 1 MIC will be used as input device, if 0
 *          LINE_IN will be used as input to Audio Codec.
 * @return	Nothing
 */
//void Board_Audio_Init(LPC_I2S_T *pI2S, int micIn);  //not used on devolo evaluation board

/**
 * @brief	Initialize pin muxing for SSP interface
 * @param	pSSP	: Pointer to SSP interface to initialize
 * @return	Nothing
 */
void Board_SSP_Init(LPC_SSP_T *pSSP);

/**
 * @brief	Initialize pin muxing for SPI interface
 * @param	isMaster	: true for master mode, false for slave mode
 * @return	Nothing
 */
void Board_SPI_Init(bool isMaster);

/**
 * @brief	Assert SSEL pin
 * @return	Nothing
 */
void Board_SPI_AssertSSEL(void);

/**
 * @brief	De-assert SSEL pin
 * @return	Nothing
 */
void Board_SPI_DeassertSSEL(void);

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
 * @brief	Initialize Joystick
 * @return	Nothing
 */
//void Board_Joystick_Init(void); //not used on devolo evaluation board

/**
 * @brief	Get Joystick status
 * @return	status of Joystick
 */
//uint8_t Joystick_GetStatus(void); //not used on devolo evaluation board

/**
 * @brief	Create Serial Stream
 * @param	Stream	: Pointer to stream
 * @return	Nothing
 */
void Serial_CreateStream(void *Stream);

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

#define LPC_USART0 0


#include "board_api.h"
#include "lpc_phy.h"

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H_ */
