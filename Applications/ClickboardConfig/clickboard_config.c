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

/* Standard includes. */
#include <stdio.h>
#include <string.h>

/* LPCOpen Includes. */
#include "board.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"

/* GreenPHY SDK includes. */
#include "GreenPhySDKConfig.h"
#include "GreenPhySDKNetConfig.h"

/* Project includes. */
#include "http_query_parser.h"
#include "http_request.h"
#include "save_config.h"
#include "clickboard_config.h"
#include "mqtt.h"

#define ARRAY_SIZE(x) ( BaseType_t ) (sizeof( x ) / sizeof( x )[ 0 ] )

/***************************************
 *** ADD YOUR OWN CLICKBOARDS BELOW. ***
 ***************************************/

/*
 * The clickboard initializer and deinitializer functions.
 */
#if ( includeCOLOR2_CLICK != 0 )
	extern BaseType_t xColor2Click_Init( const char *pcName, BaseType_t xPort );
	extern BaseType_t xColor2Click_Deinit( void );
#endif
#if ( includeTHERMO3_CLICK != 0 )
	extern BaseType_t xThermo3Click_Init( const char *pcName, BaseType_t xPort );
	extern BaseType_t xThermo3Click_Deinit( void );
#endif
#if ( includeEXPAND2_CLICK != 0 )
	extern BaseType_t xExpand2Click_Init( const char *pcName, BaseType_t xPort );
	extern BaseType_t xExpand2Click_Deinit( void );
#endif

/* An array that holds all available clickboards, so they can be activated and
deactivated through the clickboard config interface. */
static Clickboard_t pxClickboards[ ] =
{
#if( includeCOLOR2_CLICK != 0 )
	{ eClickboardIdColor2, "color2", xColor2Click_Init, xColor2Click_Deinit, eClickboardAllPorts, eClickboardInactive },
#endif
#if( includeTHERMO3_CLICK != 0 )
	{ eClickboardIdThermo3, "thermo3", xThermo3Click_Init, xThermo3Click_Deinit, eClickboardAllPorts, eClickboardInactive },
#endif
#if( includeEXPAND2_CLICK != 0 )
	{ eClickboardIdExpand2, "expand2", xExpand2Click_Init, xExpand2Click_Deinit, eClickboardAllPorts, eClickboardInactive },
#endif
};

/***************************************
 *** ADD YOUR OWN CLICKBOARDS ABOVE. ***
 ***************************************/

/*-----------------------------------------------------------*/

Clickboard_t *pxFindClickboard( char *pcName )
{
BaseType_t x;
Clickboard_t *pxClickboard = NULL;

	for( x = 0; x < ARRAY_SIZE( pxClickboards ); x++ )
	{
		if( strcmp( pxClickboards[ x ].pcName, pcName ) == 0 )
		{
			pxClickboard = &pxClickboards[ x ];
			break;
		}
	}

	return pxClickboard;
}
/*-----------------------------------------------------------*/

Clickboard_t *pxFindClickboardOnPort( BaseType_t xPort )
{
BaseType_t x;
Clickboard_t *pxClickboard = NULL;

	for( x = 0; x < ARRAY_SIZE( pxClickboards ); x++ )
	{
		if( pxClickboards[ x ].xPortsActive & xPort )
		{
			pxClickboard = &pxClickboards[ x ];
			break;
		}
	}

	return pxClickboard;
}
/*-----------------------------------------------------------*/

BaseType_t xClickboardActivate( Clickboard_t *pxClickboard, BaseType_t xPort )
{
BaseType_t xSuccess = pdFALSE;
Clickboard_t *pxClickboardOld;

	if( ( xPort > 0 ) && ( xPort <= NUM_CLICKBOARD_PORTS )
		&& ( pxClickboard != NULL ) && ( pxClickboard->xPortsAvailable & xPort ) && !( pxClickboard->xPortsActive & xPort ) )
	{
		/* Deactivate any active clickboard on the given port. */
		pxClickboardOld = pxFindClickboardOnPort( xPort );
		if( pxClickboardOld != NULL )
		{
			xClickboardDeactivate( pxClickboardOld );
		}

		/* At the moment it is not possible to have the same clickboard on
		multiple ports. Deactivate the clickboard if it is already active. */
		if( pxClickboard->xPortsActive != eClickboardInactive )
		{
			xClickboardDeactivate( pxClickboard );
		}

		/* Activate the clickboard. */
		pxClickboard->xPortsActive |= xPort;
		pxClickboard->fClickboardInit( pxClickboard->pcName, xPort );

		/* Change active clickboard in config. */
		if( xPort == eClickboardPort1 )
		{
			pvSetConfig( eConfigClickConfPort1, sizeof( pxClickboard->xClickboardId ), &( pxClickboard->xClickboardId ) );
		}
		else if( xPort == eClickboardPort2 )
		{
			pvSetConfig( eConfigClickConfPort2, sizeof( pxClickboard->xClickboardId ), &( pxClickboard->xClickboardId ) );
		}

		xSuccess = pdTRUE;
	}

	return xSuccess;
}
/*-----------------------------------------------------------*/

BaseType_t xClickboardDeactivate( Clickboard_t *pxClickboard )
{
BaseType_t xSuccess = pdFALSE;

	if( ( pxClickboard != NULL ) && ( pxClickboard->xPortsActive != eClickboardInactive ) )
	{

		/* Remove active clickboard from config. */
		if( pxClickboard->xPortsActive == eClickboardPort1 )
		{
			pvSetConfig( eConfigClickConfPort1, 0, NULL );
		}
		else if( pxClickboard->xPortsActive == eClickboardPort2 )
		{
			pvSetConfig( eConfigClickConfPort2, 0, NULL );
		}

		pxClickboard->fClickboardDeinit();
		pxClickboard->xPortsActive = eClickboardInactive;

		xSuccess = pdTRUE;
	}

	return xSuccess;
}
/*-----------------------------------------------------------*/

#if( includeHTTP_DEMO != 0 )

	BaseType_t xRequestHandler_Config( char *pcBuffer, size_t uxBufferLength, QueryParam_t *pxParams, BaseType_t xParamCount )
	{
	static const char * const ppcPorts[] = { "port1", "port2" };
	BaseType_t x, xCount = 0;
	QueryParam_t *pxParam;
	Clickboard_t *pxClickboard;
	char on;

		/* Check if "port1" or "port2" GET parameters are set
		to activate the given clickboard. */
		for( x = 0; x < ARRAY_SIZE( ppcPorts ); x++ )
		{
			pxParam = pxFindKeyInQueryParams( ppcPorts[ x ], pxParams, xParamCount );
			if( pxParam != NULL )
			{
				/* Try to find a clickboard with passed name to activate it. */
				pxClickboard = pxFindClickboard( pxParam->pcValue );
				if( pxClickboard != NULL )
				{
					xClickboardActivate( pxClickboard, ( x + 1 ) );
				}
				/* If no clickboard could be found, check if 'none' was passed,
				to deactivate a currently active clickboard. */
				else if( strcmp( pxParam->pcValue, "none" ) == 0 )
				{
					pxClickboard = pxFindClickboardOnPort( ( x + 1 ) );
					if( pxClickboard != NULL ) {
						xClickboardDeactivate( pxClickboard );
					}
				}
			}
		}

		pxParam = pxFindKeyInQueryParams( "write", pxParams, xParamCount );
		if( pxParam != NULL )
		{
			xWriteConfig();
		}

		pxParam = pxFindKeyInQueryParams( "erase", pxParams, xParamCount );
		if( pxParam != NULL )
		{
			vEraseConfig();
		}

	#if( netconfigUSEMQTT != 0 )
		pxParam = pxFindKeyInQueryParams( "mqttSwitch", pxParams, xParamCount );
		if( pxParam != NULL )
		{
			if( strcmp( pxParam->pcValue, "on" ) == 0 )
			{
				xInitMQTT();
				on = 1;
				pvSetConfig( eConfigNetworkMqttOnPwr, sizeof(on), &on );
			}
			if( strcmp( pxParam->pcValue, "off" ) == 0 )
			{
				vDeinitMQTT();
				on = 0;
				pvSetConfig( eConfigNetworkMqttOnPwr, sizeof(on), &on );
			}
		}
	#endif /* #if( netconfigUSEMQTT != 0 ) */

		/* Generate response containing all registered clickboards,
		their names and on which ports they are available and active. */
		on = *((char *)pvGetConfig( eConfigNetworkMqttOnPwr, NULL ));
		xCount += sprintf( pcBuffer, "{\"mqttSwitch\":%d,\"mqttAutoOn\":%d,\"clickboards\":[", on);

		for( x = 0; x < ARRAY_SIZE( pxClickboards ); x++ )
		{
			xCount += sprintf( pcBuffer + xCount , "{\"name\":\"%s\",\"available\":%d,\"active\":%d},",
						pxClickboards[ x ].pcName,
						pxClickboards[ x ].xPortsAvailable,
						pxClickboards[ x ].xPortsActive );
		}

		if( xCount > 1 )
		{
			/* Replace last (invalid) ',' with ']' */
			xCount--;
		}

		xCount += sprintf( pcBuffer + xCount, "]}" );

		return xCount;
	}

#endif /* includeHTTP_DEMO */
/*-----------------------------------------------------------*/

void xClickboardsInit()
{
BaseType_t x;
eClickboardId_t *pxIdPort1, *pxIdPort2;

	pxIdPort1 = (eClickboardId_t *) pvGetConfig( eConfigClickConfPort1, NULL );
	pxIdPort2 = (eClickboardId_t *) pvGetConfig( eConfigClickConfPort2, NULL );

	for( x = 0; x < ARRAY_SIZE( pxClickboards ); x++ )
	{
		/* Ensure init and deinit handlers are set. */
		configASSERT( pxClickboards[ x ].fClickboardInit != NULL );
		configASSERT( pxClickboards[ x ].fClickboardDeinit != NULL );

		/* Check if clickboard config was stored in flash. */
		if( ( pxIdPort1 != NULL ) || ( pxIdPort2 != NULL ) )
		{
			if( *pxIdPort1 == pxClickboards[ x ].xClickboardId )
			{
				pxClickboards[ x ].xPortsActive = eClickboardPort1;
				pxClickboards[ x ].fClickboardInit( pxClickboards[ x ].pcName, pxClickboards[ x ].xPortsActive );
			}
			else if( *pxIdPort2 == pxClickboards[ x ].xClickboardId )
			{
				pxClickboards[ x ].xPortsActive = eClickboardPort2;
				pxClickboards[ x ].fClickboardInit( pxClickboards[ x ].pcName, pxClickboards[ x ].xPortsActive );
			}
			else
			{
				pxClickboards[ x ].xPortsActive = eClickboardInactive;
			}
		}
		else
		{
			if( pxClickboards[ x ].xPortsActive != eClickboardInactive )
			{
				/* Ensure at max. one clickboard is active on a port. */
				configASSERT( pxFindClickboardOnPort( pxClickboards[ x ].xPortsActive ) == NULL );

				pxClickboards[ x ].fClickboardInit( pxClickboards[ x ].pcName, pxClickboards[ x ].xPortsActive );
			}
		}
	}

	#if( includeHTTP_DEMO != 0 )
	{
		xAddRequestHandler( "config", xRequestHandler_Config );
	}
	#endif

}
/*-----------------------------------------------------------*/
