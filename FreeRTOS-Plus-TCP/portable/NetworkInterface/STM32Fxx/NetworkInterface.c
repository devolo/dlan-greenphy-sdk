/*
 * Some constants, hardware definitions and comments taken from ST's HAL driver
 * library, COPYRIGHT(c) 2015 STMicroelectronics.
 */

/*
 * FreeRTOS+TCP Labs Build 160919 (C) 2016 Real Time Engineers ltd.
 * Authors include Hein Tibosch and Richard Barry
 *
 *******************************************************************************
 ***** NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ***
 ***                                                                         ***
 ***                                                                         ***
 ***   FREERTOS+TCP IS STILL IN THE LAB (mainly because the FTP and HTTP     ***
 ***   demos have a dependency on FreeRTOS+FAT, which is only in the Labs    ***
 ***   download):                                                            ***
 ***                                                                         ***
 ***   FreeRTOS+TCP is functional and has been used in commercial products   ***
 ***   for some time.  Be aware however that we are still refining its       ***
 ***   design, the source code does not yet quite conform to the strict      ***
 ***   coding and style standards mandated by Real Time Engineers ltd., and  ***
 ***   the documentation and testing is not necessarily complete.            ***
 ***                                                                         ***
 ***   PLEASE REPORT EXPERIENCES USING THE SUPPORT RESOURCES FOUND ON THE    ***
 ***   URL: http://www.FreeRTOS.org/contact  Active early adopters may, at   ***
 ***   the sole discretion of Real Time Engineers Ltd., be offered versions  ***
 ***   under a license other than that described below.                      ***
 ***                                                                         ***
 ***                                                                         ***
 ***** NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ***
 *******************************************************************************
 *
 * FreeRTOS+TCP can be used under two different free open source licenses.  The
 * license that applies is dependent on the processor on which FreeRTOS+TCP is
 * executed, as follows:
 *
 * If FreeRTOS+TCP is executed on one of the processors listed under the Special
 * License Arrangements heading of the FreeRTOS+TCP license information web
 * page, then it can be used under the terms of the FreeRTOS Open Source
 * License.  If FreeRTOS+TCP is used on any other processor, then it can be used
 * under the terms of the GNU General Public License V2.  Links to the relevant
 * licenses follow:
 *
 * The FreeRTOS+TCP License Information Page: http://www.FreeRTOS.org/tcp_license
 * The FreeRTOS Open Source License: http://www.FreeRTOS.org/license
 * The GNU General Public License Version 2: http://www.FreeRTOS.org/gpl-2.0.txt
 *
 * FreeRTOS+TCP is distributed in the hope that it will be useful.  You cannot
 * use FreeRTOS+TCP unless you agree that you use the software 'as is'.
 * FreeRTOS+TCP is provided WITHOUT ANY WARRANTY; without even the implied
 * warranties of NON-INFRINGEMENT, MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. Real Time Engineers Ltd. disclaims all conditions and terms, be they
 * implied, expressed, or statutory.
 *
 * 1 tab == 4 spaces!
 *
 * http://www.FreeRTOS.org
 * http://www.FreeRTOS.org/plus
 * http://www.FreeRTOS.org/labs
 *
 */

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_IP_Private.h"
#include "NetworkBufferManagement.h"
#include "NetworkInterface.h"

/* ST includes. */
#include "stm32f4xx_hal.h"

#ifndef	BMSR_LINK_STATUS
	#define BMSR_LINK_STATUS            0x0004UL
#endif

#ifndef	PHY_LS_HIGH_CHECK_TIME_MS
	/* Check if the LinkSStatus in the PHY is still high after 15 seconds of not
	receiving packets. */
	#define PHY_LS_HIGH_CHECK_TIME_MS	15000
#endif

#ifndef	PHY_LS_LOW_CHECK_TIME_MS
	/* Check if the LinkSStatus in the PHY is still low every second. */
	#define PHY_LS_LOW_CHECK_TIME_MS	1000
#endif

/* Interrupt events to process.  Currently only the Rx event is processed
although code for other events is included to allow for possible future
expansion. */
#define EMAC_IF_RX_EVENT        1UL
#define EMAC_IF_TX_EVENT        2UL
#define EMAC_IF_ERR_EVENT       4UL
#define EMAC_IF_ALL_EVENT       ( EMAC_IF_RX_EVENT | EMAC_IF_TX_EVENT | EMAC_IF_ERR_EVENT )

#define ETH_DMA_ALL_INTS \
	( ETH_DMA_IT_TST | ETH_DMA_IT_PMT | ETH_DMA_IT_MMC | ETH_DMA_IT_NIS | ETH_DMA_IT_AIS | ETH_DMA_IT_ER | \
	  ETH_DMA_IT_FBE | ETH_DMA_IT_ET | ETH_DMA_IT_RWT | ETH_DMA_IT_RPS | ETH_DMA_IT_RBU | ETH_DMA_IT_R | \
	  ETH_DMA_IT_TU | ETH_DMA_IT_RO | ETH_DMA_IT_TJT | ETH_DMA_IT_TPS | ETH_DMA_IT_T )

/* Naming and numbering of PHY registers. */
#define PHY_REG_00_BMCR			0x00	/* Basic mode control register */
#define PHY_REG_01_BMSR			0x01	/* Basic mode status register */
#define PHY_REG_02_PHYSID1		0x02	/* PHYS ID 1 */
#define PHY_REG_03_PHYSID2		0x03	/* PHYS ID 2 */
#define PHY_REG_04_ADVERTISE	0x04	/* Advertisement control reg */
#define	PHY_REG_19_PHYCR		0x19	/* 25 RW PHY Control Register */

/* Some defines used internally here to indicate preferences about speed, MDIX
(wired direct or crossed), and duplex (half or full). */
#define	PHY_SPEED_10       1
#define	PHY_SPEED_100      2
#define	PHY_SPEED_AUTO     (PHY_SPEED_10|PHY_SPEED_100)

#define	PHY_MDIX_DIRECT    1
#define	PHY_MDIX_CROSSED   2
#define	PHY_MDIX_AUTO      (PHY_MDIX_CROSSED|PHY_MDIX_DIRECT)

#define	PHY_DUPLEX_HALF    1
#define	PHY_DUPLEX_FULL    2
#define	PHY_DUPLEX_AUTO    (PHY_DUPLEX_FULL|PHY_DUPLEX_HALF)

/*
 * Description of all capabilities that can be advertised to
 * the peer (usually a switch or router).
 */
#define ADVERTISE_CSMA			0x0001		// Only selector supported
#define ADVERTISE_10HALF		0x0020		// Try for 10mbps half-duplex
#define ADVERTISE_10FULL		0x0040		// Try for 10mbps full-duplex
#define ADVERTISE_100HALF		0x0080		// Try for 100mbps half-duplex
#define ADVERTISE_100FULL		0x0100		// Try for 100mbps full-duplex

#define ADVERTISE_ALL			( ADVERTISE_10HALF | ADVERTISE_10FULL | \
								  ADVERTISE_100HALF | ADVERTISE_100FULL)

/*
 * Value for the 'PHY_REG_00_BMCR', the PHY's Basic mode control register
 */
#define BMCR_FULLDPLX			0x0100		// Full duplex
#define BMCR_ANRESTART			0x0200		// Auto negotiation restart
#define BMCR_ANENABLE			0x1000		// Enable auto negotiation
#define BMCR_SPEED100			0x2000		// Select 100Mbps
#define BMCR_RESET				0x8000		// Reset the PHY

#define PHYCR_MDIX_EN			0x8000		// Enable Auto MDIX
#define PHYCR_MDIX_FORCE		0x4000		// Force MDIX crossed

/*
 * Most users will want a PHY that negotiates about
 * the connection properties: speed, dmix and duplex.
 */

#if !defined( ipconfigETHERNET_AN_ENABLE )
	/* Enable auto-negotiation */
	#define ipconfigETHERNET_AN_ENABLE				1
#endif

#if !defined( ipconfigETHERNET_AUTO_CROSS_ENABLE )
	#define ipconfigETHERNET_AUTO_CROSS_ENABLE		1
#endif

#if( ipconfigETHERNET_AN_ENABLE == 0 )
	/*
	 * The following three defines are only used in case there
	 * is no auto-negotiation.
	 */
	#if !defined( ipconfigETHERNET_CROSSED_LINK )
		#define	ipconfigETHERNET_CROSSED_LINK			1
	#endif

	#if !defined( ipconfigETHERNET_USE_100MB )
		#define ipconfigETHERNET_USE_100MB				1
	#endif

	#if !defined( ipconfigETHERNET_USE_FULL_DUPLEX )
		#define ipconfigETHERNET_USE_FULL_DUPLEX		1
	#endif
#endif /* ipconfigETHERNET_AN_ENABLE == 0 */

/* Default the size of the stack used by the EMAC deferred handler task to twice
the size of the stack used by the idle task - but allow this to be overridden in
FreeRTOSConfig.h as configMINIMAL_STACK_SIZE is a user definable constant. */
#ifndef configEMAC_TASK_STACK_SIZE
	#define configEMAC_TASK_STACK_SIZE ( 2 * configMINIMAL_STACK_SIZE )
#endif

/*-----------------------------------------------------------*/

/*
 * A deferred interrupt handler task that processes
 */
static void prvEMACHandlerTask( void *pvParameters );

/*
 * Force a negotiation with the Switch or Router and wait for LS.
 */
static void prvEthernetUpdateConfig( BaseType_t xForce );

/*
 * See if there is a new packet and forward it to the IP-task.
 */
static BaseType_t prvNetworkInterfaceInput( void );

#if( ipconfigUSE_LLMNR != 0 )
	/*
	 * For LLMNR, an extra MAC-address must be configured to
	 * be able to receive the multicast messages.
	 */
	static void prvMACAddressConfig(ETH_HandleTypeDef *heth, uint32_t ulIndex, uint8_t *Addr);
#endif

/*-----------------------------------------------------------*/

typedef struct _PhyProperties_t
{
	uint8_t speed;
	uint8_t mdix;
	uint8_t duplex;
	uint8_t spare;
} PhyProperties_t;

/* Bit map of outstanding ETH interrupt events for processing.  Currently only
the Rx interrupt is handled, although code is included for other events to
enable future expansion. */
static volatile uint32_t ulISREvents;

/* A copy of PHY register 1: 'PHY_REG_01_BMSR' */
static uint32_t ulPHYLinkStatus = 0;

#if( ipconfigUSE_LLMNR == 1 )
	static const uint8_t xLLMNR_MACAddress[] = { 0x01, 0x00, 0x5E, 0x00, 0x00, 0xFC };
#endif

/* Ethernet handle. */
static ETH_HandleTypeDef xETH;

/* Value to be written into the 'Basic mode Control Register'. */
static uint32_t ulBCRvalue;

/* Value to be written into the 'Advertisement Control Register'. */
static uint32_t ulACRValue;

/* ucMACAddress as it appears in main.c */
extern const uint8_t ucMACAddress[ 6 ];

/* Holds the handle of the task used as a deferred interrupt processor.  The
handle is used so direct notifications can be sent to the task for all EMAC/DMA
related interrupts. */
static TaskHandle_t xEMACTaskHandle = NULL;

/* For local use only: describe the PHY's properties: */
const PhyProperties_t xPHYProperties =
{
	#if( ipconfigETHERNET_AN_ENABLE != 0 )
		.speed = PHY_SPEED_AUTO,
		.duplex = PHY_DUPLEX_AUTO,
	#else
		#if( ipconfigETHERNET_USE_100MB != 0 )
			.speed = PHY_SPEED_100,
		#else
			.speed = PHY_SPEED_10,
		#endif

		#if( ipconfigETHERNET_USE_FULL_DUPLEX != 0 )
			.duplex = PHY_DUPLEX_FULL,
		#else
			.duplex = PHY_DUPLEX_HALF,
		#endif
	#endif

	#if( ipconfigETHERNET_AN_ENABLE != 0 ) && ( ipconfigETHERNET_AUTO_CROSS_ENABLE != 0 )
		.mdix = PHY_MDIX_AUTO,
	#elif( ipconfigETHERNET_CROSSED_LINK != 0 )
		.mdix = PHY_MDIX_CROSSED,
	#else
		.mdix = PHY_MDIX_DIRECT,
	#endif
};

/* MAC buffers: ---------------------------------------------------------*/
__ALIGN_BEGIN ETH_DMADescTypeDef  DMARxDscrTab[ ETH_RXBUFNB ] __ALIGN_END;/* Ethernet Rx MA Descriptor */
__ALIGN_BEGIN ETH_DMADescTypeDef  DMATxDscrTab[ ETH_TXBUFNB ] __ALIGN_END;/* Ethernet Tx DMA Descriptor */
__ALIGN_BEGIN uint8_t Rx_Buff[ ETH_RXBUFNB ][ ETH_RX_BUF_SIZE ] __ALIGN_END; /* Ethernet Receive Buffer */
__ALIGN_BEGIN uint8_t Tx_Buff[ ETH_TXBUFNB ][ ETH_TX_BUF_SIZE ] __ALIGN_END; /* Ethernet Transmit Buffer */

/*-----------------------------------------------------------*/

void HAL_ETH_RxCpltCallback( ETH_HandleTypeDef *heth )
{
BaseType_t xHigherPriorityTaskWoken = 0;

	/* Ethernet RX-Complete callback function, elsewhere declared as weak. */
    ulISREvents |= EMAC_IF_RX_EVENT;
	if( xEMACTaskHandle != NULL )
	{
		vTaskNotifyGiveFromISR( xEMACTaskHandle, &xHigherPriorityTaskWoken );
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
}
/*-----------------------------------------------------------*/

void HAL_ETH_TxCpltCallback( ETH_HandleTypeDef *heth )
{
	/* This call-back would only be useful in case packets are being sent
	zero-copy.  Once they're sent, the buffers must be released. */
}
/*-----------------------------------------------------------*/

BaseType_t xNetworkInterfaceInitialise( void )
{
HAL_StatusTypeDef hal_eth_init_status;

	if( xEMACTaskHandle == NULL )
	{
		/* Initialise ETH */

		xETH.Instance = ETH;
		xETH.Init.AutoNegotiation = ETH_AUTONEGOTIATION_ENABLE;
		xETH.Init.Speed = ETH_SPEED_100M;
		xETH.Init.DuplexMode = ETH_MODE_FULLDUPLEX;
		xETH.Init.PhyAddress = 1;

		xETH.Init.MACAddr = ( uint8_t *) ucMACAddress;
		xETH.Init.RxMode = ETH_RXINTERRUPT_MODE;

		/* using the ETH_CHECKSUM_BY_HARDWARE option:
		both the IP and the protocol checksums will be calculated
		by the peripheral. */
		xETH.Init.ChecksumMode = ETH_CHECKSUM_BY_HARDWARE;

		xETH.Init.MediaInterface = ETH_MEDIA_INTERFACE_MII;
		hal_eth_init_status = HAL_ETH_Init( &xETH );

		/* Only for inspection by debugger. */
		( void ) hal_eth_init_status;

		__HAL_ETH_DMA_ENABLE_IT( &xETH , ETH_DMA_ALL_INTS );

		/* Initialize Tx Descriptors list: Chain Mode */
		HAL_ETH_DMATxDescListInit( &xETH, DMATxDscrTab, Tx_Buff[ 0 ], ETH_TXBUFNB );

		/* Initialize Rx Descriptors list: Chain Mode  */
		HAL_ETH_DMARxDescListInit( &xETH, DMARxDscrTab, Rx_Buff[ 0 ], ETH_RXBUFNB );

		#if( ipconfigUSE_LLMNR != 0 )
		{
			/* Program the LLMNR address at index 1. */
			prvMACAddressConfig( &xETH, ETH_MAC_ADDRESS1, ( uint8_t *) xLLMNR_MACAddress );
		}
		#endif

		/* Force a negotiation with the Switch or Router and wait for LS. */
		prvEthernetUpdateConfig( pdTRUE );

		/* The deferred interrupt handler task is created at the highest
		possible priority to ensure the interrupt handler can return directly
		to it.  The task's handle is stored in xEMACTaskHandle so interrupts can
		notify the task when there is something to process. */
		xTaskCreate( prvEMACHandlerTask, "EMAC", configEMAC_TASK_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, &xEMACTaskHandle );
	}
	/* When returning non-zero, the stack will become active and
    start DHCP (in configured) */
	return ( ulPHYLinkStatus & BMSR_LINK_STATUS ) != 0;
}
/*-----------------------------------------------------------*/

static void prvMACAddressConfig(ETH_HandleTypeDef *heth, uint32_t ulIndex, uint8_t *Addr)
{
uint32_t ulTempReg;

	/* Calculate the selected MAC address high register. */
	ulTempReg = 0x80000000ul | ( ( uint32_t ) Addr[ 5 ] << 8 ) | ( uint32_t ) Addr[ 4 ];

	/* Load the selected MAC address high register. */
	( *(__IO uint32_t *)( ( uint32_t ) ( ETH_MAC_ADDR_HBASE + ulIndex ) ) ) = ulTempReg;

	/* Calculate the selected MAC address low register. */
	ulTempReg = ( ( uint32_t ) Addr[ 3 ] << 24 ) | ( ( uint32_t ) Addr[ 2 ] << 16 ) | ( ( uint32_t ) Addr[ 1 ] << 8 ) | Addr[ 0 ];

	/* Load the selected MAC address low register */
	( *(__IO uint32_t *) ( ( uint32_t ) ( ETH_MAC_ADDR_LBASE + ulIndex ) ) ) = ulTempReg;
}
/*-----------------------------------------------------------*/

BaseType_t xNetworkInterfaceOutput( xNetworkBufferDescriptor_t * const pxDescriptor, BaseType_t bReleaseAfterSend )
{
BaseType_t xReturn;
uint32_t ulTransmitSize = 0;
__IO ETH_DMADescTypeDef *pxDmaTxDesc;

	#if( ipconfigDRIVER_INCLUDED_TX_IP_CHECKSUM != 0 )
	{
	ProtocolPacket_t *pxPacket;

		/* If the peripheral must calculate the checksum, it wants
		the protocol checksum to have a value of zero. */
		pxPacket = ( ProtocolPacket_t * ) ( pxDescriptor->pucEthernetBuffer );

		if( pxPacket->xICMPPacket.xIPHeader.ucProtocol == ipPROTOCOL_ICMP )
		{
			pxPacket->xICMPPacket.xICMPHeader.usChecksum = ( uint16_t )0u;
		}
	}
	#endif

	if( ( ulPHYLinkStatus & BMSR_LINK_STATUS ) != 0 )
	{
		/* This function does the actual transmission of the packet. The packet is
		contained in 'pxDescriptor' that is passed to the function. */
		pxDmaTxDesc = xETH.TxDesc;

		/* Is this buffer available? */
		if( ( pxDmaTxDesc->Status & ETH_DMATXDESC_OWN ) != 0 )
		{
			xReturn = pdFAIL;
		}
		else
		{
			/* Get bytes in current buffer. */
			ulTransmitSize = pxDescriptor->xDataLength;

			if( ulTransmitSize > ETH_TX_BUF_SIZE )
			{
				ulTransmitSize = ETH_TX_BUF_SIZE;
			}

			/* Copy the remaining bytes */
			memcpy( ( void * ) pxDmaTxDesc->Buffer1Addr, pxDescriptor->pucEthernetBuffer, ulTransmitSize );

			/* Prepare transmit descriptors to give to DMA. */
			HAL_ETH_TransmitFrame( &xETH, ulTransmitSize );

			iptraceNETWORK_INTERFACE_TRANSMIT();
			xReturn = pdPASS;
		}
	}
	else
	{
		/* The PHY has no Link Status, packet shall be dropped. */
		xReturn = pdFAIL;
	}

	#if( ipconfigZERO_COPY_TX_DRIVER == 0 )
	{
		/* The buffer has been sent so can be released. */
		if( bReleaseAfterSend != pdFALSE )
		{
			vReleaseNetworkBufferAndDescriptor( pxDescriptor );
		}
	}
	#endif

	return xReturn;
}
/*-----------------------------------------------------------*/

static BaseType_t prvNetworkInterfaceInput( void )
{
xNetworkBufferDescriptor_t *pxDescriptor;
uint16_t usReceivedLength;
__IO ETH_DMADescTypeDef *xDMARxDescriptor;
uint32_t ulSegCount;
xIPStackEvent_t xRxEvent = { eNetworkRxEvent, NULL };
const TickType_t xDescriptorWaitTime = pdMS_TO_TICKS( 250 );

	/* get received frame */
	if( HAL_ETH_GetReceivedFrame( &xETH ) != HAL_OK )
	{
		usReceivedLength = 0;
	}
	else
	{
		/* Obtain the size of the packet and put it into the "usReceivedLength" variable. */
		usReceivedLength = xETH.RxFrameInfos.length;

		if( usReceivedLength > 0 )
		{
			/* Create a buffer of the required length. */
			pxDescriptor = pxGetNetworkBufferWithDescriptor( usReceivedLength, xDescriptorWaitTime );

			if( pxDescriptor != NULL )
			{
				xDMARxDescriptor = xETH.RxFrameInfos.FSRxDesc;

				/* Copy remaining data. */
				if( usReceivedLength > pxDescriptor->xDataLength )
				{
					usReceivedLength = pxDescriptor->xDataLength;
				}

				memcpy( pxDescriptor->pucEthernetBuffer, ( uint8_t * ) xETH.RxFrameInfos.buffer, usReceivedLength);

				xRxEvent.pvData = ( void * ) pxDescriptor;

				/* Pass the data to the TCP/IP task for processing. */
				if( xSendEventStructToIPTask( &xRxEvent, xDescriptorWaitTime ) == pdFALSE )
				{
					/* Could not send the descriptor into the TCP/IP stack, it
					must be released. */
					vReleaseNetworkBufferAndDescriptor( pxDescriptor );
					iptraceETHERNET_RX_EVENT_LOST();
				}
				else
				{
					iptraceNETWORK_INTERFACE_RECEIVE();
				}

				/* Release descriptors to DMA.  Point to first descriptor. */
				xDMARxDescriptor = xETH.RxFrameInfos.FSRxDesc;
				ulSegCount = xETH.RxFrameInfos.SegCount;

				/* Set Own bit in RX descriptors: gives the buffers back to
				DMA. */
				while( ulSegCount != 0 )
				{
					xDMARxDescriptor->Status |= ETH_DMARXDESC_OWN;
					xDMARxDescriptor = ( ETH_DMADescTypeDef * ) xDMARxDescriptor->Buffer2NextDescAddr;
					ulSegCount--;
				}

				/* Clear Segment_Count */
				xETH.RxFrameInfos.SegCount = 0;
			}
			else
			{
				FreeRTOS_printf( ( "prvNetworkInterfaceInput: pxGetNetworkBuffer failed length %u\n", usReceivedLength ) );
			}
		}
		else
		{
			FreeRTOS_printf( ( "prvNetworkInterfaceInput: zero-sized packet?\n" ) );
			pxDescriptor = NULL;
		}

		/* When Rx Buffer unavailable flag is set clear it and resume
		reception. */
		if( ( xETH.Instance->DMASR & ETH_DMASR_RBUS ) != 0 )
		{
			/* Clear RBUS ETHERNET DMA flag. */
			xETH.Instance->DMASR = ETH_DMASR_RBUS;

			/* Resume DMA reception. */
			xETH.Instance->DMARPDR = 0;
		}
	}

	return ( usReceivedLength > 0 );
}
/*-----------------------------------------------------------*/

void vMACBProbePhy( void )
{
uint32_t ulPhyControl, ulConfig, ulAdvertise, ulLower, ulUpper, ulMACPhyID, ulValue;
TimeOut_t xPhyTime;
TickType_t xRemTime = 0;

	HAL_ETH_ReadPHYRegister(&xETH, PHY_REG_03_PHYSID2, &ulLower);
	HAL_ETH_ReadPHYRegister(&xETH, PHY_REG_02_PHYSID1, &ulUpper);

	ulMACPhyID = ( ( ulUpper << 16 ) & 0xFFFF0000 ) | ( ulLower & 0xFFF0 );

	/* The expected ID for the 'DP83848I' is 0x20005C90. */
	FreeRTOS_printf( ( "PHY ID %X\n", ulMACPhyID ) );

	/* Remove compiler warning if FreeRTOS_printf() is not defined. */
	( void ) ulMACPhyID;

    /* Set advertise register. */
	if( ( xPHYProperties.speed == PHY_SPEED_AUTO ) && ( xPHYProperties.duplex == PHY_DUPLEX_AUTO ) )
	{
		ulAdvertise = ADVERTISE_CSMA | ADVERTISE_ALL;
		/* Reset auto-negotiation capability. */
	}
	else
	{
		ulAdvertise = ADVERTISE_CSMA;

		if( xPHYProperties.speed == PHY_SPEED_AUTO )
		{
			if( xPHYProperties.duplex == PHY_DUPLEX_FULL )
			{
				ulAdvertise |= ADVERTISE_10FULL | ADVERTISE_100FULL;
			}
			else
			{
				ulAdvertise |= ADVERTISE_10HALF | ADVERTISE_100HALF;
			}
		}
		else if( xPHYProperties.duplex == PHY_DUPLEX_AUTO )
		{
			if( xPHYProperties.speed == PHY_SPEED_10 )
			{
				ulAdvertise |= ADVERTISE_10FULL | ADVERTISE_10HALF;
			}
			else
			{
				ulAdvertise |= ADVERTISE_100FULL | ADVERTISE_100HALF;
			}
		}
		else if( xPHYProperties.speed == PHY_SPEED_100 )
		{
			if( xPHYProperties.duplex == PHY_DUPLEX_FULL )
			{
				ulAdvertise |= ADVERTISE_100FULL;
			}
			else
			{
				ulAdvertise |= ADVERTISE_100HALF;
			}
		}
		else
		{
			if( xPHYProperties.duplex == PHY_DUPLEX_FULL )
			{
				ulAdvertise |= ADVERTISE_10FULL;
			}
			else
			{
				ulAdvertise |= ADVERTISE_10HALF;
			}
		}
	}

	/* Read Control register. */
	HAL_ETH_ReadPHYRegister(&xETH, PHY_REG_00_BMCR, &ulConfig);

	HAL_ETH_WritePHYRegister( &xETH, PHY_REG_00_BMCR, ulConfig | BMCR_RESET );
	xRemTime = ( TickType_t ) pdMS_TO_TICKS( 1000UL );
	vTaskSetTimeOutState( &xPhyTime );

	for( ; ; )
	{
		HAL_ETH_ReadPHYRegister(&xETH, PHY_REG_00_BMCR, &ulValue);
		if( ( ulValue & BMCR_RESET ) == 0 )
		{
			FreeRTOS_printf( ( "BMCR_RESET ready\n" ) );
			break;
		}
		if( xTaskCheckForTimeOut( &xPhyTime, &xRemTime ) != pdFALSE )
		{
			FreeRTOS_printf( ( "BMCR_RESET timed out\n" ) );
			break;
		}
	}
	HAL_ETH_WritePHYRegister( &xETH, PHY_REG_00_BMCR, ulConfig & ~BMCR_RESET);

	vTaskDelay( pdMS_TO_TICKS( 50ul ) );

    /* Write advertise register. */
	HAL_ETH_WritePHYRegister( &xETH, PHY_REG_04_ADVERTISE, ulAdvertise);

	/*
			AN_EN        AN1         AN0       Forced Mode
			  0           0           0        10BASE-T, Half-Duplex
			  0           0           1        10BASE-T, Full-Duplex
			  0           1           0        100BASE-TX, Half-Duplex
			  0           1           1        100BASE-TX, Full-Duplex
			AN_EN        AN1         AN0       Advertised Mode
			  1           0           0        10BASE-T, Half/Full-Duplex
			  1           0           1        100BASE-TX, Half/Full-Duplex
			  1           1           0        10BASE-T Half-Duplex
											   100BASE-TX, Half-Duplex
			  1           1           1        10BASE-T, Half/Full-Duplex
											   100BASE-TX, Half/Full-Duplex
	*/

    /* Read Control register. */
	HAL_ETH_ReadPHYRegister(&xETH, PHY_REG_00_BMCR, &ulConfig);

	ulConfig &= ~( BMCR_ANRESTART | BMCR_ANENABLE | BMCR_SPEED100 | BMCR_FULLDPLX );

	/* HT 12/9/14: always set AN-restart and AN-enable, even though the choices
	are limited. */
	ulConfig |= (BMCR_ANRESTART | BMCR_ANENABLE);

	if( xPHYProperties.speed == PHY_SPEED_100 )
	{
		ulConfig |= BMCR_SPEED100;
	}
	else if( xPHYProperties.speed == PHY_SPEED_10 )
	{
		ulConfig &= ~BMCR_SPEED100;
	}

	if( xPHYProperties.duplex == PHY_DUPLEX_FULL )
	{
		ulConfig |= BMCR_FULLDPLX;
	}
	else if( xPHYProperties.duplex == PHY_DUPLEX_HALF )
	{
		ulConfig &= ~BMCR_FULLDPLX;
	}

	/* Read PHY Control register. */
	HAL_ETH_ReadPHYRegister( &xETH, PHY_REG_19_PHYCR, &ulPhyControl );

	/* Clear bits which might get set: */
	ulPhyControl &= ~( PHYCR_MDIX_EN|PHYCR_MDIX_FORCE );

	if( xPHYProperties.mdix == PHY_MDIX_AUTO )
	{
		ulPhyControl |= PHYCR_MDIX_EN;
	}
	else if( xPHYProperties.mdix == PHY_MDIX_CROSSED )
	{
		/* Force direct link = Use crossed RJ45 cable. */
		ulPhyControl &= ~PHYCR_MDIX_FORCE;
	}
	else
	{
		/* Force crossed link = Use direct RJ45 cable. */
		ulPhyControl |= PHYCR_MDIX_FORCE;
	}
	/* update PHY Control Register. */
	HAL_ETH_WritePHYRegister( &xETH, PHY_REG_19_PHYCR, ulPhyControl );

	FreeRTOS_printf( ( "+TCP: advertise: %lX config %lX\n", ulAdvertise, ulConfig ) );

	/* Now the two values to global values for later use. */
	ulBCRvalue = ulConfig;
	ulACRValue = ulAdvertise;
}
/*-----------------------------------------------------------*/

static void prvEthernetUpdateConfig( BaseType_t xForce )
{
__IO uint32_t ulTimeout = 0;
uint32_t ulRegValue = 0;

	FreeRTOS_printf( ( "prvEthernetUpdateConfig: LS %d Force %d\n",
		( ulPHYLinkStatus & BMSR_LINK_STATUS ) != 0 ,
		xForce ) );

	if( ( xForce != pdFALSE ) || ( ( ulPHYLinkStatus & BMSR_LINK_STATUS ) != 0 ) )
	{
		/* Restart the auto-negotiation. */
		if( xETH.Init.AutoNegotiation != ETH_AUTONEGOTIATION_DISABLE )
		{
			/* Enable Auto-Negotiation. */
			HAL_ETH_WritePHYRegister( &xETH, PHY_BCR, ulBCRvalue | BMCR_ANRESTART );
			HAL_ETH_WritePHYRegister( &xETH, PHY_REG_04_ADVERTISE, ulACRValue);

			/* Wait until the auto-negotiation will be completed */
			do
			{
				ulTimeout++;
				HAL_ETH_ReadPHYRegister( &xETH, PHY_REG_01_BMSR, &ulRegValue );
			} while( ( ( ulRegValue & PHY_AUTONEGO_COMPLETE) == 0 ) && ( ulTimeout < PHY_READ_TO ) );

			HAL_ETH_WritePHYRegister( &xETH, PHY_BCR, ulBCRvalue & ~BMCR_ANRESTART );

			if( ulTimeout < PHY_READ_TO )
			{
				/* Reset Timeout counter. */
				ulTimeout = 0;

				/* Read the result of the auto-negotiation. */
				HAL_ETH_ReadPHYRegister( &xETH, PHY_SR, &ulRegValue);
				if( ( ulRegValue & PHY_LINK_STATUS ) != 0 )
				{
					ulPHYLinkStatus |= BMSR_LINK_STATUS;
				}
				else
				{
					ulPHYLinkStatus &= ~( BMSR_LINK_STATUS );
				}

				FreeRTOS_printf( ( ">> Autonego ready: %08x: %s duplex %u mbit %s status\n",
					ulRegValue,
					(ulRegValue & PHY_DUPLEX_STATUS) ? "full" : "half",
					(ulRegValue & PHY_SPEED_STATUS) ? 10 : 100,
					(ulRegValue & PHY_LINK_STATUS) ? "high" : "low" ) );

				/* Configure the MAC with the Duplex Mode fixed by the
				auto-negotiation process. */
				if( ( ulRegValue & PHY_DUPLEX_STATUS ) != ( uint32_t ) RESET )
				{
					/* Set Ethernet duplex mode to Full-duplex following the
					auto-negotiation. */
					xETH.Init.DuplexMode = ETH_MODE_FULLDUPLEX;
				}
				else
				{
					/* Set Ethernet duplex mode to Half-duplex following the
					auto-negotiation. */
					xETH.Init.DuplexMode = ETH_MODE_HALFDUPLEX;
				}

				/* Configure the MAC with the speed fixed by the
				auto-negotiation process. */
				if( ( ulRegValue & PHY_SPEED_STATUS) != 0 )
				{
					/* Set Ethernet speed to 10M following the
					auto-negotiation. */
					xETH.Init.Speed = ETH_SPEED_10M;
				}
				else
				{
					/* Set Ethernet speed to 100M following the
					auto-negotiation. */
					xETH.Init.Speed = ETH_SPEED_100M;
				}
			}	/* if( ulTimeout < PHY_READ_TO ) */
		}
		else /* AutoNegotiation Disable */
		{
		uint16_t usValue;

			/* Check parameters */
			assert_param( IS_ETH_SPEED( xETH.Init.Speed ) );
			assert_param( IS_ETH_DUPLEX_MODE( xETH.Init.DuplexMode ) );

			/* Set MAC Speed and Duplex Mode to PHY */
			usValue = ( uint16_t ) ( xETH.Init.DuplexMode >> 3 ) | ( uint16_t ) ( xETH.Init.Speed >> 1 );
			HAL_ETH_WritePHYRegister( &xETH, PHY_BCR, usValue );
		}

		/* ETHERNET MAC Re-Configuration */
		HAL_ETH_ConfigMAC( &xETH, (ETH_MACInitTypeDef *) NULL);

		/* Restart MAC interface */
		HAL_ETH_Start( &xETH);

		FreeRTOS_printf( ( "MACCR = %08x DMABMR = %08x\n",
			xETH.Instance->MACCR,
			xETH.Instance->DMABMR ) );
	}
	else
	{
		/* Stop MAC interface */
		HAL_ETH_Stop( &xETH );
	}
}
/*-----------------------------------------------------------*/

BaseType_t xGetPhyLinkStatus( void )
{
BaseType_t xReturn;

	if( ( ulPHYLinkStatus & BMSR_LINK_STATUS ) != 0 )
	{
		xReturn = pdPASS;
	}
	else
	{
		xReturn = pdFAIL;
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

static void prvEMACHandlerTask( void *pvParameters )
{
TimeOut_t xPhyTime;
TickType_t xPhyRemTime;
UBaseType_t uxLastMinBufferCount = 0;
UBaseType_t uxCurrentCount;
BaseType_t xResult = 0;
uint32_t xStatus;
const TickType_t ulMaxBlockTime = pdMS_TO_TICKS( 100UL );

	/* Remove compiler warnings about unused parameters. */
	( void ) pvParameters;

	vTaskSetTimeOutState( &xPhyTime );
	xPhyRemTime = pdMS_TO_TICKS( PHY_LS_LOW_CHECK_TIME_MS );

	for( ;; )
	{
		uxCurrentCount = uxGetMinimumFreeNetworkBuffers();
		if( uxLastMinBufferCount != uxCurrentCount )
		{
			/* The logging produced below may be helpful
			while tuning +TCP: see how many buffers are in use. */
			uxLastMinBufferCount = uxCurrentCount;
			FreeRTOS_printf( ( "Network buffers: %lu lowest %lu\n",
				uxGetNumberOfFreeNetworkBuffers(), uxCurrentCount ) );
		}

		#if( ipconfigCHECK_IP_QUEUE_SPACE != 0 )
		{
			uxCurrentCount = uxGetMinimumIPQueueSpace();
			if( uxLastMinQueueSpace != uxCurrentCount )
			{
				/* The logging produced below may be helpful
				while tuning +TCP: see how many buffers are in use. */
				uxLastMinQueueSpace = uxCurrentCount;
				FreeRTOS_printf( ( "Queue space: lowest %lu\n", uxCurrentCount ) );
			}
		}
		#endif /* ipconfigCHECK_IP_QUEUE_SPACE */

		if( ( ulISREvents & EMAC_IF_ALL_EVENT ) == 0 )
		{
			/* No events to process now, wait for the next. */
			ulTaskNotifyTake( pdFALSE, ulMaxBlockTime );
		}

		if( ( ulISREvents & EMAC_IF_RX_EVENT ) != 0 )
		{
			ulISREvents &= ~EMAC_IF_RX_EVENT;

			xResult = prvNetworkInterfaceInput();
			if( xResult > 0 )
			{
			  	while( prvNetworkInterfaceInput() > 0 )
				{
				}
			}
		}

		if( ( ulISREvents & EMAC_IF_TX_EVENT ) != 0 )
		{
			/* Future extension: code to release TX buffers if zero-copy is used. */
			ulISREvents &= ~EMAC_IF_TX_EVENT;
		}

		if( ( ulISREvents & EMAC_IF_ERR_EVENT ) != 0 )
		{
			/* Future extension: logging about errors that occurred. */
			ulISREvents &= ~EMAC_IF_ERR_EVENT;
		}

		if( xResult > 0 )
		{
			/* A packet was received. No need to check for the PHY status now,
			but set a timer to check it later on. */
			vTaskSetTimeOutState( &xPhyTime );
			xPhyRemTime = pdMS_TO_TICKS( PHY_LS_HIGH_CHECK_TIME_MS );
			xResult = 0;
		}
		else if( xTaskCheckForTimeOut( &xPhyTime, &xPhyRemTime ) != pdFALSE )
		{
			HAL_ETH_ReadPHYRegister( &xETH, PHY_REG_01_BMSR, &xStatus );
			if( ( ulPHYLinkStatus & BMSR_LINK_STATUS ) != ( xStatus & BMSR_LINK_STATUS ) )
			{
				ulPHYLinkStatus = xStatus;
				FreeRTOS_printf( ( "prvEMACHandlerTask: PHY LS now %d\n", ( ulPHYLinkStatus & BMSR_LINK_STATUS ) != 0 ) );
				prvEthernetUpdateConfig( pdFALSE );
			}

			vTaskSetTimeOutState( &xPhyTime );
			if( ( ulPHYLinkStatus & BMSR_LINK_STATUS ) != 0 )
			{
				xPhyRemTime = pdMS_TO_TICKS( PHY_LS_HIGH_CHECK_TIME_MS );
			}
			else
			{
				xPhyRemTime = pdMS_TO_TICKS( PHY_LS_LOW_CHECK_TIME_MS );
			}
		}
	}
}
/*-----------------------------------------------------------*/

void ETH_IRQHandler( void )
{
	HAL_ETH_IRQHandler( &xETH );
}
