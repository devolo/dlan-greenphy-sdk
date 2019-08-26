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
#include <stdio.h>
#include <stdlib.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

/* FreeRTOS Protocol includes. */
#include "FreeRTOS_HTTP_commands.h"
#include "FreeRTOS_HTTP_server.h"

/* FreeRTOS+FAT includes. */
#include "ff_stdio.h"

#ifndef HTTP_SERVER_BACKLOG
	#define HTTP_SERVER_BACKLOG			( 12 )
#endif

#ifndef USE_HTML_CHUNKS
	#define USE_HTML_CHUNKS				( 0 )
#endif

#if !defined( ARRAY_SIZE )
	#define ARRAY_SIZE(x) ( BaseType_t ) (sizeof( x ) / sizeof( x )[ 0 ] )
#endif

/* Some defines to make the code more readbale */
#define pcCOMMAND_BUFFER	pxClient->pxParent->pcCommandBuffer
#define pcNEW_DIR			pxClient->pxParent->pcNewDir
#define pcFILE_BUFFER		pxClient->pxParent->pcFileBuffer

#ifndef ipconfigHTTP_REQUEST_CHARACTER
	#define ipconfigHTTP_REQUEST_CHARACTER		'?'
#endif

#ifndef ipconfigHTTP_REQUEST_DELIMITER
	#define ipconfigHTTP_REQUEST_DELIMITER		'&'
#endif

/*_RB_ Need comment block, although fairly self evident. */
static void prvFileClose( HTTPClient_t *pxClient );
static BaseType_t prvProcessCmd( HTTPClient_t *pxClient, BaseType_t xIndex );
static const char *pcGetContentsType( const char *apFname );
static char *strnew( const char *pcString );
static void prvRemoveSlash( char *pcDir );
static BaseType_t prvOpenURL( HTTPClient_t *pxClient );
static BaseType_t prvSendFile( HTTPClient_t *pxClient );
static BaseType_t prvSendReply( HTTPClient_t *pxClient, BaseType_t xCode );

static const char pcEmptyString[1] = { '\0' };

typedef struct xTYPE_COUPLE
{
	const char *pcExtension;
	const char *pcType;
} TypeCouple_t;

static TypeCouple_t pxTypeCouples[ ] =
{
	{ "html", "text/html" },
	{ "css",  "text/css" },
	{ "js",   "text/javascript" },
	{ "json", "application/json" },
	{ "svg",  "image/svg+xml" },
	{ "png",  "image/png" },
	{ "jpg",  "image/jpeg" },
	{ "gif",  "image/gif" },
	{ "txt",  "text/plain" },
	{ "mp3",  "audio/mpeg3" },
	{ "wav",  "audio/wav" },
	{ "flac", "audio/ogg" },
	{ "pdf",  "application/pdf" },
	{ "ttf",  "application/x-font-ttf" },
	{ "ttc",  "application/x-font-ttf" }
};

static void prvFileClose( HTTPClient_t *pxClient )
{
	if( pxClient->pxFileHandle != NULL )
	{
		FreeRTOS_printf( ( "Closing file: %s\n", pxClient->pcCurrentFilename ) );
		ff_fclose( pxClient->pxFileHandle );
		pxClient->pxFileHandle = NULL;
	}
}
/*-----------------------------------------------------------*/

static BaseType_t prvSendReply( HTTPClient_t *pxClient, BaseType_t xCode )
{
struct xHTTP_SERVER *pxParent = pxClient->pxParent;
BaseType_t xRc;

	/* A normal command reply on the main socket (port 21). */
	char *pcBuffer = pxParent->pcFileBuffer;

	xRc = snprintf( pcBuffer, sizeof( pxParent->pcFileBuffer ),
		"HTTP/1.1 %d %s\r\n"
		"Content-Type: %s\r\n"
		"Connection: %s\r\n"
		"%s\r\n",
		( int ) xCode,
		webCodename (xCode),
		pxParent->pcContentsType[0] ? pxParent->pcContentsType : "text/html",
		pxParent->pcConnection[0] ? pxParent->pcConnection : "close",
		pxParent->pcExtraContents );

	pxParent->pcContentsType[0] = '\0';
	pxParent->pcExtraContents[0] = '\0';

	if( xCode != WEB_REPLY_OK )
	{
	BaseType_t xTrueValue = 1;
		FreeRTOS_setsockopt( pxClient->xSocket, 0, FREERTOS_SO_CLOSE_AFTER_SEND, ( void * ) &xTrueValue, sizeof( xTrueValue ) );
	}

	xRc = FreeRTOS_send( pxClient->xSocket, ( const void * ) pcBuffer, xRc, 0 );
	pxClient->bits.bReplySent = pdTRUE_UNSIGNED;

	return xRc;
}
/*-----------------------------------------------------------*/

static BaseType_t prvSendFile( HTTPClient_t *pxClient )
{
size_t uxCount;
BaseType_t xRc = 0;
BaseType_t xSockOptValue;

	if( pxClient->bits.bReplySent == pdFALSE_UNSIGNED )
	{
		pxClient->bits.bReplySent = pdTRUE_UNSIGNED;

		xSockOptValue = pdTRUE_UNSIGNED;
		FreeRTOS_setsockopt( pxClient->xSocket, 0, FREERTOS_SO_SET_FULL_SIZE, ( void * ) &xSockOptValue, sizeof( xSockOptValue ) );

		strncpy( pxClient->pxParent->pcConnection, "keep-alive", sizeof( pxClient->pxParent->pcConnection ) );
		strncpy( pxClient->pxParent->pcContentsType, pcGetContentsType( pxClient->pcCurrentFilename ), sizeof( pxClient->pxParent->pcContentsType ) );
		snprintf( pxClient->pxParent->pcExtraContents, sizeof( pxClient->pxParent->pcExtraContents ),
			"Content-Encoding: gzip\r\n"
			"Content-Length: %d\r\n", ( int ) pxClient->uxBytesLeft );

		/* "Requested file action OK". */
		xRc = prvSendReply( pxClient, WEB_REPLY_OK );
	}

	if( xRc >= 0 ) do
	{
		char *pcBuffer;
		BaseType_t xBufferLength;
		pcBuffer = ( char * )FreeRTOS_get_tx_head( pxClient->xSocket, &xBufferLength );

		if( pxClient->uxBytesLeft < (size_t) xBufferLength )
		{
			uxCount = pxClient->uxBytesLeft;
		}
		else
		{
			uxCount = xBufferLength;
		}

		if( uxCount > 0u )
		{
			ff_fread( pcBuffer, 1, uxCount, pxClient->pxFileHandle );
			pxClient->uxBytesLeft -= uxCount;

			if( pxClient->uxBytesLeft == 0u )
			{
				xSockOptValue = pdTRUE_UNSIGNED;
//				FreeRTOS_setsockopt( pxClient->xSocket, 0, FREERTOS_SO_CLOSE_AFTER_SEND, ( void * ) &xSockOptValue, sizeof( xSockOptValue ) );
			}

			xRc = FreeRTOS_send( pxClient->xSocket, NULL, uxCount, 0 );

			if( pxClient->uxBytesLeft == 0u )
			{
				xSockOptValue = pdFALSE_UNSIGNED;
				FreeRTOS_setsockopt( pxClient->xSocket, 0, FREERTOS_SO_SET_FULL_SIZE, ( void * ) &xSockOptValue, sizeof( xSockOptValue ) );
			}
			if( xRc < 0 )
			{
				break;
			}
		}
	} while( uxCount > 0u );

	if( pxClient->uxBytesLeft == 0u )
	{
		/* Writing is ready, no need for further 'eSELECT_WRITE' events. */
		FreeRTOS_FD_CLR( pxClient->xSocket, pxClient->pxParent->xSocketSet, eSELECT_WRITE );
		prvFileClose( pxClient );
	}
	else
	{
		/* Wake up the TCP task as soon as this socket may be written to. */
		FreeRTOS_FD_SET( pxClient->xSocket, pxClient->pxParent->xSocketSet, eSELECT_WRITE );
	}

	return xRc;
}
/*-----------------------------------------------------------*/

static BaseType_t prvHandleRequest( HTTPClient_t *pxClient )
{
BaseType_t xRc = 0;
size_t uxCount;
BaseType_t xSockOptValue;
char pcChunkSize[8];

	if( pxClient->bits.bReplySent == pdFALSE_UNSIGNED )
	{
		pxClient->bits.bReplySent = pdTRUE_UNSIGNED;

		xSockOptValue = pdTRUE_UNSIGNED;
		FreeRTOS_setsockopt( pxClient->xSocket, 0, FREERTOS_SO_SET_FULL_SIZE, ( void * ) &xSockOptValue, sizeof( xSockOptValue ) );

		strncpy( pxClient->pxParent->pcConnection, "keep-alive", sizeof( pxClient->pxParent->pcConnection ) );
		strncpy( pxClient->pxParent->pcContentsType, "application/json", sizeof( pxClient->pxParent->pcContentsType ) );
		strncpy( pxClient->pxParent->pcExtraContents, "Transfer-Encoding: chunked\r\n", sizeof( pxClient->pxParent->pcExtraContents ) );

		xRc = prvSendReply( pxClient, WEB_REPLY_OK );	/* "Requested file action OK" */
	}

	/* Use a loop to be able to break out on error. */
	while( xRc >= 0 )
	{
		uxCount = uxApplicationHTTPHandleRequestHook( pxClient->pcUrlData, pxClient->pxParent->pcFileBuffer, sizeof( pxClient->pxParent->pcFileBuffer ) );

		if( uxCount > 0 )
		{
			xRc = snprintf( pcChunkSize, sizeof( pcChunkSize ), "%x\r\n", uxCount );

			/* First send the chunk size. */
			if( ( xRc = FreeRTOS_send( pxClient->xSocket, pcChunkSize, xRc, 0 ) ) < 0 ) break;
			/* Now send the chunk itself. */
			if( ( xRc = FreeRTOS_send( pxClient->xSocket, pxClient->pxParent->pcFileBuffer, uxCount, 0 ) ) < 0 ) break;
		}

		xSockOptValue = pdTRUE_UNSIGNED;
//		FreeRTOS_setsockopt( pxClient->xSocket, 0, FREERTOS_SO_CLOSE_AFTER_SEND, ( void * ) &xSockOptValue, sizeof( xSockOptValue ) );

		/* At last send end of chunk and finishing 0 byte chunk at once. */
		if( ( xRc = FreeRTOS_send( pxClient->xSocket, "\r\n0\r\n\r\n", sizeof( "\r\n0\r\n\r\n" ) - 1, 0 ) ) < 0 ) break;

		xSockOptValue = pdFALSE_UNSIGNED;
		FreeRTOS_setsockopt( pxClient->xSocket, 0, FREERTOS_SO_SET_FULL_SIZE, ( void * ) &xSockOptValue, sizeof( xSockOptValue ) );

		break;
	}

	return xRc;
}
/*-----------------------------------------------------------*/

static BaseType_t prvOpenURL( HTTPClient_t *pxClient )
{
BaseType_t xRc;
char pcSlash[ 2 ];

	pxClient->bits.ulFlags = 0;

	#if( ipconfigHTTP_HAS_HANDLE_REQUEST_HOOK != 0 )
	{
		if( strchr( pxClient->pcUrlData, ipconfigHTTP_REQUEST_CHARACTER ) != NULL )
		{
			/* Although against the coding standard of FreeRTOS, a return is
			done here  to simplify this conditional code. */
			return prvHandleRequest( pxClient );
		}
	}
	#endif /* ipconfigHTTP_HAS_HANDLE_REQUEST_HOOK */

	if( pxClient->pcUrlData[ 0 ] != '/' )
	{
		/* Insert a slash before the file name. */
		pcSlash[ 0 ] = '/';
		pcSlash[ 1 ] = '\0';
	}
	else
	{
		/* The browser provided a starting '/' already. */
		pcSlash[ 0 ] = '\0';
	}
	snprintf( pxClient->pcCurrentFilename, sizeof( pxClient->pcCurrentFilename ), "%s%s%s",
		pxClient->pcRootDir,
		pcSlash,
		pxClient->pcUrlData);

	/* _ML_: redirect root to index.html. */
	if( strcmp( pxClient->pcCurrentFilename, "/" ) == 0 )
	{
		strcpy( pxClient->pcCurrentFilename, ipconfigHTTP_DIRECTORY_INDEX );
	}

	pxClient->pxFileHandle = ff_fopen( pxClient->pcCurrentFilename, "rb" );

	FreeRTOS_printf( ( "Open file '%s': %s\n", pxClient->pcCurrentFilename,
		pxClient->pxFileHandle != NULL ? "Ok" : strerror( stdioGET_ERRNO() ) ) );

	if( pxClient->pxFileHandle == NULL )
	{
		/* "404 File not found". */
		xRc = prvSendReply( pxClient, WEB_NOT_FOUND );
	}
	else
	{
		pxClient->uxBytesLeft = ( size_t ) pxClient->pxFileHandle->ulFileSize;
		xRc = prvSendFile( pxClient );
	}

	return xRc;
}
/*-----------------------------------------------------------*/

static BaseType_t prvProcessCmd( HTTPClient_t *pxClient, BaseType_t xIndex )
{
BaseType_t xResult = 0;

	/* A new command has been received. Process it. */
	switch( xIndex )
	{
	case ECMD_GET:
		xResult = prvOpenURL( pxClient );
		break;

	case ECMD_HEAD:
	case ECMD_POST:
	case ECMD_PUT:
	case ECMD_DELETE:
	case ECMD_TRACE:
	case ECMD_OPTIONS:
	case ECMD_CONNECT:
	case ECMD_PATCH:
	case ECMD_UNK:
		{
			FreeRTOS_printf( ( "prvProcessCmd: Not implemented: %s\n",
				xWebCommands[xIndex].pcCommandName ) );
		}
		break;
	}

	return xResult;
}
/*-----------------------------------------------------------*/

BaseType_t xHTTPClientWork( HTTPClient_t *pxClient )
{
BaseType_t xRc;
TickType_t xTicksToWait = pdMS_TO_TICKS( 5000 );

	if( pxClient->pxFileHandle != NULL )
	{
		prvSendFile( pxClient );
	}
	else
	{

		xRc = FreeRTOS_recv( pxClient->xSocket, ( void * )pcCOMMAND_BUFFER, sizeof( pcCOMMAND_BUFFER ), 0 );

		if( xRc > 0 )
		{
		BaseType_t xIndex;
		const char *pcEndOfCmd;
		const struct xWEB_COMMAND *curCmd;
		char *pcBuffer = pcCOMMAND_BUFFER;

			if( xRc < ( BaseType_t ) sizeof( pcCOMMAND_BUFFER ) )
			{
				pcBuffer[ xRc ] = '\0';
			}
			while( xRc && ( pcBuffer[ xRc - 1 ] == 13 || pcBuffer[ xRc - 1 ] == 10 ) )
			{
				pcBuffer[ --xRc ] = '\0';
			}
			pcEndOfCmd = pcBuffer + xRc;

			curCmd = xWebCommands;

			/* Pointing to "/index.html HTTP/1.1". */
			pxClient->pcUrlData = pcBuffer;

			/* Pointing to "HTTP/1.1". */
			pxClient->pcRestData = pcEmptyString;

			/* Last entry is "ECMD_UNK". */
			for( xIndex = 0; xIndex < WEB_CMD_COUNT - 1; xIndex++, curCmd++ )
			{
			BaseType_t xLength;

				xLength = curCmd->xCommandLength;
				if( ( xRc >= xLength ) && ( memcmp( curCmd->pcCommandName, pcBuffer, xLength ) == 0 ) )
				{
				char *pcLastPtr;

					pxClient->pcUrlData += xLength + 1;
					for( pcLastPtr = (char *)pxClient->pcUrlData; pcLastPtr < pcEndOfCmd; pcLastPtr++ )
					{
						char ch = *pcLastPtr;
						if( ( ch == '\0' ) || ( strchr( "\n\r \t", ch ) != NULL ) )
						{
							*pcLastPtr = '\0';
							pxClient->pcRestData = pcLastPtr + 1;
							break;
						}
					}
					break;
				}
			}

			if( xIndex < WEB_CMD_COUNT - 1 )
			{
				xRc = prvProcessCmd( pxClient, xIndex );
			}

			/* Received data, refresh timeout. */
			vTaskSetTimeOutState( &pxClient->xTimeOut );
			pxClient->xTicksToWait = xTicksToWait;
			pxClient->bits.bShutdownInitiated = pdFALSE_UNSIGNED;
		}
		else if( xRc < 0 )
		{
			/* The connection will be closed and the client will be deleted. */
			FreeRTOS_printf( ( "xHTTPClientWork: rc = %ld\n", xRc ) );
		}
		else
		{
			/* Check for timeout. */
			if( pxClient->bits.bShutdownInitiated == pdFALSE_UNSIGNED )
			{
				if( xTaskCheckForTimeOut( &pxClient->xTimeOut, &pxClient->xTicksToWait ) != pdFALSE )
				{
					/* Try to gracefully shutdown connection. */
					FreeRTOS_printf( ( "xHTTPClientWork: keep-alive timed out, shutdown connection.\r\n" ) );
					FreeRTOS_shutdown( pxClient->xSocket, FREERTOS_SHUT_RDWR );
					pxClient->bits.bShutdownInitiated = pdTRUE_UNSIGNED;
				}
			}
		}
	}
	return xRc;
}
/*-----------------------------------------------------------*/

void vHTTPClientDelete( HTTPClient_t *pxClient )
{
	/* This HTTP client stops, close / release all resources. */
	if( pxClient->xSocket != FREERTOS_NO_SOCKET )
	{

		/* Check if socket is a reused listen socket and let it listen again. */
		if( FreeRTOS_listen( pxClient->xSocket, 0 ) != 0 )
		{
			/* Otherwise it must be a child socket which will be closed now. */
			FreeRTOS_FD_CLR( pxClient->xSocket, pxClient->pxParent->xSocketSet, eSELECT_ALL );
			FreeRTOS_closesocket( pxClient->xSocket );
			pxClient->xSocket = FREERTOS_NO_SOCKET;
		}
	}
	prvFileClose( pxClient );
}
/*-----------------------------------------------------------*/

static HTTPServer_t xServer = { 0 };

HTTPServer_t *FreeRTOS_CreateHTTPServer( const struct xSERVER_CONFIG *pxConfig )
{
BaseType_t xPortNumber = pxConfig->xPortNumber;
SocketSet_t xSocketSet;
Socket_t xSocket;
struct freertos_sockaddr xAddress;
BaseType_t xTrueValue = pdTRUE;
TickType_t xNoTimeout = 0;

	xSocketSet = FreeRTOS_CreateSocketSet();

	if( xSocketSet != NULL )
	{
		xServer.xSocketSet = xSocketSet;

		if( xPortNumber > 0 )
		{
			xSocket = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP );
			FreeRTOS_printf( ( "TCP socket on port %d\n", ( int ) xPortNumber ) );

			if( xSocket != FREERTOS_NO_SOCKET )
			{
				xAddress.sin_addr = FreeRTOS_GetIPAddress(); // Single NIC, currently not used
				xAddress.sin_port = FreeRTOS_htons( xPortNumber );

				FreeRTOS_bind( xSocket, &xAddress, sizeof( xAddress ) );
				FreeRTOS_listen( xSocket, pxConfig->xBackLog );
				if( pxConfig->xBackLog == 0 ) {
					/* _ML_ Do not create child sockets, instead reuse listen socket.
					This limits the server to just one active connection at a time. */
					FreeRTOS_setsockopt( xSocket, 0, FREERTOS_SO_REUSE_LISTEN_SOCKET, ( void * ) &xTrueValue, sizeof( xTrueValue ) );
				}

				FreeRTOS_setsockopt( xSocket, 0, FREERTOS_SO_RCVTIMEO, ( void * ) &xNoTimeout, sizeof( BaseType_t ) );
				FreeRTOS_setsockopt( xSocket, 0, FREERTOS_SO_SNDTIMEO, ( void * ) &xNoTimeout, sizeof( BaseType_t ) );

				#if( ipconfigHTTP_RX_BUFSIZE > 0 )
				{
					WinProperties_t xWinProps;

					memset( &xWinProps, '\0', sizeof( xWinProps ) );
					/* The parent socket itself won't get connected. The properties below
					will be inherited by each new child socket. */
					xWinProps.lTxBufSize = ipconfigHTTP_TX_BUFSIZE;
					xWinProps.lTxWinSize = ipconfigHTTP_TX_WINSIZE;
					xWinProps.lRxBufSize = ipconfigHTTP_RX_BUFSIZE;
					xWinProps.lRxWinSize = ipconfigHTTP_RX_WINSIZE;

					/* Set the window and buffer sizes. */
					FreeRTOS_setsockopt( xSocket, 0, FREERTOS_SO_WIN_PROPERTIES, ( void * ) &xWinProps,	sizeof( xWinProps ) );
				}
				#endif

				FreeRTOS_FD_SET( xSocket, xSocketSet, eSELECT_READ|eSELECT_EXCEPT );
				xServer.xSocket = xSocket;
				xServer.pcRootDir = strnew( pxConfig->pcRootDir );
				prvRemoveSlash( ( char * ) xServer.pcRootDir );
			}
		}
	}

	return  &xServer;
}
/*-----------------------------------------------------------*/


void FreeRTOS_HTTPServerWork( HTTPServer_t *pxServer, TickType_t xBlockingTime )
{
struct freertos_sockaddr xAddress;
HTTPClient_t *pxClient = NULL, *pxThis;
Socket_t xNexSocket;
socklen_t xSocketLength;
BaseType_t xRc;
BaseType_t xSize;
TickType_t xTicksToWait = pdMS_TO_TICKS( 5000 );

	//FreeRTOS_printf( ( "HTTPServerWork: called\n" ) );

	/* Let the server do one working cycle. */
	xRc = FreeRTOS_select( pxServer->xSocketSet, xBlockingTime );

	if( FreeRTOS_FD_ISSET( pxServer->xSocket, pxServer->xSocketSet ) != 0 )
	{
		/* Accept connection, if the listening socket has enough free
		connections left. */
		xSocketLength = sizeof( xAddress );
		xNexSocket = FreeRTOS_accept( pxServer->xSocket, &xAddress, &xSocketLength);
		if( ( xNexSocket != FREERTOS_NO_SOCKET ) && ( xNexSocket != FREERTOS_INVALID_SOCKET ) )
		{
			//FreeRTOS_printf( ( "HTTPServerWork: add new client\n" ) );

			/* Add new client connection in front of list. */
			xSize = sizeof( HTTPClient_t );
			pxClient = ( HTTPClient_t* ) pvPortMallocLarge( xSize );
			memset( pxClient, 0, xSize );

			if( pxClient != NULL )
			{
				pxClient->pcRootDir = pxServer->pcRootDir;
				pxClient->pxParent = pxServer;
				pxClient->xSocket = xNexSocket;
				pxClient->pxNextClient = pxServer->pxClients;
				pxServer->pxClients = pxClient;
				FreeRTOS_FD_SET( xNexSocket, pxServer->xSocketSet, eSELECT_READ|eSELECT_EXCEPT );

				/* Established connection, set timeout. */
				vTaskSetTimeOutState( &pxClient->xTimeOut );
				pxClient->xTicksToWait = xTicksToWait;
				pxClient->bits.bShutdownInitiated = pdFALSE_UNSIGNED;
			}
			else
			{
				FreeRTOS_closesocket( xNexSocket );
			}
		}
	}

	pxClient = pxServer->pxClients;
	while( pxClient != NULL )
	{
		pxThis = pxClient;
		pxClient = pxThis->pxNextClient;

		//FreeRTOS_printf( ( "HTTPServerWork: do client work\n" ) );

		xRc = xHTTPClientWork( pxThis );

		if (xRc < 0 )
		{
			if( pxServer->pxClients == pxThis )
			{
				pxServer->pxClients = pxClient;
			}

			/* Close handles, resources */
			vHTTPClientDelete( pxThis );
			/* Free the space */
			vPortFreeLarge( pxThis );
			pxThis = NULL;
		}
	}
}
/*-----------------------------------------------------------*/


static const char *pcGetContentsType (const char *apFname)
{
	const char *slash = NULL;
	const char *dot = NULL;
	const char *ptr;
	const char *pcResult = "text/html";
	BaseType_t x;

	for( ptr = apFname; *ptr; ptr++ )
	{
		switch(*ptr)
		{
		case '.': dot   = ptr; break;
		case '/': slash = ptr; break;
		}
	}
	if( dot > slash )
	{
		dot++;
		for( x = 0; x < ARRAY_SIZE( pxTypeCouples ); x++ )
		{
			if( strcasecmp( dot, pxTypeCouples[ x ].pcExtension ) == 0 )
			{
				pcResult = pxTypeCouples[ x ].pcType;
				break;
			}
		}
	}
	return pcResult;
}
/*-----------------------------------------------------------*/

static char *strnew( const char *pcString )
{
BaseType_t xLength;
char *pxBuffer;

	xLength = strlen( pcString ) + 1;
	pxBuffer = ( char * ) pvPortMalloc( xLength );
	if( pxBuffer != NULL )
	{
		memcpy( pxBuffer, pcString, xLength );
	}

	return pxBuffer;
}
/*-----------------------------------------------------------*/

static void prvRemoveSlash( char *pcDir )
{
BaseType_t xLength = strlen( pcDir );

	while( ( xLength > 0 ) && ( pcDir[ xLength - 1 ] == '/' ) )
	{
		pcDir[ --xLength ] = '\0';
	}
}
/*-----------------------------------------------------------*/
