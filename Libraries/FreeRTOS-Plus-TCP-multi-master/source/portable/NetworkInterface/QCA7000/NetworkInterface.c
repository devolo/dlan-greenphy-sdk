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

/* LPCOpen includes. */
#include "board.h"

/* QCA7k includes */
#include "qca_7k.h"

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
#include "NetworkBufferManagement.h"

#if( ipconfigREAD_MAC_FROM_GREENPHY != 0 )
	#include "mme_handler.h"
	#include "qca_vs_mme.h"
	extern void vUpdateHostname( NetworkEndPoint_t *pxEndPoint );
#endif

/* If ipconfigETHERNET_DRIVER_FILTERS_FRAME_TYPES is set to 1, then the Ethernet
 driver will filter incoming packets and only pass the stack those packets it
 considers need processing. */
 #if( ipconfigETHERNET_DRIVER_FILTERS_FRAME_TYPES == 0 )
 	#define ipCONSIDER_FRAME_FOR_PROCESSING( pucEthernetBuffer ) eProcessBuffer
 #else
 	#define ipCONSIDER_FRAME_FOR_PROCESSING( pucEthernetBuffer ) eConsiderFrameForProcessing( ( pucEthernetBuffer ) )
 #endif

/*-----------------------------------------------------------*/
void GreenPHY_GPIO_IRQHandler (portBASE_TYPE * xHigherPriorityTaskWoken);
void GreenPHY_DMA_IRQHandler (portBASE_TYPE * xHigherPriorityTaskWoken);
/*-----------------------------------------------------------*/

/* The struct to hold all QCA7k and SPI related information */
static struct qcaspi qca = { 0 };

/* The handle of the task that processes Rx packets.  The handle is required so
the task can be notified when new packets arrive. */
static TaskHandle_t xGreenPHYTaskHandle = NULL;
SemaphoreHandle_t xGreenPHY_DMASemaphore;

/*-----------------------------------------------------------*/

extern void qcaspi_spi_thread(void *data);
BaseType_t xQCA7000_NetworkInterfaceInitialise( NetworkInterface_t *pxInterface )
{
BaseType_t xReturn = pdPASS;
#if( ipconfigREAD_MAC_FROM_GREENPHY != 0 )
	NetworkEndPoint_t *pxEndPoint;
	NetworkBufferDescriptor_t *pxDescriptor;
	uint16_t usMMType;
	struct CCMMEFrame *pxMMEFrame;
	BaseType_t xRetries = 5;
#endif

	if( xGreenPHYTaskHandle == NULL )
	{
		Chip_GPDMA_Init(LPC_GPDMA);
		Board_SSP_Init(LPC_SSP0, true);

		qca.SSPx = LPC_SSP0;
		qca.sync = QCASPI_SYNC_UNKNOWN;
		qca.txQueue = xQueueCreate( ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS, sizeof( NetworkBufferDescriptor_t *) );
		qca.rx_desc = NULL;

		qca.pxInterface = pxInterface;
		QcaFrmFsmInit(&qca.lFrmHdl);
		/* When reading data over SPI, there also needs to occur
		a write process. To guard both DMA channels this semaphore
		counts to two. */
		xGreenPHY_DMASemaphore = xSemaphoreCreateCounting( 2u, 0u);

		/* QCA7000 reset pin setup */
		Chip_GPIO_SetPinDIROutput(LPC_GPIO, GREENPHY_RESET_GPIO_PORT, GREENPHY_RESET_GPIO_PIN);

		xTaskCreate( qcaspi_spi_thread, pxInterface->pcName, 240, &qca, tskIDLE_PRIORITY+4, &xGreenPHYTaskHandle);

		{
			registerInterruptHandlerGPIO(GREENPHY_INT_PORT, GREENPHY_INT_PIN, GreenPHY_GPIO_IRQHandler);
		}

		/* Wait for sync. */
		vTaskDelay( pdMS_TO_TICKS( 6000 ) );

		#if( ipconfigREAD_MAC_FROM_GREENPHY != 0 )
		{
			while( xRetries-- > 0 )
			{
				/* Create GetSwVersion MME */
				pxDescriptor = qcaspi_create_get_sw_version_mme( pxInterface, &usMMType );

				if( pxDescriptor != NULL ) {
					/* Send MME and receive answer */
					pxDescriptor = send_receive_mme_blocking( pxInterface, pxDescriptor, usMMType );

					if( pxDescriptor != NULL ) {
						/* Received MME, extract MAC. */
						pxMMEFrame = (struct CCMMEFrame *) pxDescriptor->pucEthernetBuffer;
						pxEndPoint = FreeRTOS_FirstEndPoint( pxInterface );
						if( pxEndPoint == NULL )
						{
							pxEndPoint = FreeRTOS_FirstEndPoint( NULL );
						}
						if( pxEndPoint != NULL )
						{
							memcpy( pxEndPoint->xMACAddress.ucBytes, pxMMEFrame->mOSA, sizeof( MACAddress_t ) );
							/* Toggle 2nd bit of first byte to indicate a locally administered MAC. */
							pxEndPoint->xMACAddress.ucBytes[0] ^= (1 << 1);
							vUpdateHostname( pxEndPoint );
						}

						/* Release the Network Buffer, as we are responsible for it. */
						vReleaseNetworkBufferAndDescriptor( pxDescriptor );
						xRetries = 0;
					}
				}
			}
		}
		#endif

		pxInterface->bits.bInterfaceInitialised = pdTRUE_UNSIGNED;

	}

	return xReturn;
}
/*-----------------------------------------------------------*/

BaseType_t xQCA7000_NetworkInterfaceOutput( NetworkInterface_t *pxInterface, NetworkBufferDescriptor_t * const pxDescriptor, BaseType_t bReleaseAfterSend )
{
BaseType_t xReturn = pdFAIL;

	xReturn = xQueueSend(qca.txQueue, ( void * ) &pxDescriptor, 0);
	xTaskNotify( xGreenPHYTaskHandle, QCAGP_TX_FLAG, eSetBits );

	return xReturn;
}
/*-----------------------------------------------------------*/

static BaseType_t xQCA7000_GetPhyLinkStatus( NetworkInterface_t *pxInterface )
{
BaseType_t xReturn = pdPASS;

	/* _ML_ The link is maintained by the QCA7k chip itself, is
	there a simple way to figure out whether the device is paired
	or not? */

	if( pxInterface->bits.bInterfaceInitialised == pdFALSE_UNSIGNED )
	{
		xReturn = pdFAIL;
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

NetworkInterface_t *pxQCA7000_FillInterfaceDescriptor( BaseType_t xIndex, NetworkInterface_t *pxInterface )
{
static char pcName[ 8 ];
/* This function pxQCA7000_FillInterfaceDescriptor() adds a network-interface.
Make sure that the object pointed to by 'pxInterface'
is declared static or global, and that it will remain to exist. */

	snprintf( pcName, sizeof( pcName ), "plc%ld", xIndex );

	memset( pxInterface, '\0', sizeof( *pxInterface ) );
	pxInterface->pcName				= pcName;					/* Just for logging, debugging. */
	pxInterface->pvArgument			= (void*) pxInterface;		/* Has only meaning for the driver functions. */
	pxInterface->pfInitialise		= xQCA7000_NetworkInterfaceInitialise;
	pxInterface->pfOutput			= xQCA7000_NetworkInterfaceOutput;
	pxInterface->pfGetPhyLinkStatus = xQCA7000_GetPhyLinkStatus;

	return pxInterface;
}

/*-----------------------------------------------------------*/
void GreenPHY_GPIO_IRQHandler (portBASE_TYPE * xHigherPriorityTaskWoken)
{
	/* Unregister GPIO Interrupt until the task handled it. */
	unregisterInterruptHandlerGPIO(GREENPHY_INT_PORT, GREENPHY_INT_PIN);

	/* wake up the handler task */
	xTaskNotifyFromISR( xGreenPHYTaskHandle,  QCAGP_INT_FLAG, eSetBits, xHigherPriorityTaskWoken );

	/* Interrupt status clear and context switching
	 * is done by GPIO Interrupt handler */
}
/*-----------------------------------------------------------*/

void GreenPHY_DMA_IRQHandler (portBASE_TYPE * xHigherPriorityTaskWoken)
{
	xSemaphoreGiveFromISR( xGreenPHY_DMASemaphore, xHigherPriorityTaskWoken );
}
/*-----------------------------------------------------------*/

