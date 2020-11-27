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

/*-----------------------------------------------------------*/

/* Exclude the entire file if bridge support is not enabled. */
#if( ipconfigUSE_BRIDGE != 0 )

static BaseType_t xBridge_NetworkInterfaceInitialise( NetworkInterface_t *pxInterface )
{
BaseType_t xReturn = pdPASS;
NetworkInterface_t *pxBridgedInterface;
NetworkEndPoint_t *pxEndPoint;

	/* Do not initialise the bridge interface unless all bridged interfaces are up. */
	for( pxBridgedInterface = FreeRTOS_FirstNetworkInterfaceInBridge();
		 pxBridgedInterface != NULL;
		 pxBridgedInterface = FreeRTOS_NextNetworkInterfaceInBridge( pxBridgedInterface ) )
	{
		if( pxBridgedInterface->bits.bInterfaceInitialised == pdFALSE_UNSIGNED  )
		{
			xReturn = pdFAIL;
			break;
		}
	}

	if( xReturn != pdFAIL )
	{
		pxEndPoint = FreeRTOS_FirstEndPoint( pxInterface );
		if( pxEndPoint != NULL )
		{
			/* Add the EndPoints MAC address to the forwarding table. */
			vRefreshForwardingTableEntry( &pxEndPoint->xMACAddress, pxInterface, INFINITE_FORWARDING_TABLE_AGE );
		}
		pxInterface->bits.bInterfaceInitialised = pdTRUE_UNSIGNED;
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

static BaseType_t xBridge_NetworkInterfaceOutput( NetworkInterface_t *pxInterface, NetworkBufferDescriptor_t * const pxNetworkBuffer, BaseType_t bReleaseAfterSend )
{
const TickType_t xDescriptorWaitTime = pdMS_TO_TICKS( 250 );
BaseType_t xReturn = pdFAIL;
IPStackEvent_t xRxEvent = { eNetworkRxEvent, NULL };

	if( pxNetworkBuffer->pxInterface != NULL )
	{
		/* Frame coming in from Bridge itself, pass it to the
		TCP/IP task for processing. */
		pxNetworkBuffer->pxInterface = pxInterface;
		xRxEvent.pvData = ( void * ) pxNetworkBuffer;
		xReturn = xSendEventStructToIPTask( &xRxEvent, xDescriptorWaitTime );
		if( xReturn == pdFAIL )
		{
			if( bReleaseAfterSend == pdTRUE )
			{
				/* Could not send the descriptor into the TCP/IP stack,
				it must be released. */
				vReleaseNetworkBufferAndDescriptor( pxNetworkBuffer );
				iptraceETHERNET_RX_EVENT_LOST();
			}
		}
		else
		{
			iptraceNETWORK_INTERFACE_RECEIVE();
		}
	}
	else
	{
		/* Frame coming from the TCP/IP task, let the Bridge process it.
		The interface is set, so the Bridge won't return the buffer to
		this interface. */
		pxNetworkBuffer->pxInterface = pxInterface;
		xReturn = xBridge_Process( pxNetworkBuffer );
		if( xReturn == pdFAIL )
		{
			if( bReleaseAfterSend == pdTRUE )
			{
				/* The Bridge could not process the descriptor,
				it must be released. */
				vReleaseNetworkBufferAndDescriptor( pxNetworkBuffer );
				iptraceETHERNET_RX_EVENT_LOST();
			}
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

static BaseType_t xBridge_GetPhyLinkStatus( NetworkInterface_t *pxInterface )
{
BaseType_t xReturn = pdPASS;

	return xReturn;
}
/*-----------------------------------------------------------*/

NetworkInterface_t *pxBridge_FillInterfaceDescriptor( BaseType_t xIndex, NetworkInterface_t *pxInterface )
{
static char pcName[ 8 ];
/* This function pxBridge_FillInterfaceDescriptor() adds a network-interface.
Make sure that the object pointed to by 'pxInterface'
is declared static or global, and that it will remain to exist. */

	snprintf( pcName, sizeof( pcName ), "br%ld", xIndex );

	memset( pxInterface, '\0', sizeof( *pxInterface ) );
	pxInterface->pcName				= pcName;					/* Just for logging, debugging. */
	pxInterface->pvArgument			= (void*) pxInterface;		    /* Has only meaning for the driver functions. */
	pxInterface->pfInitialise		= xBridge_NetworkInterfaceInitialise;
	pxInterface->pfOutput			= xBridge_NetworkInterfaceOutput;
	pxInterface->pfGetPhyLinkStatus = xBridge_GetPhyLinkStatus;

	/* Bridge Interfaces are only connected to the IP Task,
	when the Interface is initialised the EndPoint's MAC is
	added to the forwarding table. */
	pxInterface->bits.bForwardingTableKnown = 1;

	return pxInterface;
}
/*-----------------------------------------------------------*/

#endif /* ipconfigUSE_BRIDGE != 0 */
