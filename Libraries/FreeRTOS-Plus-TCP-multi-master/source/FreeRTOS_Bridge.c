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

/* Exclude the entire file if bridge support is not enabled. */
#if( ipconfigUSE_BRIDGE != 0 )

#if( ipconfigUSE_FORWARDING_TABLE != 0 )

typedef struct xFORWARDING_TABLE_ROW
{
	struct xNetworkInterface *pxInterface;		/* The Network Interface of a forwarding table entry. */
	MACAddress_t xMACAddress;                   /* The MAC address of a forwarding table entry. */
	uint8_t ucAge;				                /* A value that is periodically decremented but can also be refreshed by active communication. */
} ForwardingTableRow_t;

static UBaseType_t uxForwardTableMaxUse = 0;
static ForwardingTableRow_t xForwardingTable[ipconfigFORWARDING_TABLE_ENTRIES];

NetworkInterface_t *pxFindInterfaceOnMAC( const MACAddress_t *pxMACAddress )
{
BaseType_t x;
NetworkInterface_t *pxInterface = NULL;

	for( x = 0; x < uxForwardTableMaxUse; x++ )
	{
		if( ( xForwardingTable->pxInterface != NULL )
			&& ( memcmp( xForwardingTable[x].xMACAddress.ucBytes, pxMACAddress->ucBytes, ipMAC_ADDRESS_LENGTH_BYTES ) ) == 0 )
		{
			/* Found. */
			pxInterface =  xForwardingTable[x].pxInterface;
			break;
		}
	}

	return pxInterface;
}
/*-----------------------------------------------------------*/

void vRefreshForwardingTableEntry( const MACAddress_t *pxMACAddress, NetworkInterface_t *pxInterface, uint8_t ucAge )
{
BaseType_t xUseEntry, xOldestEntry;
uint8_t ucMinAgeFound = 0;

	/* Start with the maximum possible number. */
	ucMinAgeFound--;

	/* Find the first unused entry or one with the same MAC. */
	for( xUseEntry = 0; xUseEntry < uxForwardTableMaxUse; xUseEntry++ )
	{
		if( xForwardingTable[xUseEntry].pxInterface == NULL )
		{
			/* Unused entry, save MAC here so the table stays compact.
			A previously used entry of this MAC may be left for time out. */
			break;
		}
		if( memcmp( xForwardingTable[xUseEntry].xMACAddress.ucBytes, pxMACAddress->ucBytes, ipMAC_ADDRESS_LENGTH_BYTES ) == 0 )
		{
			/* Found MAC, no need to copy it. */
			pxMACAddress = NULL;
			break;
		}

		/* Look for the oldest entry, in case the table is full */
		if( xForwardingTable[xUseEntry].ucAge < ucMinAgeFound )
		{
			ucMinAgeFound = xForwardingTable[xUseEntry].ucAge;
			xOldestEntry = xUseEntry;
		}
	}

	if( xUseEntry == uxForwardTableMaxUse )
	{
		if( xUseEntry == ( sizeof( xForwardingTable ) / sizeof( xForwardingTable[0] ) ) )
		{
			/* The table is full, replace the oldest entry */
			xUseEntry = xOldestEntry;
		}
		else
		{
			/* A new row is used, increase the usage counter. */
			uxForwardTableMaxUse++;
		}
	}

	if( pxMACAddress != NULL )
	{
		memcpy( xForwardingTable[xUseEntry].xMACAddress.ucBytes, pxMACAddress->ucBytes, ipMAC_ADDRESS_LENGTH_BYTES );
	}
	xForwardingTable[xUseEntry].pxInterface = pxInterface;
	xForwardingTable[xUseEntry].ucAge = ucAge;
}
/*-----------------------------------------------------------*/

void vAgeForwardingTable( void )
{
BaseType_t x;

	/* Loop through each entry in the ARP cache. */
	for( x = 0; x < uxForwardTableMaxUse; x++ )
	{
		if( xForwardingTable[ x ].ucAge != INFINITE_FORWARDING_TABLE_AGE )
		{
			/* Decrement the age value of the entry in this table row.
			When the age reaches zero it is no longer considered valid. */
			xForwardingTable[ x ].ucAge--;

			if( xForwardingTable[ x ].ucAge == 0u )
			{
				/* The entry is no longer valid.  Wipe it out. */
				xForwardingTable[ x ].pxInterface = NULL;
			}
		}
	}
}
/*-----------------------------------------------------------*/

#endif /* ipconfigUSE_FORWARDING_TABLE != 0 */

NetworkInterface_t *FreeRTOS_FirstNetworkInterfaceInBridge( void )
{
NetworkInterface_t *pxInterface = FreeRTOS_FirstNetworkInterface();

	while( pxInterface != NULL )
	{
		if( pxInterface->bits.bIsBridged != 0 )
		{
			break;
		}
		pxInterface = pxInterface->pxNext;
	}

	return pxInterface;
}
/*-----------------------------------------------------------*/

NetworkInterface_t *FreeRTOS_NextNetworkInterfaceInBridge( NetworkInterface_t *pxInterface )
{
	while( pxInterface != NULL )
	{
		pxInterface = pxInterface->pxNext;
		if( pxInterface->bits.bIsBridged != 0 )
		{
			break;
		}
	}

	return pxInterface;
}
/*-----------------------------------------------------------*/

BaseType_t xBridge_Process( NetworkBufferDescriptor_t * const pxNetworkBuffer )
{
BaseType_t xReturn = pdFAIL;
#if( ipconfigUSE_FORWARDING_TABLE != 0)
	EthernetHeader_t *pxEthernetHeader;
#endif
NetworkInterface_t *pxInterface;
NetworkInterface_t *pxSendToInterface = NULL;
NetworkBufferDescriptor_t *pxNetworkBufferDuplicate;
BaseType_t xIsBroadcast = pdFALSE;

	/* The receiving interface must be set */
	configASSERT( pxNetworkBuffer->pxInterface );

	#if( ipconfigUSE_FORWARDING_TABLE != 0)
	{
		pxEthernetHeader = ( EthernetHeader_t * ) ( pxNetworkBuffer->pucEthernetBuffer );

		if( memcmp( ( void * ) xBroadcastMACAddress.ucBytes, ( void * ) pxEthernetHeader->xDestinationAddress.ucBytes, sizeof( MACAddress_t ) ) != 0 )
		{
			/* No broadcast, try to find the correct interface in the forwarding table. */
			pxSendToInterface = pxFindInterfaceOnMAC( &(pxEthernetHeader->xDestinationAddress) );
		}
		else
		{
			xIsBroadcast = pdTRUE;
		}

		if( pxNetworkBuffer->pxInterface->bits.bForwardingTableKnown == 0 )
		{
			/* Update the forwarding table with the source address of the
			received frame. */
			vRefreshForwardingTableEntry( &(pxEthernetHeader->xSourceAddress), pxNetworkBuffer->pxInterface, ipconfigMAX_FORWARDING_TABLE_AGE );
		}
	}
	#endif /* ipconfigUSE_FORWARDING_TABLE != 0 */

	if( pxSendToInterface == NULL )
	{
		/* Send frame to all interfaces except the receiving one. */

		for( pxInterface = FreeRTOS_FirstNetworkInterface();
			 pxInterface != NULL;
			 pxInterface = FreeRTOS_NextNetworkInterfaceInBridge( pxInterface ) )
		{
			/* Do not send to Interfaces whose forwarding table is fully known,
			unless it's a broadcast packet.
			Do not send back to the receiving interface.
			Also check if the interface's link is up. */
			if( ( pxInterface->bits.bForwardingTableKnown == 0 || xIsBroadcast )
				&& ( pxInterface != pxNetworkBuffer->pxInterface )
				&& ( pxInterface->pfGetPhyLinkStatus( pxInterface ) == pdPASS ) )
			{
				/* Store the interface, so the NetworkBuffer is only
				duplicated when necessary. */
				if( pxSendToInterface == NULL )
				{
					pxSendToInterface = pxInterface;
				}
				else
				{
					pxNetworkBufferDuplicate = pxDuplicateNetworkBufferWithDescriptor( pxNetworkBuffer, pxNetworkBuffer->xDataLength );
					if( pxNetworkBufferDuplicate != NULL )
					{
						iptraceBRIDGE_FORWARD_PACKET( pxNetworkBuffer, pxInterface );
						pxInterface->pfOutput( pxInterface, pxNetworkBufferDuplicate, pdTRUE );
					}
					else
					{
						/* Unable to duplicate network buffer. */
						iptraceSTACK_TX_EVENT_LOST();
					}
				}
			}
		}
	}

	if( ( pxSendToInterface != NULL ) && ( pxSendToInterface != pxNetworkBuffer->pxInterface ) )
	{
		iptraceBRIDGE_FORWARD_PACKET( pxNetworkBuffer, pxSendToInterface );
		pxSendToInterface->pfOutput( pxSendToInterface, pxNetworkBuffer, pdTRUE );
		xReturn = pdPASS;
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

#endif /* ipconfigUSE_BRIDGE != 0 */
