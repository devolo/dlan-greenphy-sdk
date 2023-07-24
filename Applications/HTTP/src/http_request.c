/*
 * Copyright (c) 2023, devolo GmbH, Aachen, Germany.
 * All rights reserved.
 *
 * This Software is part of the devolo GreenPHY-SDK.
 *
 * Usage in source form and redistribution in binary form, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Usage in source form is subject to a current end user license agreement
 *    with the devolo GmbH.
 * 2. Neither the name of the devolo GmbH nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 3. Redistribution in binary form is limited to the usage on the GreenPHY
 *    module of the devolo GmbH.
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
#include <stdio.h>

/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include <task.h>

/* FreeRTOS+TCP includes. */
#include <FreeRTOS_IP.h>

#include "http_query_parser.h"
#include "http_request.h"

extern BaseType_t xRequestHandler_Status( char *pcBuffer, size_t uxBufferLength, QueryParam_t *pxParams, BaseType_t xParamCount );

static HTTPRequestHandler_t pxHTTPRequestHandlers[ HTTP_MAX_REQUEST_HANDLERS ] =
{
	{ "status", xRequestHandler_Status },
};

HTTPRequestHandler_t *prvFindRequestHandler( const char *pcName )
{
BaseType_t x;
HTTPRequestHandler_t *pxRequestHandler = NULL;

	if( pcName != NULL )
	{
		for( x = 0; x < HTTP_MAX_REQUEST_HANDLERS; x++ )
		{
			if( pxHTTPRequestHandlers[ x ].pcName != NULL )
			{
				if( strcmp( pcName, pxHTTPRequestHandlers[ x ].pcName ) == 0 )
				{
					pxRequestHandler = &pxHTTPRequestHandlers[ x ];
					break;
				}
			}
		}
	}

	return pxRequestHandler;
}

BaseType_t xAddRequestHandler( const char *pcName, FHTTPRequestHandler fRequestHandler )
{
BaseType_t x, xSuccess = pdFALSE;

	if( prvFindRequestHandler( pcName ) == NULL )
	{
		for( x = 0; x < HTTP_MAX_REQUEST_HANDLERS; x++ )
		{
			if( pxHTTPRequestHandlers[ x ].pcName == NULL )
			{
				pxHTTPRequestHandlers[ x ].pcName = pcName;
				pxHTTPRequestHandlers[ x ].fRequestHandler = fRequestHandler;

				xSuccess = pdTRUE;
				break;
			}
		}
	}

	return xSuccess;
}

BaseType_t xRemoveRequestHandler( char *pcName )
{
BaseType_t xSuccess = pdFALSE;
HTTPRequestHandler_t *pxRequestHandler;

	pxRequestHandler = prvFindRequestHandler( pcName );
	if( pxRequestHandler != NULL )
	{
		pxRequestHandler->pcName = NULL;
		pxRequestHandler->fRequestHandler = NULL;

		xSuccess = pdTRUE;
	}

	return xSuccess;
}

size_t uxApplicationHTTPHandleRequestHook( const char *pcURLData, char *pcBuffer, size_t uxBufferLength )
{
char *pcQuery, *pcExt;
QueryParam_t pxParams[ HTTP_MAX_QUERY_PARAMS ];
BaseType_t xParamCount, uxResult = 0;
HTTPRequestHandler_t *pxRequestHandler;

	/* Split request query from the filename. */
	pcQuery = strchr( pcURLData, ipconfigHTTP_REQUEST_CHARACTER );
	*pcQuery++ = '\0';

	/* Strip extension from the filename. */
	pcExt = strchr( pcURLData, '.' );
	*pcExt++ = '\0';

	/* Find Handler with the files basename without leading '/'. */
	pxRequestHandler = prvFindRequestHandler( pcURLData + 1 );
	if( ( pxRequestHandler != NULL ) && ( pxRequestHandler->fRequestHandler != NULL ) )
	{
		xParamCount = xParseQuery( pcQuery, pxParams, HTTP_MAX_QUERY_PARAMS );

		if( pxRequestHandler->fRequestHandler != NULL )
		{
			uxResult = pxRequestHandler->fRequestHandler( pcBuffer, uxBufferLength, pxParams, xParamCount );
		}
	}

	return ( size_t ) uxResult;
}
/*-----------------------------------------------------------*/
