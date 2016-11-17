/*
 * Copyright (c) 2012, devolo AG, Aachen, Germany.
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
 * debug.h
 *
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#include <lpc_types.h>

/*-------------------------------PORT-DEFINITIONS------------------------------------*/

#define UART0 0
#define UART1 1
#define UART2 2
#define UART3 3

#ifdef DEBUG

/*----------------------------BASIC-LEVEL-DEFINITIONS-------------------------------*/

#define NOTHING					(0x0)
#define GREEN_PHY_RX 			(0x1<<0)
#define GREEN_PHY_TX 			(0x1<<1)
#define ETHERNET_RX  			(0x1<<2)
#define ETHERNET_TX  			(0x1<<3)
#define GREEN_PHY_RX_BINARY 	(0x1<<4)
#define GREEN_PHY_TX_BINARY 	(0x1<<5)
#define ETHERNET_RX_BINARY  	(0x1<<6)
#define ETHERNET_TX_BINARY  	(0x1<<7)
#define GREEN_PHY_INTERUPT 		(0x1<<8)
#define ETHERNET_INTERUPT 		(0x1<<9)
#define DEBUG_PANIC      		(0x1<<10)
#define DEBUG_EMERG	          	(0x1<<11)
#define DEBUG_ALERT	          	(0x1<<12)
#define DEBUG_CRIT	          	(0x1<<13)
#define DEBUG_ERR	          	(0x1<<14)
#define DEBUG_WARNING         	(0x1<<15)
#define DEBUG_NOTICE          	(0x1<<16)
#define DEBUG_INFO	          	(0x1<<17)
#define DEBUG_TASK	          	(0x1<<18)
#define DEBUG_ERROR_SEARCH     	(0x1<<19)
#define DMA_INTERUPT 			(0x1<<20)
#define CLI						(0x1<<21)
#define GPIO_INTERUPT 			(0x1<<22)
#define ETHERNET_FLOW_CONTROL 	(0x1<<23)
#define GREEN_PHY_FRAME_SYNC_STATUS 	(0x1<<24)
#define GREEN_PHY_SYNC_STATUS	(0x1<<25)
#define DEBUG_BUFFER			(0x1<<26)
#define GREEN_PHY_FW_FEATURES	(0x1<<27)
#define DEBUG_ALL 				(0xffffffff)

/*-------------------------------COMPUND-DEFINITIONS----------------------------------*/

#define DEBUG_INTERRUPTS		(GPIO_INTERUPT|DMA_INTERUPT|ETHERNET_INTERUPT|GREEN_PHY_INTERUPT)
#define DEBUG_BINARY			(GREEN_PHY_RX_BINARY|GREEN_PHY_TX_BINARY|ETHERNET_RX_BINARY|ETHERNET_TX_BINARY)
#define DEBUG_GREEN_PHY			(GREEN_PHY_RX|GREEN_PHY_TX|GREEN_PHY_RX_BINARY|GREEN_PHY_TX_BINARY|GREEN_PHY_INTERUPT|GREEN_PHY_FRAME_SYNC_STATUS|GREEN_PHY_SYNC_STATUS)
#define DEBUG_DATA_FLOW			(GREEN_PHY_RX|GREEN_PHY_TX|ETHERNET_RX|ETHERNET_TX)

/*********************************************************************//**
 * @brief 		Initialization routine for the debug print system
 * @param[in] 	uartPort		used for debug
 * @return 		-
 ***********************************************************************/
void debug_init(uint32_t uartPort);

/*********************************************************************//**
 * @brief 		Print out a debug string
 * @param[in] 	level		to be used for printing
 * @param[in]	format		to print, followed by variable argument list
 * @return 		-
 ***********************************************************************/
void debug_print(int level,const char * __restrict format, ...);

/*********************************************************************//**
 * @brief 		Dump memory
 * @param[in] 	level		to be used for dumping
 * @param[in] 	mem			address to start dumping
 * @param[in] 	size		to be dumped
 * @param[in] 	message		to be displayed
 * @return 		-
 ***********************************************************************/
void dump (const int level, constData_t mem, length_t size, const char const * message);

/*********************************************************************//**
 * @brief 		Set the current debug level
 * @param[in] 	level		to be used
 * @return 		-
 ***********************************************************************/
void debug_level_set(uint32_t level);

/*********************************************************************//**
 * @brief 		Get the current debug level
 * @return 		current debug level
 ***********************************************************************/
uint32_t debug_level_get(void);

/*********************************************************************//**
 * @brief 		dumps only IcmpFrames, print out the sequence number
 * @param[in] 	level		to be used for dumping
 * @param[in] 	frame		address of the frame to dump
 * @param[in] 	length		to be dumped
 * @param[in] 	message		to be displayed
 * @return 		-
 ***********************************************************************/
void dumpIcmpFrame(const int level, data_t frame, length_t length, char * message);

/*------------------------------DEBUG-DEFINITIONS------------------------*/

/*********************************************************************//**
 * Use only the following defines, so the praeprocessor can remove all
 * debugs ....
 ***********************************************************************/

#define DEBUG_INIT(X) debug_init(X);
#define DEBUG_PRINT(LEVEL, STRING, args...) debug_print(LEVEL, STRING, ##args)
#define DEBUG_DUMP(LEVEL,MEM,SIZE,MESSAGE) dump(LEVEL,MEM,SIZE,MESSAGE)
//#define DEBUG_DUMP_ICMP_FRAME(LEVEL,FRAME,SIZE,MESSAGE) dumpIcmpFrame(LEVEL,FRAME,SIZE,MESSAGE)
#define DEBUG_DUMP_ICMP_FRAME(LEVEL,FRAME,SIZE,MESSAGE)
#define DEBUG_LEVEL_SET(X) debug_level_set(X);
#define DEBUG_LEVEL_GET() debug_level_get();
#define DEBUG_EXECUTE(X) X;

#else

#define DEBUG_INIT(X)
#define DEBUG_PRINT(LEVEL, STRING, args...)
#define DEBUG_DUMP(LEVEL,MEM,SIZE,MESSAGE)
#define DEBUG_DUMP_ICMP_FRAME(LEVEL,FRAME,SIZE,MESSAGE)
#define DEBUG_LEVEL_SET(X)
#define DEBUG_LEVEL_GET()
#define DEBUG_EXECUTE(X)

#endif

/*********************************************************************//**
 * @brief 		Initialization routine for the print system
 * @param[in] 	uartPort		used for printing
 * @return 		-
 ***********************************************************************/
void printInit(uint32_t uartPort);

/*********************************************************************//**
 * @brief 		Print out a string
 * @param[in]	format		to print, followed by variable argument list
 * @return 		-
 ***********************************************************************/
void printToUart(const char * __restrict format, ...);

#endif /* DEBUG_H_ */
