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

/* LPCOpen includes. */
#include "board.h"

/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include "task.h"

/* FreeRTOS +TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_Routing.h"
#include "FreeRTOS_HTTP_server.h"

/* GreenPHY SDK includes. */
#include "GreenPhySDKNetConfig.h"
#include "network.h"
#include "mqtt.h"
#include "save_config.h"


/* Verify network configuration is sane. */
#if( ( netconfigUSE_BRIDGE == 0 ) && ( netconfigUSE_IP != 0 ) && ( netconfigIP_INTERFACE == netconfigBRIDGE_INTERFACE ) )
	#error "netconfigBRIDGE_INTERFACE used for IP stack, but netconfigUSE_BRIDGE not defined"
#endif

#if( ( netconfigUSE_IP != 0 ) && ( netconfigIP_INTERFACE != netconfigETH_INTERFACE ) \
                              && ( netconfigIP_INTERFACE != netconfigPLC_INTERFACE ) \
                              && ( netconfigIP_INTERFACE != netconfigBRIDGE_INTERFACE ) )
	#error "netconfigUSE_IP defined, but no netconfigIP_INTERFACE"
#endif

#if( ( ipconfigREAD_MAC_FROM_GREENPHY != 0 ) && ( netconfigIP_INTERFACE == netconfigETH_INTERFACE ) )
	#error "ipconfigREAD_MAC_FROM_GREENPHY defined, but not supported for use with ethernet interface only. \
			Please set netconfigIP_INTERFACE to netconfigPLC_INTERFACE or netconfigBRIDGE_INTERFACE or disable \
			ipconfigREAD_MAC_FROM_GREENPHY."
#endif

/* Storage for the network interfaces in use. */
#if( ( netconfigUSE_BRIDGE != 0 ) || ( ( netconfigUSE_IP != 0 ) && ( netconfigIP_INTERFACE == netconfigETH_INTERFACE ) ) )
	static NetworkInterface_t xEthInterface    = { 0 };
#endif
#if( ( netconfigUSE_BRIDGE != 0 ) || ( ( netconfigUSE_IP != 0 ) && ( netconfigIP_INTERFACE == netconfigPLC_INTERFACE ) ) )
	static NetworkInterface_t xPlcInterface    = { 0 };
#endif
#if( netconfigUSE_BRIDGE != 0 )
	static NetworkInterface_t xBridgeInterface = { 0 };
#endif

#if( netconfigUSE_IP != 0 )
	static NetworkEndPoint_t  xEndPoint        = { 0 };

	static const uint8_t ucIPAddress[ 4 ]        = { netconfigIP_ADDR0,         netconfigIP_ADDR1,         netconfigIP_ADDR2,         netconfigIP_ADDR3 };
	static const uint8_t ucNetMask[ 4 ]          = { netconfigNET_MASK0,        netconfigNET_MASK1,        netconfigNET_MASK2,        netconfigNET_MASK3 };
	static const uint8_t ucGatewayAddress[ 4 ]   = { netconfigGATEWAY_ADDR0,    netconfigGATEWAY_ADDR1,    netconfigGATEWAY_ADDR2,    netconfigGATEWAY_ADDR3 };
	static const uint8_t ucDNSServerAddress[ 4 ] = { netconfigDNS_SERVER_ADDR0, netconfigDNS_SERVER_ADDR1, netconfigDNS_SERVER_ADDR2, netconfigDNS_SERVER_ADDR3 };

	const uint8_t ucMACAddress[ 6 ] = { netconfigMAC_ADDR0, netconfigMAC_ADDR1, netconfigMAC_ADDR2, netconfigMAC_ADDR3, netconfigMAC_ADDR4, netconfigMAC_ADDR5 };
#endif

#if( netconfigUSE_DYNAMIC_HOSTNAME != 0 )
	static char hostname[] = "devolo-000";
#endif


/*-----------------------------------------------------------*/

#if( netconfigUSE_IP != 0 )
	static void prvServerWorkTask( void *pvParameters )
	{
	const TickType_t xInitialBlockTime = pdMS_TO_TICKS( 200UL );
	HTTPServer_t *pxHTTPServer = NULL;

	/* A structure that defines the servers to be created.  Which servers are
	included in the structure depends on the mainCREATE_HTTP_SERVER and
	mainCREATE_FTP_SERVER settings at the top of this file. */
	static const struct xSERVER_CONFIG xServerConfiguration =

		/* Server type,		port number,	backlog, 	root dir. */
		{ 					80, 			0, 		"" }
	;

		/* Remove compiler warning about unused parameter. */
		( void ) pvParameters;

		/* Create the servers defined by the xServerConfiguration array above. */
		pxHTTPServer = FreeRTOS_CreateHTTPServer( &xServerConfiguration );
		configASSERT( pxHTTPServer );

		for( ;; )
		{
			FreeRTOS_HTTPServerWork( pxHTTPServer, xInitialBlockTime );
		}
	}
#endif
/*-----------------------------------------------------------*/

void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent, NetworkEndPoint_t *pxEndPoint  ) {
	#if( netconfigUSE_IP != 0 )
	{
	uint32_t ulIPAddress, ulNetMask, ulGatewayAddress, ulDNSServerAddress;
	static BaseType_t xTasksAlreadyCreated = pdFALSE;
	int8_t cBuffer[ 16 ];

		/* Check this was a network up event, as opposed to a network down event. */
		DEBUGOUT("Network hook\r\n");
		if( eNetworkEvent == eNetworkUp )
		{
			DEBUGOUT("Network up\r\n");
			/* Create the tasks that use the TCP/IP stack if they have not already been
			created. */
			if( xTasksAlreadyCreated == pdFALSE )
			{
				/*
				 * Create the tasks here.
				 */
				vStartEthTasks();

			#if( netconfigUSEMQTT != 0 )
				char *cMqtt = NULL;
				cMqtt = (char *)pvGetConfig( eConfigNetworkMqttOnPwr, NULL );
				if( (*cMqtt) > 0 )
				{
					// Start MQTT Task
					xInitMQTT();
					// Wait for Task to initialize
					vTaskDelay( pdMS_TO_TICKS( 500 ) );
					// Connect MQTT Broker
					MqttJob_t xJob;
					xJob.eJobType = eConnect;
					QueueHandle_t xMqttQueue = xGetMQTTQueueHandle();
					if( xMqttQueue != NULL )
						xQueueSendToBack( xMqttQueue, &xJob, 0 );
				}
			#endif /* #if( netconfigUSEMQTT != 0 ) */

				#define	mainTCP_SERVER_STACK_SIZE						240 /* Not used in the Win32 simulator. */

				xTaskCreate( prvServerWorkTask, "SvrWork", mainTCP_SERVER_STACK_SIZE, NULL, ipconfigIP_TASK_PRIORITY - 1, NULL );


				xTasksAlreadyCreated = pdTRUE;
			}

			/* The network is up and configured.  Print out the configuration,
			which may have been obtained from a DHCP server. */
			FreeRTOS_GetAddressConfiguration( pxEndPoint,
											  &ulIPAddress,
											  &ulNetMask,
											  &ulGatewayAddress,
											  &ulDNSServerAddress );

			/* Convert the IP address to a string then print it out. */
			FreeRTOS_inet_ntoa( ulIPAddress, cBuffer );
			DEBUGOUT( "IP Address: %s\r\n", cBuffer );

			/* Convert the net mask to a string then print it out. */
			FreeRTOS_inet_ntoa( ulNetMask, cBuffer );
			DEBUGOUT( "Subnet Mask: %s\r\n", cBuffer );

			/* Convert the IP address of the gateway to a string then print it out. */
			FreeRTOS_inet_ntoa( ulGatewayAddress, cBuffer );
			DEBUGOUT( "Gateway IP Address: %s\r\n", cBuffer );

			/* Convert the IP address of the DNS server to a string then print it out. */
			FreeRTOS_inet_ntoa( ulDNSServerAddress, cBuffer );
			DEBUGOUT( "DNS server IP Address: %s\r\n", cBuffer );
		}
	}
	#endif /* netconfigUSE_IP */
}
/*-----------------------------------------------------------*/

const char *pcApplicationHostnameHook( void )
{
#if( netconfigUSE_DYNAMIC_HOSTNAME == 0 )
	char *hostname = netconfigHOSTNAME;
#endif

	return hostname;
}
/*-----------------------------------------------------------*/

/*_HT_ introduced this memory-check temporarily for debugging. */
BaseType_t xApplicationMemoryPermissions( uint32_t aAddress )
{
	return 0x03;
}
/*-----------------------------------------------------------*/

#if( netconfigUSE_DYNAMIC_HOSTNAME != 0 )

	void vUpdateHostname( NetworkEndPoint_t *pxEndPoint )
	{
		/* Update hostname based on MAC address. */
			snprintf( &hostname[7], (sizeof(hostname) - 7), "%x%x",
					 ( pxEndPoint->xMACAddress.ucBytes[4] & 0x0F ),
					 pxEndPoint->xMACAddress.ucBytes[5] );
	}

#endif
/*-----------------------------------------------------------*/

void vNetworkInit( void )
{
	#if( netconfigUSE_BRIDGE != 0 )
	{
		extern NetworkInterface_t *pxBridge_FillInterfaceDescriptor( BaseType_t xIndex, NetworkInterface_t *pxInterface );
		pxBridge_FillInterfaceDescriptor(0, &xBridgeInterface);
		FreeRTOS_AddNetworkInterface(&xBridgeInterface);
	}
	#endif

	#if( ( netconfigUSE_BRIDGE != 0 ) || ( ( netconfigUSE_IP != 0 ) && ( netconfigIP_INTERFACE == netconfigETH_INTERFACE ) ) )
		extern NetworkInterface_t *pxLPC1758_FillInterfaceDescriptor( BaseType_t xIndex, NetworkInterface_t *pxInterface );
		pxLPC1758_FillInterfaceDescriptor(0, &xEthInterface);
		#if( netconfigUSE_BRIDGE != 0 )
		{
			xEthInterface.bits.bIsBridged = 1;
		}
		#endif
		FreeRTOS_AddNetworkInterface(&xEthInterface);
	#endif

	#if( ( netconfigUSE_BRIDGE != 0 ) || ( ( netconfigUSE_IP != 0 ) && ( netconfigIP_INTERFACE == netconfigPLC_INTERFACE ) ) )
		extern NetworkInterface_t *pxQCA7000_FillInterfaceDescriptor( BaseType_t xIndex, NetworkInterface_t *pxInterface );
		pxQCA7000_FillInterfaceDescriptor(0, &xPlcInterface);
		#if( netconfigUSE_BRIDGE != 0)
		{
			xPlcInterface.bits.bIsBridged = 1;
		}
		#endif
		FreeRTOS_AddNetworkInterface(&xPlcInterface);
	#endif

	#if( netconfigUSE_IP != 0 )
		FreeRTOS_FillEndPoint(&xEndPoint, ucIPAddress, ucNetMask, ucGatewayAddress, ucDNSServerAddress, ucMACAddress);
		xEndPoint.bits.bIsDefault = pdTRUE_UNSIGNED;
		#if( netconfigUSE_DHCP != 0 )
		{
			xEndPoint.bits.bWantDHCP = pdTRUE_UNSIGNED;
		}
		#endif
		#if( netconfigIP_INTERFACE == netconfigETH_INTERFACE )
		{
			FreeRTOS_AddEndPoint(&xEthInterface, &xEndPoint);
		}
		#elif( netconfigIP_INTERFACE == netconfigPLC_INTERFACE )
		{
			FreeRTOS_AddEndPoint(&xPlcInterface, &xEndPoint);
		}
		#elif( netconfigIP_INTERFACE == netconfigBRIDGE_INTERFACE )
		{
			FreeRTOS_AddEndPoint(&xBridgeInterface, &xEndPoint);
		}
		#endif /* netconfigIP_INTERFACE */
	#endif /* netconfigUSE_IP */

	FreeRTOS_IPStart();
}
/*-----------------------------------------------------------*/
