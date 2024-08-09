/*
 * Copyright (c) 2016-2024, devolo solutions GmbH, Aachen, Germany.
 * All rights reserved.
 *
 * This Software is part of the devolo GreenPHY-SDK.
 *
 * Usage in source form and redistribution in binary form, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Usage in source form is subject to a current end user license agreement
 *    with the devolo solutions GmbH.
 * 2. Neither the name of the devolo solutions GmbH nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 3. Redistribution in binary form is limited to the usage on the GreenPHY
 *    module of the devolo solutions GmbH.
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

/* Standard includes. */
#include <string.h>

/* LPCOpen Includes. */
#include "board.h"

/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include "task.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_Routing.h"

/* GreenPHY SDK includes. */
#include "GreenPhySDKConfig.h"

/* Project includes. */
#include "http_query_parser.h"

BaseType_t xRequestHandler_Status( char *pcBuffer, size_t uxBufferLength, QueryParam_t *pxParams, BaseType_t xParamCount )
{
BaseType_t xCount = 0;
QueryParam_t *pxParam;
NetworkEndPoint_t *pxEndPoint;
uint32_t ulIPAddress, ulNetMask, ulGatewayAddress, ulDNSServerAddress;
int8_t cBuffer[ 16 ];

	pxParam = pxFindKeyInQueryParams( "reset", pxParams, xParamCount );
	if( pxParam != NULL )
	{
		NVIC_SystemReset();
	}

	pxParam = pxFindKeyInQueryParams( "led", pxParams, xParamCount );
	if( pxParam != NULL )
	{
		if( strcmp(pxParam->pcValue, "on") == 0 )
		{
			Board_LED_Set( LEDS_LED0, TRUE );
		}
		else if( strcmp(pxParam->pcValue, "off") == 0 )
		{
			Board_LED_Set( LEDS_LED0, FALSE );
		}
		else if( strcmp(pxParam->pcValue, "toggle") == 0 )
		{
			Board_LED_Toggle( LEDS_LED0 );
		}
	}

	xCount = snprintf( pcBuffer, uxBufferLength,
			"{"
				"\"uptime\":"    "%d,"
				"\"free_heap\":" "%d,"
				"\"led\":"       "%s,"
				"\"button\":"    "%s,"
				"\"build\":"     "\"%s\","
				"\"hostname\":"  "\"%s\",",
			( portGET_RUN_TIME_COUNTER_VALUE() / 10000UL ),
			xPortGetFreeHeapSize(),
			( Board_LED_Test( LEDS_LED0 ) ? "true" : "false" ),
			( ( Buttons_GetStatus() != 0 ) ? "true" : "false" ),
			BUILD_STRING,
			pcApplicationHostnameHook()
	);

	pxEndPoint = FreeRTOS_FirstEndPoint( NULL );
	if( pxEndPoint != NULL )
	{
		xCount += snprintf( pcBuffer + xCount, uxBufferLength - xCount,
				"\"mac\":\"%02x:%02x:%02x:%02x:%02x:%02x\",",
				pxEndPoint->xMACAddress.ucBytes[0],
				pxEndPoint->xMACAddress.ucBytes[1],
				pxEndPoint->xMACAddress.ucBytes[2],
				pxEndPoint->xMACAddress.ucBytes[3],
				pxEndPoint->xMACAddress.ucBytes[4],
				pxEndPoint->xMACAddress.ucBytes[5]
		);

		FreeRTOS_GetEndPointConfiguration( &ulIPAddress,
										   &ulNetMask,
										   &ulGatewayAddress,
										   &ulDNSServerAddress,
										   pxEndPoint );

		FreeRTOS_inet_ntoa( ulIPAddress, cBuffer );
		xCount += snprintf( pcBuffer + xCount, uxBufferLength - xCount, "\"ip\":\"%s\",", cBuffer);
		FreeRTOS_inet_ntoa( ulNetMask, cBuffer );
		xCount += snprintf( pcBuffer + xCount, uxBufferLength - xCount, "\"netmask\":\"%s\",", cBuffer);
		FreeRTOS_inet_ntoa( ulGatewayAddress, cBuffer );
		xCount += snprintf( pcBuffer + xCount, uxBufferLength - xCount, "\"gateway\":\"%s\",", cBuffer);
		FreeRTOS_inet_ntoa( ulDNSServerAddress, cBuffer );
		xCount += snprintf( pcBuffer + xCount, uxBufferLength - xCount, "\"dns\":\"%s\"", cBuffer);
	}

	xCount += snprintf( pcBuffer + xCount, uxBufferLength - xCount, "}" );


	return xCount;
}
