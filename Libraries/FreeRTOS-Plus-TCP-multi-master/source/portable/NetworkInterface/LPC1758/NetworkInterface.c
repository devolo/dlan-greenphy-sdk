/*
 * FreeRTOS+TCP Labs Build 160916 (C) 2016 Real Time Engineers ltd.
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
#include "FreeRTOS_Routing.h"
#include "FreeRTOS_Bridge.h"
#include "NetworkBufferManagement.h"
#include "NetworkInterface.h"

/* LPCOpen includes. */
#include "chip.h"
#include "lpc_phy.h"

/* The size of the stack allocated to the task that handles Rx packets. */
#define configEMAC_TASK_STACK_SIZE	200

#ifndef	PHY_LS_HIGH_CHECK_TIME_MS
	/* Check if the LinkSStatus in the PHY is still high after 15 seconds of not
	receiving packets. */
	#define PHY_LS_HIGH_CHECK_TIME_MS	15000
#endif

#ifndef	PHY_LS_LOW_CHECK_TIME_MS
	/* Check if the LinkSStatus in the PHY is still low every second. */
	#define PHY_LS_LOW_CHECK_TIME_MS	3000
#endif

#ifndef configUSE_RMII
	#define configUSE_RMII 1
#endif

#ifndef ipconfigNUM_RX_DESCRIPTORS
	#error please define ipconfigNUM_RX_DESCRIPTORS in your FreeRTOSIPConfig.h
#endif

#ifndef ipconfigNUM_TX_DESCRIPTORS
	#error please define ipconfigNUM_TX_DESCRIPTORS in your FreeRTOSIPConfig.h
#endif

#ifndef NETWORK_IRQHandler
	#error NETWORK_IRQHandler must be defined to the name of the function that is installed in the interrupt vector table to handle Ethernet interrupts.
#endif

#if( ipconfigZERO_COPY_RX_DRIVER == 0 )
	#warning ipconfigZERO_COPY_RX_DRIVER 0 is not tested
#endif

#if( ipconfigZERO_COPY_TX_DRIVER == 0 )
	#warning ipconfigZERO_COPY_TX_DRIVER 0 is not tested
#endif

 /* If ipconfigETHERNET_DRIVER_FILTERS_FRAME_TYPES is set to 1, then the Ethernet
 driver will filter incoming packets and only pass the stack those packets it
 considers need processing. */
 #if( ipconfigETHERNET_DRIVER_FILTERS_FRAME_TYPES != 1 )
 	#define ipCONSIDER_FRAME_FOR_PROCESSING( pucEthernetBuffer ) eProcessBuffer
 #else
 	#define ipCONSIDER_FRAME_FOR_PROCESSING( pucEthernetBuffer ) eConsiderFrameForProcessing( ( pucEthernetBuffer ) )
 #endif

/* Receive group interrupts */
#define RXINTGROUP (ENET_INT_RXOVERRUN | ENET_INT_RXERROR | ENET_INT_RXDONE)

/* Transmit group interrupts */
#define TXINTGROUP (ENET_INT_TXUNDERRUN | ENET_INT_TXERROR | ENET_INT_TXDONE)

/*-----------------------------------------------------------*/


/*
 * Delay function passed into the library.  The implementation uses FreeRTOS
 * calls so the scheduler must be started before the driver can be used.
 */
static void prvDelay( uint32_t ulMilliSeconds );

/*
 * Initialises the Tx and Rx descriptors respectively.
 */
static void prvSetupTxDescriptors( void );
static void prvSetupRxDescriptors( void );

/*
 * A task that processes received frames.
 */
static void prvEMACHandlerTask( void *pvParameters );

/*
 * Sets up the MAC with the results of an auto-negotiation.
 */
static void prvSetLinkSpeed( void );

/*-----------------------------------------------------------*/

/* A copy of PHY status */
static uint32_t ulPHYLinkStatus = 0;

/* Index to the next DMA descriptor that needs to be released */
static UBaseType_t ulTxCleanupIndex = 0;

/* The handle of the task that processes Rx packets.  The handle is required so
the task can be notified when new packets arrive. */
static TaskHandle_t xEMACTaskHandle = NULL;

/* xTXDescriptorSemaphore is a counting semaphore with
a maximum count of ipconfigNUM_TX_DESCRIPTORS, which is the number of
DMA TX descriptors. */
static SemaphoreHandle_t xTXDescriptorSemaphore = NULL;
static SemaphoreHandle_t xTXMutex = NULL;

/* The EMAC DMA descriptors are stored in AHB SRAM for faster access */
static __attribute__ ((section(".bss.$RAM2")))
		ENET_RXDESC_T xDMARxDescriptors[ipconfigNUM_RX_DESCRIPTORS];
static __attribute__ ((section(".bss.$RAM2")))
		ENET_RXSTAT_T xDMARxStatus[ipconfigNUM_RX_DESCRIPTORS];
static __attribute__ ((section(".bss.$RAM2")))
		ENET_TXDESC_T xDMATxDescriptors[ipconfigNUM_TX_DESCRIPTORS];
static __attribute__ ((section(".bss.$RAM2")))
		ENET_TXSTAT_T xDMATxStatus[ipconfigNUM_TX_DESCRIPTORS];

/*-----------------------------------------------------------*/

void vCheckBuffersAndQueue( void )
{
static UBaseType_t uxLastBufferCount = 0;
#if( ipconfigCHECK_IP_QUEUE_SPACE != 0 )
	static UBaseType_t uxLastMinQueueSpace;
#endif
static UBaseType_t uxCurrentCount;

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
	uxCurrentCount = uxGetNumberOfFreeNetworkBuffers();
	if( uxLastBufferCount != uxCurrentCount )
	{
		/* The logging produced below may be helpful
		while tuning +TCP: see how many buffers are in use. */
		uxLastBufferCount = uxCurrentCount;
		//FreeRTOS_printf( ( "Network buffers: %lu lowest %lu\n",
		//	uxCurrentCount, uxGetMinimumFreeNetworkBuffers() ) );
	}
}
/*-----------------------------------------------------------*/

static void prvDelay( uint32_t ulMilliSeconds )
{
	/* Ensure the scheduler was started before attempting to use the scheduler to
	create a delay. */
	configASSERT( xTaskGetSchedulerState() == taskSCHEDULER_RUNNING );

	vTaskDelay( pdMS_TO_TICKS( ulMilliSeconds ) );
}
/*-----------------------------------------------------------*/

static void prvSetupTxDescriptors( void )
{
	BaseType_t x;

	/* Start with Tx descriptors clear. */
	memset( ( void * ) xDMATxDescriptors, 0, sizeof( xDMATxDescriptors ) );

	/* Build TX descriptors for local buffers */
	for (x = 0; x < ipconfigNUM_TX_DESCRIPTORS; x++) {
		#if( ipconfigZERO_COPY_TX_DRIVER != 0 )
		{
			/* Nothing to do, Packet pointer will be set when data is ready to transmit.
			Currently the memset above will have set it to NULL. */
		}
		#else
		{
			/* Allocate a buffer to the Tx descriptor.  This is the most basic
			way of creating a driver as the data is then copied into the
			buffer. */
			xDMATxDescriptors[ x ].Packet = ( uint32_t ) pvPortMalloc( ipTOTAL_ETHERNET_FRAME_SIZE );

			/* Use an assert to check the allocation as +TCP applications will
			often not use a malloc() failed hook as the TCP stack will recover
			from allocation failures. */
			configASSERT( xDMATxDescriptors[ x ].Packet );
		}
		#endif

		/* TX DMA descriptor's control register is set to interrupt and end-of-chain
		on output, as it also contains the buffers actual length. */
	}

	/* Point the DMA to the base of the descriptor list. */
	Chip_ENET_InitTxDescriptors(LPC_ETHERNET, xDMATxDescriptors, xDMATxStatus, ipconfigNUM_TX_DESCRIPTORS);
}
/*-----------------------------------------------------------*/

static void prvSetupRxDescriptors( void )
{
BaseType_t x;
#if( ipconfigZERO_COPY_RX_DRIVER != 0 )
	NetworkBufferDescriptor_t *pxNetworkBuffer;
#endif

	/* Clear RX descriptor list. */
	memset( ( void * )  xDMARxDescriptors, 0, sizeof( xDMARxDescriptors ) );

	/* Build RX descriptors for local buffers */
	for( x = 0; x < ipconfigNUM_RX_DESCRIPTORS; x++ )
	{
		/* Allocate a buffer of the largest	possible frame size as it is not
		known what size received frames will be. */

		#if( ipconfigZERO_COPY_RX_DRIVER != 0 )
		{
			pxNetworkBuffer = pxGetNetworkBufferWithDescriptor( ipTOTAL_ETHERNET_FRAME_SIZE, ( TickType_t ) 0 );

			/* During start-up there should be enough Network Buffers available,
			so it is safe to use configASSERT().
			In case this assert fails, please check: ipconfigNUM_RX_DESCRIPTORS,
			ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS, and in case BufferAllocation_2.c
			is included, check the amount of available heap. */
			configASSERT( pxNetworkBuffer != NULL );

			/* Pass the actual buffer to DMA. */
			xDMARxDescriptors[ x ].Packet = ( uint32_t ) pxNetworkBuffer->pucEthernetBuffer;
		}
		#else
		{
			/* All DMA descriptors are populated with permanent memory blocks.
			Their contents will be copy to Network Buffers. */
			xDMARxDescriptors[ x ].Packet = ( uint32_t ) pvPortMalloc( ipTOTAL_ETHERNET_FRAME_SIZE );
		}
		#endif /* ipconfigZERO_COPY_RX_DRIVER */

		/* Use an assert to check the allocation as +TCP applications will often
		not use a malloc failed hook as the TCP stack will recover from
		allocation failures. */
		configASSERT( xDMARxDescriptors[ x ].Packet );

		/* Set RX interrupt and buffer size */
		xDMARxDescriptors[ x ].Control = ENET_RCTRL_INT | (( uint32_t ) ENET_RCTRL_SIZE( ipTOTAL_ETHERNET_FRAME_SIZE ));
	}

	/* Point the DMA to the base of the descriptor list. */
	Chip_ENET_InitRxDescriptors(LPC_ETHERNET, xDMARxDescriptors, xDMARxStatus, ipconfigNUM_RX_DESCRIPTORS);
}
/*-----------------------------------------------------------*/

static void prvSetLinkSpeed( void )
{
	if( ( ulPHYLinkStatus & PHY_LINK_CONNECTED ) != 0x00 )
	{
		/* Set interface speed and duplex. */
		if( ( ulPHYLinkStatus & PHY_LINK_SPEED100 ) != 0x00 )
		{
			Chip_ENET_Set100Mbps( LPC_ETHERNET );
		}
		else
		{
			Chip_ENET_Set10Mbps( LPC_ETHERNET );
		}

		if( ( ulPHYLinkStatus & PHY_LINK_FULLDUPLX ) != 0x00 )
		{
			Chip_ENET_SetFullDuplex( LPC_ETHERNET );
		}
		else
		{
			Chip_ENET_SetHalfDuplex( LPC_ETHERNET );
		}

	}
}
/*-----------------------------------------------------------*/

BaseType_t prvUpdatePHYLinkStatus( TickType_t xDelay )
{
const TickType_t xTimeBetweenAttempts = pdMS_TO_TICKS( 50UL );
BaseType_t xReturn = pdFAIL;
TickType_t xTimeOnEntering = xTaskGetTickCount();

	do
	{
		ulPHYLinkStatus = lpcPHYStsPoll();

		if( ulPHYLinkStatus & PHY_LINK_CONNECTED )
		{
			if( ulPHYLinkStatus & PHY_LINK_CHANGED )
			{
				prvSetLinkSpeed();
			}

			xReturn = pdPASS;
			break;
		}

		vTaskDelay( pdMS_TO_TICKS( xTimeBetweenAttempts ) );
	} while( ( xTaskGetTickCount() - xTimeOnEntering ) < xDelay );

	return xReturn;
}
/*-----------------------------------------------------------*/

static BaseType_t xLPC1758_GetPhyLinkStatus( NetworkInterface_t *pxInterface )
{
BaseType_t xReturn;

	if( ( ulPHYLinkStatus & PHY_LINK_CONNECTED ) != 0 )
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

BaseType_t xLPC1758_NetworkInterfaceInitialise( NetworkInterface_t *pxInterface )
{
const TickType_t xAutoNegotiateDelay = pdMS_TO_TICKS( 2000UL );
BaseType_t xReturn = pdPASS;
NetworkEndPoint_t *pxEndPoint;

	/* The interrupt will be turned on when a link is established. */
	NVIC_DisableIRQ( ETHERNET_IRQn );

	/* Disable packet reception. */
	Chip_ENET_RXDisable(LPC_ETHERNET);
	Chip_ENET_TXDisable(LPC_ETHERNET);

	if( pxInterface->bits.bInterfaceInitialised == pdFALSE_UNSIGNED )
	{
		/* Call the LPCOpen function to initialise the hardware. */
		#if( configUSE_RMII == 1 )
		{
			Chip_ENET_Init(LPC_ETHERNET, true);
		}
		#else
		{
			Chip_ENET_Init(LPC_ETHERNET, false);
		}
		#endif

		/* _ML_ TODO: Ethernet driver needs to wait until GreenPHY has read its MAC. */
//	#if( ipconfigUSE_BRIDGE != 0 )
//		if( pxInterface->bits.bIsBridged != 0 )
//		{
//			/* If interface is bridged run in promiscuous mode and
//			do not activate RX filters */
//		}
//		else
//	#endif
		{
			/* Not bridged. Get MAC address from associated Endpoint. */
			pxEndPoint = FreeRTOS_FirstEndPoint( NULL /*pxInterface*/ );
			configASSERT( pxEndPoint != NULL );

			/* Save MAC address. */
			Chip_ENET_SetADDR( LPC_ETHERNET, pxEndPoint->xMACAddress.ucBytes );

			/* Perfect match and Broadcast enabled */
//			Chip_ENET_EnableRXFilter(LPC_ETHERNET, ENET_RXFILTERCTRL_APE | ENET_RXFILTERCTRL_ABE);
		}

		/* Initialize the PHY */
		#define LPC_PHYDEF_PHYADDR 1
		Chip_ENET_SetupMII(LPC_ETHERNET, Chip_ENET_FindMIIDiv(LPC_ETHERNET, 2500000), LPC_PHYDEF_PHYADDR);

		#if( configUSE_RMII == 1 )
		{
			if( lpc_phy_init( pdTRUE, prvDelay ) != SUCCESS )
			{
				xReturn = pdFAIL;
			}
		}
		#else
		{
			#warning This path has not been tested.
			if( lpc_phy_init( pdFALSE, prvDelay ) != SUCCESS )
			{
				xReturn = pdFAIL;
			}
		}
		#endif

		/* Guard the descriptors from being initialised more than once. */
		if( xTXDescriptorSemaphore == NULL )
		{
			/* Initialise the descriptors. */
			prvSetupTxDescriptors();
			prvSetupRxDescriptors();

			/* Create a semaphore which counts free TX descriptors. */
			xTXDescriptorSemaphore = xSemaphoreCreateCounting( ( UBaseType_t ) ipconfigNUM_TX_DESCRIPTORS, ( UBaseType_t ) ipconfigNUM_TX_DESCRIPTORS );
			configASSERT( xTXDescriptorSemaphore );
			xTXMutex = xSemaphoreCreateMutex();
			configASSERT( xTXMutex );
		}

		/* Guard against the task being created more than once. */
		if( xEMACTaskHandle == NULL )
		{
			xTaskCreate( prvEMACHandlerTask, pxInterface->pcName, configEMAC_TASK_STACK_SIZE, pxInterface, configMAX_PRIORITIES - 1, &xEMACTaskHandle );
			configASSERT( xEMACTaskHandle );
		}

		if( xReturn != pdFAIL )
		{
			/* Auto-negotiate was already started.  Wait for it to complete. */
			prvUpdatePHYLinkStatus( xAutoNegotiateDelay );
		}

		pxInterface->bits.bInterfaceInitialised = pdTRUE_UNSIGNED;
	}

	if( xReturn != pdFAIL )
	{
		xReturn = xLPC1758_GetPhyLinkStatus( pxInterface );
		if( xReturn == pdPASS )
		{
			/* Resume the EMAC Task in case it was suspended after a PHY disconnect. */
//			vTaskResume( xEMACTaskHandle );

			/* Clear and enable RX/TX interrupts. */
			Chip_ENET_EnableInt(LPC_ETHERNET, RXINTGROUP | TXINTGROUP);

			/* Enable RX and TX. */
			Chip_ENET_RXEnable(LPC_ETHERNET);
			Chip_ENET_TXEnable(LPC_ETHERNET);

			/* Enable interrupts in the NVIC. */
			NVIC_SetPriority( ETHERNET_IRQn, configETHERNET_INTERRUPT_PRIORITY );
			NVIC_EnableIRQ( ETHERNET_IRQn );
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

BaseType_t xLPC1758_NetworkInterfaceOutput( NetworkInterface_t *pxInterface, NetworkBufferDescriptor_t * const pxDescriptor, BaseType_t bReleaseAfterSend )
{
BaseType_t xReturn = pdFAIL;
const TickType_t xBlockTimeTicks = pdMS_TO_TICKS( 250 );
UBaseType_t ulTxProduceIndex;

	if( xLPC1758_GetPhyLinkStatus( pxInterface ) == pdPASS )
	{

		do
		{
			if( ( xTXDescriptorSemaphore == NULL ) || ( xTXMutex == NULL ) )
			{
				break;
			}
			if( xSemaphoreTake( xTXDescriptorSemaphore, xBlockTimeTicks ) != pdPASS )
			{
				/* Time-out waiting for a free TX descriptor. */
				break;
			}

			/* The semaphore was taken, so there should be a TX DMA descriptor
			available */
			configASSERT( !Chip_ENET_IsTxFull( LPC_ETHERNET ) );

			/* Ensure only one task is accessing critical section of
			xLPC1758_NetworkInterfaceOutput at the same time. */
			if( xSemaphoreTake( xTXMutex, xBlockTimeTicks ) != pdPASS )
			{
				/* Mutex access failed, release semaphore and exit. */
				xSemaphoreGive( xTXDescriptorSemaphore );
				break;
			}

			ulTxProduceIndex = Chip_ENET_GetTXProduceIndex( LPC_ETHERNET );

			#if( ipconfigZERO_COPY_TX_DRIVER != 0 )
			{
				/* bReleaseAfterSend should always be set when using the zero
				copy driver. */
				configASSERT( bReleaseAfterSend != pdFALSE );

				/* The DMA's descriptor to point directly to the data in the
				network buffer descriptor.  The data is not copied. */
				xDMATxDescriptors[ ulTxProduceIndex ].Packet = ( uint32_t ) pxDescriptor->pucEthernetBuffer;

				/* The DMA descriptor will 'own' this Network Buffer,
				until it has been sent.  So don't release it now. */
				bReleaseAfterSend = pdFALSE;
			}
			#else
			{
				/* The data is copied from the network buffer descriptor into
				the DMA's descriptor. */
				memcpy( ( void * ) xDMATxDescriptors[ ulTxProduceIndex ].Packet, ( void * ) pxDescriptor->pucEthernetBuffer, pxDescriptor->xDataLength );
			}
			#endif

			/* Set DMA descriptor data length, enable TxDone Interrupt and
			indicate it's the frame's last (and only) descriptor */
			xDMATxDescriptors[ ulTxProduceIndex ].Control = ( uint32_t ) ENET_TCTRL_SIZE( pxDescriptor->xDataLength ) | ENET_TCTRL_INT | ENET_TCTRL_LAST;

			/* Increase the current Tx Produce Descriptor Index to start transmission*/
			Chip_ENET_IncTXProduceIndex(LPC_ETHERNET);

			xSemaphoreGive( xTXMutex );

			iptraceNETWORK_INTERFACE_TRANSMIT();

			/* The Tx has been initiated. */
			xReturn = pdPASS;

		} while( 0 );
	}

	/* The buffer has been sent so can be released. */
	if( bReleaseAfterSend != pdFALSE )
	{
		vReleaseNetworkBufferAndDescriptor( pxDescriptor );
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

static void prvEMACHandlerTask( void *pvParameters )
{
const TickType_t xDescriptorWaitTime = pdMS_TO_TICKS( 250 );
const UBaseType_t uxMinimumBuffersRemaining = 2UL;

TickType_t xPhyPollTime = pdMS_TO_TICKS( PHY_LS_HIGH_CHECK_TIME_MS );
NetworkInterface_t *pxInterface = ( NetworkInterface_t *) pvParameters;
uint32_t ulNotificationValue;
eFrameProcessingResult_t eResult;
UBaseType_t ulConsumeIndex;
NetworkBufferDescriptor_t *pxDescriptor;
#if( ipconfigZERO_COPY_RX_DRIVER != 0 )
	NetworkBufferDescriptor_t *pxNewDescriptor;
#endif /* ipconfigZERO_COPY_RX_DRIVER */
size_t xDataLength;
IPStackEvent_t xIPStackEvent;

	for( ;; )
	{
		vCheckBuffersAndQueue();

		/* Take notification
		 * 0 timeout
		 * ENET_INT_RXDONE receive
		 * ENET_INT_TXDONE TX cleanup
		 * */
		ulNotificationValue = ulTaskNotifyTake( pdTRUE, xPhyPollTime );

		/* Wait until a packet has been received or PHY link status times out */

		if( ulNotificationValue == 0 )
		{
			/* A timeout happened, check PHY link status now */
			if( prvUpdatePHYLinkStatus( (TickType_t) 0 ) == pdFAIL )
			{
				xPhyPollTime = pdMS_TO_TICKS( PHY_LS_LOW_CHECK_TIME_MS );
			}
			else
			{
				xPhyPollTime = pdMS_TO_TICKS( PHY_LS_HIGH_CHECK_TIME_MS );
			}

			if( ulPHYLinkStatus & PHY_LINK_CHANGED )
			{
				/* PHY link changed, notify IP Task. */
				xIPStackEvent.eEventType = eNetworkDownEvent;
				xIPStackEvent.pvData = pxInterface;
				xSendEventStructToIPTask( &xIPStackEvent, portMAX_DELAY );
			}
		}
		else
		{
			/* Task got notified */
			if ( ( ulNotificationValue & ENET_INT_RXDONE ) != 0x00 )
			{
				/* Check if a packet has been received. */
				while( !Chip_ENET_IsRxEmpty(LPC_ETHERNET) )
				{
					ulConsumeIndex = Chip_ENET_GetRXConsumeIndex( LPC_ETHERNET );

					eResult = ipCONSIDER_FRAME_FOR_PROCESSING( ( const uint8_t * const ) ( xDMARxDescriptors[ ulConsumeIndex ].Packet ) );
					if( eResult == eProcessBuffer )
					{
						/* A packet was received, set PHY status to connected. */
						if( ( ulPHYLinkStatus & PHY_LINK_CONNECTED ) == 0 )
						{
							ulPHYLinkStatus |= PHY_LINK_CONNECTED;
							FreeRTOS_printf( ( "prvEMACHandlerTask: PHY LS now %d (message received)\n", ( ulPHYLinkStatus & PHY_LINK_CONNECTED ) != 0 ) );
							xPhyPollTime = pdMS_TO_TICKS( PHY_LS_HIGH_CHECK_TIME_MS );
						}

					#if( ipconfigZERO_COPY_RX_DRIVER != 0 )
						if( uxGetNumberOfFreeNetworkBuffers() > uxMinimumBuffersRemaining )
						{
							pxNewDescriptor = pxGetNetworkBufferWithDescriptor( ipTOTAL_ETHERNET_FRAME_SIZE, xDescriptorWaitTime );
						}
						else
						{
							/* Too risky to allocate a new Network Buffer. */
							pxNewDescriptor = NULL;
						}
						if( pxNewDescriptor != NULL )
					#else
						if( uxGetNumberOfFreeNetworkBuffers() > uxMinimumBuffersRemaining )
					#endif /* ipconfigZERO_COPY_RX_DRIVER */
						{

							/* Get the actual length. */
							xDataLength = ( size_t ) ENET_RINFO_SIZE( xDMARxStatus[ ulConsumeIndex ].StatusInfo ) - 4; /* Remove FCS */

							#if( ipconfigZERO_COPY_RX_DRIVER != 0 )
							{
								/* Obtain the associated network buffer to pass this
								data into the stack. */
								pxDescriptor = pxPacketBuffer_to_NetworkBuffer( ( uint8_t * ) xDMARxDescriptors[ ulConsumeIndex ].Packet );
								/* This zero-copy driver makes sure that every 'xDMARxDescriptors' contains
								a reference to a Network Buffer at any time.
								In case it runs out of Network Buffers, a DMA buffer won't be replaced,
								and the received messages is dropped. */
								configASSERT( pxDescriptor != NULL );

								/* Assign the new Network Buffer to the DMA descriptor. */
								xDMARxDescriptors[ ulConsumeIndex ].Packet = ( uint32_t ) pxNewDescriptor->pucEthernetBuffer;
							}
							#else
							{
								/* Create a buffer of exactly the required length. */
								pxDescriptor = pxGetNetworkBufferWithDescriptor( xDataLength, xDescriptorWaitTime );
							}
							#endif /* ipconfigZERO_COPY_RX_DRIVER */

							if( pxDescriptor != NULL )
							{

								/* Update the the length of the network buffer descriptor
								with the number of received bytes */
								pxDescriptor->xDataLength = xDataLength;
								#if( ipconfigZERO_COPY_RX_DRIVER == 0 )
								{
									/* Copy the data into the allocated buffer. */
									memcpy( ( void * ) pxDescriptor->pucEthernetBuffer, ( void * ) xDMARxDescriptors[ ulConsumeIndex ].Packet, usLength );
								}
								#endif /* ipconfigZERO_COPY_RX_DRIVER */

								/* Set the receiving interface in the network buffer descriptor */
								pxDescriptor->pxInterface = pxInterface;

							#if( ipconfigUSE_BRIDGE != 0 )
								if( pxInterface->bits.bIsBridged )
								{
									if( xBridge_Process( pxDescriptor ) == pdFAIL )
									{
										/* The Bridge could not process the descriptor,
										it must be released. */
										vReleaseNetworkBufferAndDescriptor( pxDescriptor );
										iptraceETHERNET_RX_EVENT_LOST();
									}
								}
								else
							#endif
								{
									/* Pass the data to the TCP/IP task for processing. */
									xIPStackEvent.eEventType = eNetworkRxEvent;
									xIPStackEvent.pvData = ( void * ) pxDescriptor;
									if( xSendEventStructToIPTask( &xIPStackEvent, xDescriptorWaitTime ) == pdFAIL )
									{
										/* Could not send the descriptor into the TCP/IP
										stack, it must be released. */
										vReleaseNetworkBufferAndDescriptor( pxDescriptor );
										iptraceETHERNET_RX_EVENT_LOST();
									}
									else
									{
										iptraceNETWORK_INTERFACE_RECEIVE();

										/* The data that was available at the top of this
										loop has been sent, so is no longer available. */
									}
								}
							}
						}
						else
						{
							iptraceETHERNET_RX_EVENT_LOST();
						}
					}

					/* Release the DMA descriptor. */
					Chip_ENET_IncRXConsumeIndex( LPC_ETHERNET );
				}
			} /* ENET_INT_RXDONE */
			if ( ( ulNotificationValue & ENET_INT_TXDONE ) != 0x00 )
			{
				/* TX needs cleanup. */
				ulConsumeIndex = Chip_ENET_GetTXConsumeIndex( LPC_ETHERNET );
				while( ulTxCleanupIndex != ulConsumeIndex )
				{
					#if( ipconfigZERO_COPY_TX_DRIVER != 0 )
					{
						/* Obtain the associated network buffer to release it. */
						pxDescriptor = pxPacketBuffer_to_NetworkBuffer( ( uint8_t * ) xDMATxDescriptors[ ulTxCleanupIndex ].Packet );
						/* This zero-copy driver makes sure that every 'xDMARxDescriptors' contains
						a reference to a Network Buffer at any time.
						In case it runs out of Network Buffers, a DMA buffer won't be replaced,
						and the received messages is dropped. */
						configASSERT( pxDescriptor != NULL );

						vReleaseNetworkBufferAndDescriptor( pxDescriptor ) ;
						xDMATxDescriptors[ ulTxCleanupIndex ].Packet = ( uint32_t )0u;
					}
					#endif /* ipconfigZERO_COPY_TX_DRIVER */

					/* Tell the counting semaphore that one more TX descriptor is available. */
					xSemaphoreGive( xTXDescriptorSemaphore );

					/* Advance to the next descriptor, wrapping if necessary */
					++ulTxCleanupIndex;
					if( ulTxCleanupIndex >= ipconfigNUM_TX_DESCRIPTORS )
					{
						ulTxCleanupIndex = 0;
					}
				}
			}
		}

	}
}
/*-----------------------------------------------------------*/

void NETWORK_IRQHandler( void )
{
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
uint32_t ulInterrupts;

	configASSERT( xEMACTaskHandle );

	/* Get pending interrupts. */
	ulInterrupts = Chip_ENET_GetIntStatus(LPC_ETHERNET);

	/* RX done interrupt. */
	if( ( ulInterrupts & ENET_INT_RXDONE ) != 0x00 )
	{
		xTaskNotifyFromISR( xEMACTaskHandle, ENET_INT_RXDONE, eSetBits, &xHigherPriorityTaskWoken );
	}
	/* TX done interrupt. */
	if( ( ulInterrupts & ENET_INT_TXDONE ) != 0x00 )
	{
		xTaskNotifyFromISR( xEMACTaskHandle, ENET_INT_TXDONE, eSetBits, &xHigherPriorityTaskWoken );
	}

	// TODO: Handle Error interrupts like ENET_INT_RXOVERRUN and ENET_INT_RXERROR

	/* Clear pending interrupts. */
	Chip_ENET_ClearIntStatus(LPC_ETHERNET, ulInterrupts);

	/* Context switch needed? */
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}
/*-----------------------------------------------------------*/


NetworkInterface_t *pxLPC1758_FillInterfaceDescriptor( BaseType_t xIndex, NetworkInterface_t *pxInterface )
{
static char pcName[ 8 ];
/* This function pxLPC1758_FillInterfaceDescriptor() adds a network-interface.
Make sure that the object pointed to by 'pxInterface'
is declared static or global, and that it will remain to exist. */

	snprintf( pcName, sizeof( pcName ), "eth%ld", xIndex );

	memset( pxInterface, '\0', sizeof( *pxInterface ) );
	pxInterface->pcName				= pcName;					/* Just for logging, debugging. */
	pxInterface->pvArgument			= (void*) pxInterface;		/* Has only meaning for the driver functions. */
	pxInterface->pfInitialise		= xLPC1758_NetworkInterfaceInitialise;
	pxInterface->pfOutput			= xLPC1758_NetworkInterfaceOutput;
	pxInterface->pfGetPhyLinkStatus = xLPC1758_GetPhyLinkStatus;

	return pxInterface;
}
