/*
    FreeRTOS V7.1.0 - Copyright (C) 2011 Real Time Engineers Ltd.


    ***************************************************************************
     *                                                                       *
     *    FreeRTOS tutorial books are available in pdf and paperback.        *
     *    Complete, revised, and edited pdf reference manuals are also       *
     *    available.                                                         *
     *                                                                       *
     *    Purchasing FreeRTOS documentation will not only help you, by       *
     *    ensuring you get running as quickly as possible and with an        *
     *    in-depth knowledge of how to use FreeRTOS, it will also help       *
     *    the FreeRTOS project to continue with its mission of providing     *
     *    professional grade, cross platform, de facto standard solutions    *
     *    for microcontrollers - completely free of charge!                  *
     *                                                                       *
     *    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
     *                                                                       *
     *    Thank you for using FreeRTOS, and thank you for your support!      *
     *                                                                       *
    ***************************************************************************


    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    >>>NOTE<<< The modification to the GPL is included to allow you to
    distribute a combined work that includes FreeRTOS without being obliged to
    provide the source code for proprietary components outside of the FreeRTOS
    kernel.  FreeRTOS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/

/* Originally adapted from file written by Andreas Dannenberg.  Supplied with permission. */

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
 * emac.c
 *
 */

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"

#include "emac.h"
#include "emacLPC17xx.h"
#include "debug.h"

#include "string.h"
#include "busyWait.h"
#include "interuptHandlerGPIO.h"
#include "greenPhyModuleConfig.h"

#define INIT_DELAY_CALL busyWaitMs

//#define TEST_PAUSE_FRAMES
//#define PASS_AND_PRINT_CONTROL_FRAMES

/*-----------------------------------------------------------*/

/* How long to wait in ms before attempting to init the MAC again. */
#define emacINIT_WAIT    ( 100 )

const signed char * const ethAutoNegCheckName = (const signed char * const) "EthAutoNegCheck";

struct emacNetdevice {
	/* device interface structure at first */
	struct netDeviceInterface dev;
	/* used to show that the autonegotiation guard is active and watching  */
	int ethAutoNegGuard;
	/* The semaphore used to wake the open() after autonegotiation completed. */
	xSemaphoreHandle xEMACAutoNegotiationSemaphore;
	/* The semaphore used to wake the listening task when data arrives. */
	xSemaphoreHandle xEMACSemaphore;
#if ETHERNET_LPC1758_FLOW_CONTROL  == ON
	int useEthFlowControl;
#endif
	int initialised;

	/* the TX queue, simply a FreeRTOS queue */
	xQueueHandle txQueue;
	/* the RX queue is implemented in the LPC1758 HW */

	/* storage to hold the buffer pointers to be freed after tx int */
	void * pBuffer[NUM_TX_FRAG];

};

static struct emacNetdevice eth0 = {
		.dev = {0},
		.ethAutoNegGuard = 0,
		.xEMACAutoNegotiationSemaphore = NULL,
		.xEMACSemaphore = NULL,
#if ETHERNET_LPC1758_FLOW_CONTROL  == ON
		.useEthFlowControl = 0,
#endif
		.initialised = 0,
		.txQueue = NULL
} ;

// functions for the lpc17xx
static unsigned int input_MDIO (void);
static void turnaround_MDIO (void);
static void output_MDIO (unsigned int val, unsigned int n);

/*-----------------------------------------------------------*/

static long lEMACInit( void )
{
	long lReturn = pdPASS;
	unsigned long ulID1, ulID2;

	/* Reset peripherals, configure port pins and registers. */
	prvSetupEMACHardware();

	/* Check the PHY part number is as expected. */
	ulID1 = prvReadPHY( PHY_REG_IDR1, &lReturn );
	ulID2 = prvReadPHY( PHY_REG_IDR2, &lReturn );
	DEBUG_PRINT(DEBUG_NOTICE,"PHY ID 0x%x\r\n", ( (ulID1 << 16UL ) | ( ulID2 & 0xFFF0UL ) ));
	if( ( (ulID1 << 16UL ) | ( ulID2 & 0xFFF0UL ) ) == LAN_8710i_ID )
	{
		/* Set the Ethernet MAC Address registers */
		LPC_EMAC->SA0 = ( configMAC_ADDR0 << 8 ) | configMAC_ADDR1;
		LPC_EMAC->SA1 = ( configMAC_ADDR2 << 8 ) | configMAC_ADDR3;
		LPC_EMAC->SA2 = ( configMAC_ADDR4 << 8 ) | configMAC_ADDR5;

		/* Initialize Tx and Rx DMA Descriptors */
		prvInitDescriptors();

		/* Receive broadcast, multicast and unicast packets (promiscuous) */
		LPC_EMAC->RxFilterCtrl = RFC_UCAST_EN | RFC_BCAST_EN | RFC_MCAST_EN;

#if ETHERNET_LPC1758_FLOW_CONTROL  == ON
		/* Configure AutoNegotiationAdvertisement. */
		unsigned long ulANAR = prvReadPHY( PHY_REG_ANAR, &lReturn );
		ulANAR |= ( PHY_AUTO_NEG_PAUSE_SYM | PHY_AUTO_NEG_PAUSE_ASYM );
		DEBUG_PRINT(DEBUG_NOTICE|ETHERNET_FLOW_CONTROL,"ETH flow control on -> PHY new ANAR 0x%x\r\n", ulANAR);
		prvWritePHY( PHY_REG_ANAR, (ulANAR));
#endif

		/* Setup the interupt handling. */
		prvWritePHY( PHY_REG_INT_MASK, (PHY_INT_AUTO_NEG_COMPLETE));

		/* Setup auto negotiation. */
		prvWritePHY( PHY_REG_BMCR, PHY_AUTO_NEG );

		/* Reset all interrupts */
		LPC_EMAC->IntClear = ( INT_RX_OVERRUN | INT_RX_ERR | INT_RX_FIN | INT_RX_DONE | INT_TX_UNDERRUN | INT_TX_ERR | INT_TX_FIN | INT_TX_DONE | INT_SOFT_INT | INT_WAKEUP );

		/* Enable receive and transmit mode of MAC Ethernet core */
		LPC_EMAC->Command |= ( CR_RX_EN | CR_TX_EN );
		LPC_EMAC->MAC1 |= MAC1_REC_EN;
	}
	else
	{
		lReturn = pdFAIL;
		DEBUG_PRINT(DEBUG_NOTICE,"Wrong PHY ID\r\n");
	}

	return lReturn;
}
/*-----------------------------------------------------------*/

static void prvInitDescriptors( void )
{
	long x;

	static int initialised = 0;
	if (!initialised)
	{
		initialised = 1;
		for( x = 0; x < NUM_RX_FRAG; x++ )
		{
			struct netdeviceQueueElement * tmp = getQueueElement(); //  |
			DEBUG_PRINT(DEBUG_BUFFER,"[#I1#]\r\n");

			/* Allocate the next Ethernet buffer to this descriptor. */
			RX_DESC_PACKET( x ) = (int) removeDataFromQueueElement(&tmp);;
			RX_DESC_CTRL( x ) = RCTRL_INT | ( ETH_FRAG_SIZE - 1 );
			RX_STAT_INFO( x ) = 0;
			RX_STAT_HASHCRC( x ) = 0;
		}

		/* Set EMAC Receive Descriptor Registers. */
		LPC_EMAC->RxDescriptor = RX_DESC_BASE;
		LPC_EMAC->RxStatus = RX_STAT_BASE;
		LPC_EMAC->RxDescriptorNumber = NUM_RX_FRAG - 1;

		/* Rx Descriptors Point to 0 */
		LPC_EMAC->RxConsumeIndex = 0;

		/* A buffer is not allocated to the Tx descriptors until they are actually
		used. */
		for( x = 0; x < NUM_TX_FRAG; x++ )
		{
			TX_DESC_PACKET( x ) = ( unsigned long ) NULL;
			TX_DESC_CTRL( x ) = 0;
			TX_STAT_INFO( x ) = 0;
		}

		/* Set EMAC Transmit Descriptor Registers. */
		LPC_EMAC->TxDescriptor = TX_DESC_BASE;
		LPC_EMAC->TxStatus = TX_STAT_BASE;
		LPC_EMAC->TxDescriptorNumber = NUM_TX_FRAG - 1;

		/* Tx Descriptors Point to 0 */
		LPC_EMAC->TxProduceIndex = 0;
	}
}
/*-----------------------------------------------------------*/

static void prvSetupEMACHardware( void )
{
	unsigned short us;
	long x, lDummy;

	/* Enable P1 Ethernet Pins. */
	LPC_PINCON->PINSEL2 = emacPINSEL2_VALUE;
	LPC_PINCON->PINSEL4 &= ~0x000F0000;
	LPC1758_MDIO_PORT->FIODIR |=  MDC;

	/* Power Up the EMAC controller. */
	LPC_SC->PCONP |= PCONP_PCENET;
	INIT_DELAY_CALL( emacSHORT_DELAY );

	/* Reset all EMAC internal modules. */
	LPC_EMAC->MAC1 = MAC1_RES_TX | MAC1_RES_MCS_TX | MAC1_RES_RX | MAC1_RES_MCS_RX | MAC1_SIM_RES | MAC1_SOFT_RES;
	LPC_EMAC->Command = CR_REG_RES | CR_TX_RES | CR_RX_RES | CR_PASS_RUNT_FRM;

	/* A short delay after reset. */
	INIT_DELAY_CALL( emacSHORT_DELAY );

	LPC_EMAC->MAC1 = 0;

	/* Initialize MAC control registers. */
#ifdef PASS_AND_PRINT_CONTROL_FRAMES
	LPC_EMAC->MAC1 |= MAC1_PASS_ALL;
#endif
#if ETHERNET_LPC1758_FLOW_CONTROL  == ON
	LPC_EMAC->MAC1 |= (MAC1_RX_FLOWC | MAC1_TX_FLOWC);
#endif
	LPC_EMAC->MAC2 = MAC2_CRC_EN | MAC2_PAD_EN;
	LPC_EMAC->MAXF = ETH_MAX_FLEN;
	LPC_EMAC->CLRT = CLRT_DEF;
	LPC_EMAC->IPGR = IPGR_DEF;

	/* Enable Reduced MII interface. */

	LPC_EMAC->MCFG = MCFG_CLK_DIV20 | MCFG_RES_MII;
	INIT_DELAY_CALL( emacSHORT_DELAY );
	LPC_EMAC->MCFG = MCFG_CLK_DIV20;

	LPC_EMAC->Command = CR_RMII | CR_PASS_RUNT_FRM;

	/* Reset Reduced MII Logic. */
	LPC_EMAC->SUPP = SUPP_RES_RMII;
	INIT_DELAY_CALL( emacSHORT_DELAY );
	LPC_EMAC->SUPP = 0;

	/* Put the PHY in reset mode */
	prvWritePHY( PHY_REG_BMCR, MCFG_RES_MII );
	INIT_DELAY_CALL( emacSHORT_DELAY * 5);

	/* Wait for hardware reset to end. */
	for( x = 0; x < 100; x++ )
	{
		INIT_DELAY_CALL( emacSHORT_DELAY * 5 );
		us = prvReadPHY( PHY_REG_BMCR, &lDummy );
		if( !( us & MCFG_RES_MII ) )
		{
			/* Reset complete */
			break;
		}
	}
}

/*-----------------------------------------------------------*/

#if ETHERNET_LPC1758_FLOW_CONTROL == ON
static inline void enableEthFlowControl (void)
{
	/* Configure flow control counter */
	static const uint16_t MirrorCounter = 0x100;
	static const uint16_t PauseTimer = 0x1000;

	LPC_EMAC->FlowControlCounter = ((PauseTimer << 16) | MirrorCounter);

	/* Enable IEEE 802.3 / clause 32 flow control */
	LPC_EMAC->Command |= ( CR_TX_FLOW_CTRL );

	DEBUG_PRINT(ETHERNET_INTERUPT,"F-");
	DEBUG_PRINT(ETHERNET_FLOW_CONTROL,"F-\r\n");
}

static inline void disableEthFlowControl (void)
{
#ifdef TEST_PAUSE_FRAMES
#else
	/* Disable IEEE 802.3 / clause 32 flow control */
	LPC_EMAC->Command &= ~( CR_TX_FLOW_CTRL );

	DEBUG_PRINT(ETHERNET_INTERUPT,"F+");
	DEBUG_PRINT(ETHERNET_FLOW_CONTROL,"F+\r\n");
#endif
}
#endif

/*-----------------------------------------------------------*/

static long prvSetupLinkStatus( struct emacNetdevice * dev )
{
	long rv = pdPASS;

	unsigned short usLinkStatus;


	usLinkStatus = prvReadPHY( PHY_REG_AUTO_NEG_LINK_PARTNER_ABILITY, &rv );

	DEBUG_PRINT(DEBUG_NOTICE,"Read 0x%x so the other side can ",usLinkStatus);

	if(usLinkStatus & 1<<5)	{
		DEBUG_PRINT(DEBUG_NOTICE,"10 Mbps half duplex, ");
	}
	if(usLinkStatus & 1<<6)	{
		DEBUG_PRINT(DEBUG_NOTICE,"10 Mbps full duplex, ");
	}

	if(usLinkStatus & 1<<7)	{
		DEBUG_PRINT(DEBUG_NOTICE,"100 Mbps, ");
	}

	if(usLinkStatus & 1<<8)	{
		DEBUG_PRINT(DEBUG_NOTICE,"100 Mbps full duplex, ");
	}

	DEBUG_PRINT(DEBUG_NOTICE," .... done\r\n");

	usLinkStatus = prvReadPHY( PHY_REG_PHY_CTRL, &rv );
	/* Configure Full/Half Duplex mode. */
	if(  usLinkStatus & PHY_SPEED_FDUPLX )
	{
		DEBUG_PRINT(DEBUG_NOTICE,"Full");

		/* Full duplex is enabled. */
		LPC_EMAC->MAC2 |= MAC2_FULL_DUP;
		LPC_EMAC->Command |= CR_FULL_DUP;
		LPC_EMAC->IPGT = IPGT_FULL_DUP;
	}
	else
	{
		DEBUG_PRINT(DEBUG_NOTICE,"Half");
		/* Half duplex mode. */
		LPC_EMAC->IPGT = IPGT_HALF_DUP;
	}

	DEBUG_PRINT(DEBUG_NOTICE," duplex 10");

	/* Configure 100MBit/10MBit mode. */
	if( !(usLinkStatus & PHY_SPEED_100) )
	{
		/* 10MBit mode. */
		LPC_EMAC->SUPP = 0;
	}
	else
	{
		DEBUG_PRINT(DEBUG_NOTICE,"0");
		/* 100MBit mode. */
		LPC_EMAC->SUPP = SUPP_SPEED;
	}

	DEBUG_PRINT(DEBUG_NOTICE," MBit ");

#if ETHERNET_LPC1758_FLOW_CONTROL == ON
	/* Check the PHY flow control status. */
	usLinkStatus  = prvReadPHY( PHY_REG_ANLPAR, &rv );

	DEBUG_PRINT(DEBUG_NOTICE," PHY_REG_ANLPAR=0x%x ",usLinkStatus);

	if( (usLinkStatus & PHY_AUTO_NEG_PAUSE_RES) )
	{
		dev->useEthFlowControl = 1;
#ifdef TEST_PAUSE_FRAMES
		enableEthFlowControl();
		DEBUG_PRINT(DEBUG_NOTICE|ETHERNET_FLOW_CONTROL,"sending pause frames at base of 0x%x\r\n",LPC_EMAC->FlowControlCounter);
#endif
		disableEthFlowControl();
	}
	else
	{
		dev->useEthFlowControl = 0;

		DEBUG_PRINT(DEBUG_NOTICE|ETHERNET_FLOW_CONTROL,"no ");
	}
#else
	DEBUG_PRINT(DEBUG_NOTICE|ETHERNET_FLOW_CONTROL,"disabled ");
#endif
	DEBUG_PRINT(DEBUG_NOTICE|ETHERNET_FLOW_CONTROL,"flowControl\r\n");

	return rv;
}
/*-----------------------------------------------------------*/

struct netdeviceQueueElement * ulGetEMACRxDataGreenPhy( struct emacNetdevice * dev )
{
	struct netdeviceQueueElement * rv = NULL;

	if( LPC_EMAC->RxProduceIndex != LPC_EMAC->RxConsumeIndex )
	{
		long lIndex;

		rv = getQueueElementByBuffer((( uint8_t *  ) RX_DESC_PACKET( LPC_EMAC->RxConsumeIndex )),(( RX_STAT_INFO( LPC_EMAC->RxConsumeIndex ) & RINFO_SIZE ) - 3));

		if (rv)
		{
#ifdef PASS_AND_PRINT_CONTROL_FRAMES
			if(( RX_STAT_INFO( LPC_EMAC->RxConsumeIndex ) & RINFO_CTRL_FRAME ))
			{
				DEBUG_DUMP(DEBUG_ALL,getDataFromQueueElement(rv),getLengthFromQueueElement(rv),"ETH RX Control Frame");
			}
#endif
			DEBUG_PRINT(ETHERNET_RX,"(ETHERNET) rx buffer: 0x%x length %d\r\n",getDataFromQueueElement(rv),getLengthFromQueueElement(rv));

			DEBUG_DUMP(ETHERNET_RX_BINARY,getDataFromQueueElement(rv),getLengthFromQueueElement(rv),"ETH RX");

			DEBUG_DUMP_ICMP_FRAME(ETHERNET_TX,getDataFromQueueElement(rv),getLengthFromQueueElement(rv),"ETH RX");

			/* Allocate a new buffer to the descriptor. */
			struct netdeviceQueueElement * tmp = getQueueElement();
			DEBUG_PRINT(DEBUG_BUFFER,"[#I2#]\r\n");
			uint8_t * pData = removeDataFromQueueElement(&tmp);
			DEBUG_PRINT(ETHERNET_RX,"(ETHERNET) new buffer: 0x%x\r\n",pData);

			if(pData)
			{
				RX_DESC_PACKET( LPC_EMAC->RxConsumeIndex ) = ( unsigned long ) pData;
			}
			else
			{
				DEBUG_PRINT(DEBUG_ERR,"(ETH) %s no buffer...\r\n",__func__);
				DEBUG_PRINT(DEBUG_BUFFER,"[#I3#]\r\n");
				removeDataFromQueueElement(&rv);
			}
		}

		/* Move the consume index onto the next position, ensuring it wraps to
		the beginning at the appropriate place. */
		lIndex = LPC_EMAC->RxConsumeIndex;

		lIndex++;
		if( lIndex >= NUM_RX_FRAG )
		{
			lIndex = 0;
		}

		LPC_EMAC->RxConsumeIndex = lIndex;

#if ETHERNET_LPC1758_FLOW_CONTROL  == ON
		if(dev->useEthFlowControl)
		{
			if( LPC_EMAC->RxProduceIndex == LPC_EMAC->RxConsumeIndex )
			{
				disableEthFlowControl();
			}
		}
#else
		/* dev is only used for flow control */
		(void) dev;
#endif
	}
	else
	{
		DEBUG_PRINT(ETHERNET_RX,"(ETHERNET) empty! RxProduceIndex 0x%x == RxConsumeIndex 0x%x\r\n",LPC_EMAC->RxProduceIndex,LPC_EMAC->RxConsumeIndex);
	}

	return rv;
}

/*-----------------------------------------------------------*/

static long prvWritePHY( long lPhyReg, long lValue )
{
	const long lMaxTime = 10;
	long x= 0;

   /* Software MII Management for LPC175x. */
   /* Remapped MDC on P2.8 and MDIO on P2.9 do not work. */
   LPC1758_MDIO_PORT->FIODIR |= MDIO;

   /* 32 consecutive ones on MDO to establish sync */
   output_MDIO (0xFFFFFFFF, 32);

   /* start code (01), write command (01) */
   output_MDIO (0x05, 4);

   /* write PHY address */
   output_MDIO (DP83848C_DEF_ADR >> 8, 5);

   /* write the PHY register to write */
   output_MDIO (lPhyReg, 5);

   /* turnaround MDIO (1,0)*/
   output_MDIO (0x02, 2);

   /* write the data value */
   output_MDIO (lValue, 16);

   /* turnaround MDO is tristated */
   turnaround_MDIO ();

	if( x < lMaxTime )
	{
		return pdPASS;
	}
	else
	{
		return pdFAIL;
	}
}
/*-----------------------------------------------------------*/

static unsigned short prvReadPHY( unsigned char ucPhyReg, long *plStatus )
{
	unsigned int val;

	/* Software MII Management for LPC175x. */
	LPC1758_MDIO_PORT->FIODIR |= MDIO;

	/* 32 consecutive ones on MDO to establish sync */
	output_MDIO (0xFFFFFFFF, 32);

	/* start code (01), read command (10) */
	output_MDIO (0x06, 4);

	/* write PHY address */
	output_MDIO (DP83848C_DEF_ADR >> 8, 5);

	/* write the PHY register to write */
	output_MDIO (ucPhyReg, 5);

	/* turnaround MDO is tristated */
	turnaround_MDIO ();

	/* read the data value */
	val = input_MDIO ();

	/* turnaround MDIO is tristated */
	turnaround_MDIO ();
	*plStatus = pdPASS;
	return (val);
}
/*-----------------------------------------------------------*/

void vEMAC_ISR( void )
{
	DEBUG_PRINT(ETHERNET_INTERUPT,"[");
	unsigned long ulStatus;
	long lHigherPriorityTaskWoken = pdFALSE;

	ulStatus = LPC_EMAC->IntStatus;

	/* Clear the interrupt. */
	LPC_EMAC->IntClear = ulStatus;

	if( ulStatus & INT_RX_DONE )
	{
		DEBUG_PRINT(ETHERNET_INTERUPT,"r");

		/* Ensure the uIP task is not blocked as data has arrived. */
		if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
		{
			xSemaphoreGiveFromISR( eth0.xEMACSemaphore, &lHigherPriorityTaskWoken );
		}
		else
		{
			DEBUG_PRINT(DEBUG_ALL,"Scheduler not running:(1)%s ",__func__);
		}
	}

	if( ulStatus & INT_TX_DONE )
	{
		DEBUG_PRINT(ETHERNET_INTERUPT,"t");
		{
			/* The Tx buffer is no longer required. */
			DEBUG_PRINT(DEBUG_BUFFER,"0x%x",( uint8_t * ) TX_DESC_PACKET( emacTX_DESC_INDEX ) );
			prvReturnBuffer(  eth0.pBuffer[emacTX_DESC_INDEX] );
			eth0.pBuffer[emacTX_DESC_INDEX] = NULL;
			TX_DESC_PACKET( emacTX_DESC_INDEX ) = ( unsigned long ) NULL;
		}
	}

#if ETHERNET_LPC1758_FLOW_CONTROL  == ON
	/* the interface here is hard coded, just because the ISR hander gets
	 * no argument, only one ETH is supported by the LPC1758 */
	if(eth0.useEthFlowControl)
	{
		if( LPC_EMAC->RxProduceIndex != LPC_EMAC->RxConsumeIndex )
		{
			enableEthFlowControl();
		}
		else
		{
			disableEthFlowControl();
		}
	}
#endif

	if(lHigherPriorityTaskWoken)
	{
		DEBUG_PRINT(ETHERNET_INTERUPT,"W");
	}

	DEBUG_PRINT(ETHERNET_INTERUPT,"]\r\n");

	portEND_SWITCHING_ISR( lHigherPriorityTaskWoken );
}

/*--------------------------- output_MDIO -----------------------------------*/
static void delay (void) {;};

static void output_MDIO (unsigned int val, unsigned int n) {
	/* Output a value to the MII PHY management interface. */

	for (val <<= (32 - n); n; val <<= 1, n--) {
		if (val & 0x80000000) {
			LPC1758_MDIO_PORT->FIOSET = MDIO;
		}
		else {
			LPC1758_MDIO_PORT->FIOCLR = MDIO;
		}
		delay();
		LPC1758_MDIO_PORT->FIOSET = MDC;
		delay ();
		LPC1758_MDIO_PORT->FIOCLR = MDC;
	}
}

/*--------------------------- turnaround_MDIO -------------------------------*/

static void turnaround_MDIO (void) {
	/* Turnaround MDO is tristated. */

	LPC1758_MDIO_PORT->FIODIR &= ~MDIO;
	LPC1758_MDIO_PORT->FIOSET  = MDC;
	delay ();
	LPC1758_MDIO_PORT->FIOCLR  = MDC;
	delay ();
}

/*--------------------------- input_MDIO ------------------------------------*/

static unsigned int input_MDIO (void) {
	/* Input a value from the MII PHY management interface. */
	unsigned int i,val = 0;

	for (i = 0; i < 16; i++) {
		val <<= 1;
		LPC1758_MDIO_PORT->FIOSET = MDC;
		delay ();
		LPC1758_MDIO_PORT->FIOCLR = MDC;
		if (LPC1758_MDIO_PORT->FIOPIN & MDIO) {
			val |= 1;
		}
	}
	return (val);
}

/*-----------------------------------------------------------*/

static void ethAutoNegCheck( xTimerHandle xTimer )
{
	DEBUG_PRINT(ETHERNET_INTERUPT,"ETH AutoNeg timer with 0x%x expired\r\n",xTimer);

	struct emacNetdevice * emacNetdevice = (struct emacNetdevice *) pvTimerGetTimerID(xTimer);
	xTimerDelete(xTimer,0);
	emacNetdevice->ethAutoNegGuard = 0;
	if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
	{
		xSemaphoreGive(emacNetdevice->xEMACAutoNegotiationSemaphore);
	}
	else
	{
		DEBUG_PRINT(DEBUG_ALL,"Scheduler not running:(2)%s\r\n",__func__);
	}
}

/*-----------------------------------------------------------*/

static void startEthAutoNegGuard(struct netDeviceInterface * dev)
{
	struct emacNetdevice * emacNetdevice = (struct emacNetdevice *) dev;

	if(!emacNetdevice->ethAutoNegGuard)
	{
		xTimerHandle xTimer = xTimerCreate(ethAutoNegCheckName, 3000, FALSE, emacNetdevice, ethAutoNegCheck);
		DEBUG_PRINT(ETHERNET_INTERUPT,"start ETH AutoNeg timer 0x%x with emacNetdevice 0x%x\r\n",xTimer,emacNetdevice);
		emacNetdevice->ethAutoNegGuard = 1;
		xTimerStart(xTimer, 0);
	}
}

/*-----------------------------------------------------------*/

static void emacNetdeviceReset(struct netDeviceInterface * dev)
{
	/* reset is normally active low, so reset ... */
	GPIO_ClearValue(ETH_PHY_RESET_PORT,GPIO_MAP_PIN(ETH_PHY_RESET_PIN));
	DEBUG_PRINT(ETHERNET_INTERUPT ,"EthPhy reset low\r\n");
	/*  ... for 100 ms ... */
	INIT_DELAY_CALL( 100 / portTICK_RATE_MS);
	/* ... and release poor PHY from reset */
	GPIO_SetValue(ETH_PHY_RESET_PORT,GPIO_MAP_PIN(ETH_PHY_RESET_PIN));
	DEBUG_PRINT(ETHERNET_INTERUPT ,"EthPhy reset high\r\n");

	/* Initialise the MAC. */
	lEMACInit();

	DEBUG_PRINT(ETHERNET_INTERUPT ,"init done\r\n");

	startEthAutoNegGuard(dev);

	/* enable the EMAC interrupt */
	LPC_EMAC->IntEnable = ( INT_RX_DONE | INT_TX_DONE );

	/* Set the interrupt priority to the max permissible to cause some
	interrupt nesting. */
	NVIC_SetPriority( ENET_IRQn, configEMAC_INTERRUPT_PRIORITY );

	/* Enable the ETH interrupt. */
	NVIC_EnableIRQ( ENET_IRQn );

}

/*-----------------------------------------------------------*/

void ethPhyIrqHandler (portBASE_TYPE * xHigherPriorityTaskWoken)
{
	DEBUG_PRINT(ETHERNET_INTERUPT,"{");

	(void) xHigherPriorityTaskWoken;

    long lReturn = pdPASS;
#ifdef DEBUG
    unsigned long interupt;

    interupt = prvReadPHY( PHY_REG_INT_SOURCE, &lReturn );
	DEBUG_PRINT(ETHERNET_INTERUPT,"I0x%x 0x%x ",interupt,(interupt&0xfe));
#else
    prvReadPHY( PHY_REG_INT_SOURCE, &lReturn );
#endif
    /* Clear the ETH_PHY interrupt bit */
    GPIO_ClearInt(ETH_PHY_INTERRUPT_PORT,GPIO_MAP_PIN(ETH_PHY_INTERRUPT_PIN));

    /* Setup the PHY. eth0 is hardcoded, only one ETH interface present */
    prvSetupLinkStatus(&eth0);

	if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
	{
		xSemaphoreGiveFromISR( eth0.xEMACAutoNegotiationSemaphore, xHigherPriorityTaskWoken );
	}
	else
	{
		DEBUG_PRINT(DEBUG_ALL,"Scheduler not running:(3)%s\r\n",__func__);
	}

	DEBUG_PRINT(ETHERNET_INTERUPT,"}");
}

/*-----------------------------------------------------------*/

static int emacNetdeviceInit(struct netDeviceInterface * dev)
{
	int rv = 0;

	if(xTaskGetSchedulerState() != taskSCHEDULER_RUNNING)
	{
		DEBUG_PRINT(DEBUG_ALL,"Scheduler not running: %s\r\n",__func__);
	}

	struct emacNetdevice * emacNetdevice = (struct emacNetdevice *) dev;

	if( emacNetdevice->xEMACSemaphore == NULL )
	{
		vSemaphoreCreateBinary( emacNetdevice->xEMACSemaphore );
		vSemaphoreCreateBinary( emacNetdevice->xEMACAutoNegotiationSemaphore );

		if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
		{
			xSemaphoreTake( emacNetdevice->xEMACAutoNegotiationSemaphore, portMAX_DELAY );
		}
		else
		{
			DEBUG_PRINT(DEBUG_ALL,"Scheduler not running:(4)%s\r\n",__func__);
		}

		if( emacNetdevice->xEMACSemaphore != NULL && emacNetdevice->xEMACAutoNegotiationSemaphore != NULL )
		{
			/* PHY interrupt pin set up */
			GPIO_SetDir(ETH_PHY_INTERRUPT_PORT,GPIO_MAP_PIN(ETH_PHY_INTERRUPT_PIN),GPIO_IN);
			GPIO_IntCmd(ETH_PHY_INTERRUPT_PORT,GPIO_MAP_PIN(ETH_PHY_INTERRUPT_PIN),GPIO_INTERRUPT_FALLING_EDGE);

			/* PHY reset pin set up */
			DEBUG_PRINT(ETHERNET_INTERUPT,"EthPhy reset set to output\r\n");
			GPIO_SetDir(ETH_PHY_RESET_PORT,GPIO_MAP_PIN(ETH_PHY_RESET_PIN),GPIO_OUT);
		}
		else
		{
			if(emacNetdevice->xEMACSemaphore)
			{
				vSemaphoreDelete(emacNetdevice->xEMACSemaphore);
				emacNetdevice->xEMACSemaphore = NULL;
			}
			if(emacNetdevice->xEMACAutoNegotiationSemaphore)
			{
				vSemaphoreDelete(emacNetdevice->xEMACAutoNegotiationSemaphore);
				emacNetdevice->xEMACAutoNegotiationSemaphore = NULL;
			}
		}
	}

	return rv;
}

/*-----------------------------------------------------------*/

static int emacNetdeviceTx(struct netDeviceInterface * dev, struct netdeviceQueueElement * element)
{
	int rv = 0;
	struct emacNetdevice * eth = (struct emacNetdevice * ) dev;

	if(xQueueSend(eth->txQueue,&element,0) == pdTRUE)
	{
		rv = 1;
	}
	else
	{
		DEBUG_PRINT(ETHERNET_TX,"%s FULL! ",__func__);
		DEBUG_PRINT(DEBUG_BUFFER,"[#B#]\r\n");
		returnQueueElement(&element);
	}

	return rv;
}

/*-----------------------------------------------------------*/

static int emacNetdevicePutElementToHw(struct emacNetdevice * eth, struct netdeviceQueueElement * element)
{
	int rv = 0;

	unsigned long ulAttempts = 0UL;

	/* Check to see if the Tx descriptor is free, indicated by its buffer being
	NULL. */
	while( TX_DESC_PACKET( emacTX_DESC_INDEX ) != ( unsigned long ) NULL )
	{
		DEBUG_PRINT(ETHERNET_TX,"*");
		/* Wait for the Tx descriptor to become available. */
		vTaskDelay( emacBUFFER_WAIT_DELAY );
		ulAttempts++;
		if( ulAttempts > emacBUFFER_WAIT_ATTEMPTS)
		{
			/* Something has gone wrong as the Tx descriptor is still in use.
			Clear it down manually, the data it was sending will probably be
			lost. */
			DEBUG_PRINT(ETHERNET_TX|DEBUG_ERR|DEBUG_BUFFER,"rTX 0x%x\r\n",( uint8_t * )TX_DESC_PACKET( emacTX_DESC_INDEX ));
			// todo this is not failsave, returnQueueElement() must be used, or reset the system .... I never have seen this code to be executed ....
			prvReturnBuffer( ( uint8_t *  ) TX_DESC_PACKET( emacTX_DESC_INDEX ) );
			break;
		}
	}

	/* Setup the Tx descriptor for transmission. */

	uint16_t length = getLengthFromQueueElement(element)-1;
	uint8_t * pFrame = getDataFromQueueElement(element);
	DEBUG_PRINT(DEBUG_BUFFER,"[#I4#]\r\n");
	uint8_t * pData = removeDataFromQueueElement(&element);
	DEBUG_PRINT(ETHERNET_TX,"(ETHERNET) tx buffer: 0x%x length : %d\r\n",pData,length);

	DEBUG_DUMP(ETHERNET_TX_BINARY,pFrame,length,"(ETHERNET) TX");
	DEBUG_DUMP_ICMP_FRAME(ETHERNET_TX,pFrame,length,"(ETHERNET) TX ICMP");

	TX_DESC_PACKET( emacTX_DESC_INDEX ) = ( unsigned long ) pFrame;
	TX_DESC_CTRL( emacTX_DESC_INDEX ) = ( length | TCTRL_LAST | TCTRL_INT );

	eth->pBuffer[emacTX_DESC_INDEX] = pData;

	emacTX_DESC_INDEX += 1;
	if(emacTX_DESC_INDEX >= NUM_TX_FRAG)
	{
		emacTX_DESC_INDEX = 0;
	}

	LPC_EMAC->TxProduceIndex = ( emacTX_DESC_INDEX );

	return rv;
}

/*-----------------------------------------------------------*/

static struct netdeviceQueueElement * 	emacNetdeviceRxWithTimeout(struct netDeviceInterface * dev, unsigned long  timeout_ms)
{
	struct netdeviceQueueElement * rv;

	struct emacNetdevice * emacNetdevice = (struct emacNetdevice *) dev;

	rv = ulGetEMACRxDataGreenPhy(emacNetdevice);
	if(!rv)
	{
		DEBUG_PRINT(ETHERNET_RX,"(ETHERNET) taking sema ...\r\n");
		if( xSemaphoreTake( emacNetdevice->xEMACSemaphore, timeout_ms ) == pdTRUE )
		{
			DEBUG_PRINT(ETHERNET_RX,"(ETHERNET) sema taken\r\n");
			rv = ulGetEMACRxDataGreenPhy(emacNetdevice);
		}
	}

	return rv;
}

/*-----------------------------------------------------------*/

static int emacNetdeviceOpen(struct netDeviceInterface * dev)
{
	int rv = 0;

	struct emacNetdevice * emacNetdevice = (struct emacNetdevice *) dev;
	emacNetdevice->dev.tx = emacNetdeviceTx;

	/* register and enable the PHY interrupt */
	registerInterruptHandlerGPIO(ethPhyIrqHandler,ETH_PHY_INTERRUPT_PORT,ETH_PHY_INTERRUPT_PIN);

	/* reset the PHY */
	emacNetdeviceReset(dev);

	/* wait for the autonegotiation process to complete */
	if( xSemaphoreTake( emacNetdevice->xEMACAutoNegotiationSemaphore, portMAX_DELAY ) == pdTRUE )
	{
		DEBUG_PRINT(DEBUG_NOTICE," Autonegotiation complete\r\n");
	}
	else
	{
		DEBUG_PRINT(DEBUG_NOTICE," go on ... \r\n");
	}

	return rv;
}

/*-----------------------------------------------------------*/

static int emacNetdeviceClose(struct netDeviceInterface * dev)
{
	int rv = 0;
	struct emacNetdevice * emacNetdevice = (struct emacNetdevice *) dev;
	emacNetdevice->dev.tx = netdeviceReturnBuffer;
	NVIC_DisableIRQ( ENET_IRQn );
	unregisterInterruptHandlerGPIO(ethPhyIrqHandler);
	return rv;
}

/*-----------------------------------------------------------*/

static int emacNetdeviceExit(struct netDeviceInterface ** pdev)
{
	int rv = 1;

	if(pdev)
	{
		struct emacNetdevice * emacNetdevice = (struct emacNetdevice *) * pdev;

		emacNetdeviceClose(&emacNetdevice->dev);
		if(emacNetdevice->xEMACSemaphore)
		{
			vSemaphoreDelete( emacNetdevice->xEMACSemaphore );
			emacNetdevice->xEMACSemaphore = NULL;
		}
		if(emacNetdevice->xEMACAutoNegotiationSemaphore)
		{
			vSemaphoreDelete( emacNetdevice->xEMACAutoNegotiationSemaphore );
			emacNetdevice->xEMACAutoNegotiationSemaphore = NULL;
		}

		rv = 0;
	}

	return rv;
}

/*-----------------------------------------------------------*/

static void ethTxTask ( void * parameters)
{
	struct emacNetdevice * eth = (struct emacNetdevice * ) parameters;
	struct netdeviceQueueElement * element=NULL;

	for (;;)
	{
		xQueueReceive(eth->txQueue ,&element, portMAX_DELAY);
		if(element)
		{
			emacNetdevicePutElementToHw(eth,element);
			element = NULL;
		}
	}
}

/*-----------------------------------------------------------*/

struct netDeviceInterface * ethInitNetdevice ( void )
{
	struct netDeviceInterface * rv = NULL;

	struct emacNetdevice * emacNetdevice = &eth0;
	if( emacNetdevice != NULL && emacNetdevice->initialised == 0)
	{
		static const signed char * const name = (const signed char * const) "ETHtxTask";
		if ( pdPASS == xTaskCreate( ethTxTask, name, 250, emacNetdevice, tskIDLE_PRIORITY+4, NULL))
		{
			/* set up the txQueue */
			emacNetdevice->txQueue = xQueueCreate( 1, sizeof( struct netdeviceQueueElement *) );

			emacNetdevice->initialised = 1;
			emacNetdevice->dev.init = emacNetdeviceInit;
			emacNetdevice->dev.open = emacNetdeviceOpen;
			emacNetdevice->dev.close = emacNetdeviceClose;
			emacNetdevice->dev.exit = emacNetdeviceExit;
			emacNetdevice->dev.reset = emacNetdeviceReset;
			emacNetdevice->dev.tx = netdeviceReturnBuffer;
			emacNetdevice->dev.rxWithTimeout = emacNetdeviceRxWithTimeout;
			rv = &emacNetdevice->dev;
		}
	}

	return rv;
}
/*-----------------------------------------------------------*/
