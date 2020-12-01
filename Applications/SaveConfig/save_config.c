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

/* Project includes. */
#include "save_config.h"

#define member_size(type, member) sizeof(((type *)0)->member)

/*********************************************************************//**
 * @brief		Get Sector Number
 *
 * @param[in] adr	           Sector Address
 *
 * @return 	Sector Number.
 *
 **********************************************************************/
 uint32_t GetSecNum (uint32_t adr)
{
    uint32_t n;

    n = adr >> 12;                               //  4kB Sector
    if (n >= 0x10) {
      n = 0x0E + (n >> 3);                       // 32kB Sector
    }

    return (n);                                  // Sector Number
}
 /*-----------------------------------------------------------*/

typedef enum eCONFIG_VERSIONS
{
	eConfigVersion1 = 1,
	eConfigVersionLast
} eConfigVersion_t;

typedef struct __attribute__((__packed__)) xCONFIG_TLV
{
	uint8_t ucTag;        /* eConfigTag_t. */
	uint8_t reserved;     /* Reserved for alignment. */
	uint16_t usLength;    /* Length of pucValue. */
	uint8_t pucValue[1];  /* Data. Actual space needs to be allocated. */
} ConfigTLV_t;

typedef struct __attribute__((__packed__)) xCONFIG
{
	uint8_t ucVersion;			/* Config version. */
	uint8_t ucCount;            /* Count config rewrites. */
	uint16_t usLength;			/* Total length in bytes of TLV list. */
	uint32_t ulSignature;       /* Signature 0xAAAA5555 to validate config. */
	uint8_t pucTLVList[1];       /* List of TLV elements. */
} Config_t;

typedef enum eCONFIG_TAG_INDEXES {
	eConfigTagNotFound = -1,
#define X( id, name ) name##Index,
	LIST_OF_CONFIG_TAGS
#undef X
	eConfigNumberOfTags
} eConfigTagIndex_t;

typedef struct xCONFIG_WRITE_HANDLE {
	uint32_t ulDestination;
	uint8_t *pucBuffer;
	uint16_t usBufferSize;
	uint16_t usLength;
	uint16_t usBytesWritten;
	uint8_t  ucCount;
	struct
	{
		uint8_t
			bEraseConfig : 1;
	} bits;
} ConfigWriteHandle_t;

/*-----------------------------------------------------------*/

static const ConfigTLV_t *ppxConfigCacheTLVList[ eConfigNumberOfTags ];
static const Config_t *pxConfigInFlash = NULL;

/*-----------------------------------------------------------*/

/*
 * Calculates the total size of a ConfigTLV from its data length.
 */
static uint16_t prvSizeOfTLV( uint16_t usLength )
{
uint16_t usSize;

	/* Calculate size needed for TLV. pucValue already includes one data byte. */
	usSize = ( sizeof( ConfigTLV_t ) - member_size( ConfigTLV_t, pucValue ) + usLength );
	/* Add padding to TLV so all TLVs are aligned properly. */
	usSize += ( ( 4 - ( usSize % 4 ) ) % 4 );
	return usSize;
}
/*-----------------------------------------------------------*/

/*
 * Calculates the total size of the config from the length of its TLV list.
 */
static uint16_t prvSizeOfConfig( uint16_t usLength )
{
uint16_t usSize;

	/* Calculate size needed for config. Config_t already includes one ConfigTLV_t. */
	usSize = ( sizeof( Config_t ) - member_size( Config_t, pucTLVList ) + usLength );
	/* Add padding to config so config meets 256 byte alignment needed for flash. */
	usSize += ( ( 256 - ( usSize % 256 ) ) % 256 );
	return usSize;
}
/*-----------------------------------------------------------*/

/*
 * Returns the corresponding cache index to a tag.
 */
static eConfigTagIndex_t prvFindTLVInCache( eConfigTag_t xTag )
{
eConfigTagIndex_t eReturn;

	switch( xTag )
	{
#define X( id, name ) case name: eReturn = name##Index; break;
	LIST_OF_CONFIG_TAGS
#undef X
	default: eReturn = eConfigTagNotFound; break;
	}

	return eReturn;
}
/*-----------------------------------------------------------*/

/*
 * Checks whether TLV resides in RAM or in flash.
 */
static inline BaseType_t prvTLVInRAM( const ConfigTLV_t *pxTLV )
{
	return ( pxTLV > (ConfigTLV_t *) CONFIG_FLASH_AREA_END );
}
/*-----------------------------------------------------------*/

/*
 * Returns pdPASS if pxConfig points to valid config, pdFAIL otherwise.
 */
static const BaseType_t prvVerifyConfig( const Config_t *pxConfig )
{
const uint8_t *pucConfig = (uint8_t *) pxConfig;
ConfigTLV_t *pxTLV;
const uint8_t *pucTLV;
BaseType_t xReturn = pdFAIL;

	if( pxConfig != NULL )
	{
		do {
			/* Verify that config is in valid flash area and 256 byte aligned. */
			if( (pucConfig < (uint8_t *) CONFIG_FLASH_AREA_START )
				|| ( ( pucConfig + prvSizeOfConfig( pxConfig->usLength ) ) >= (uint8_t *) CONFIG_FLASH_AREA_END )
				|| ( ( (uintptr_t) pxConfig & 0xFF ) != 0 ) )
			{
				break;
			}

			/* Verify signature and config version. */
			if( ( pxConfig->ulSignature != CONFIG_SIGNATURE ) || ( pxConfig->ucVersion >= eConfigVersionLast ) )
			{
				break;
			}

			/* Iterate through all TLVs in config to reach its end. */
			for( pucTLV = &pxConfig->pucTLVList[0];
				 pucTLV < ( &pxConfig->pucTLVList[0] + pxConfig->usLength );
				 pucTLV += prvSizeOfTLV( pxTLV->usLength ) )
			{
				pxTLV = (ConfigTLV_t *) pucTLV;
			}

			/* Verify that end of config matches with end of last TLV. */
			if( pxConfig->usLength != (uint16_t ) ( pucTLV - &pxConfig->pucTLVList[0] ) )
			{
				break;
			}

			/* All tests passed. */
			xReturn = pdPASS;

		} while( 0 );
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

/*
 * Returns the most recent config in flash and returns a pointer
 * to it if it is valid, otherwise NULL.
 */
static const Config_t *prvFindConfig( void )
{
const uint8_t *pucConfig;
const Config_t *pxConfig = NULL;

	/* Fast forward config pointer by just comparing the signature. */
	for( pucConfig = (uint8_t *) CONFIG_FLASH_AREA_START;
		 pucConfig < (uint8_t *) CONFIG_FLASH_AREA_END;
		 pucConfig += prvSizeOfConfig( pxConfig->usLength ) )
	{
		if( ( ( (Config_t *) pucConfig )->ulSignature != CONFIG_SIGNATURE ) )
		{
			/* Not a valid config. */
			break;
		}

		pxConfig = (Config_t *) pucConfig;
	}

	/* Verify that pxConfig points to a valid config. */
	if( prvVerifyConfig( pxConfig ) != pdPASS )
	{
		pxConfig = NULL;
	}

	return pxConfig;
}
/*-----------------------------------------------------------*/

static void prvCleanCache( void ) {
BaseType_t x;

	for( x = 0; x < eConfigNumberOfTags; x++ )
	{
		/* Check if there is a cached TLV in RAM and free it. */
		if( prvTLVInRAM( ppxConfigCacheTLVList[ x ] ) )
		{
			vPortFree( (void *) ppxConfigCacheTLVList[ x ] );
		}

		ppxConfigCacheTLVList[ x ] = NULL;
	}
}
/*-----------------------------------------------------------*/

/* The config cache is updated with pxConfig. It is asserted, that pxConfig is valid. */
static BaseType_t prvUpdateCache( const Config_t *pxConfig )
{
ConfigTLV_t *pxTLV;
const uint8_t *pucTLV;
eConfigTagIndex_t eCacheIndex;
BaseType_t xReturn = pdFAIL;

	if( ( pxConfig != NULL ) )
	{
		configASSERT( prvVerifyConfig( pxConfig ) == pdPASS );

		/* Clean cache before (re-)reading config. */
		prvCleanCache();

		/* Iterate through all TLVs in config. */
		for( pucTLV = pxConfig->pucTLVList;
			 pucTLV < pxConfig->pucTLVList + pxConfig->usLength;
			 pucTLV += prvSizeOfTLV( pxTLV->usLength ) )
		{
			pxTLV = (ConfigTLV_t *) pucTLV;

			eCacheIndex = prvFindTLVInCache( pxTLV->ucTag );
			if( eCacheIndex != eConfigTagNotFound )
			{
				/* Store a pointer to the flash TLV in cache. */
				ppxConfigCacheTLVList[ eCacheIndex ] = pxTLV;
			}
			else
			{
				DEBUGOUT( "ReadConfig: unknown tag %d", pxTLV->ucTag );
			}
		}

		xReturn = pdPASS;
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

static void prvWriteToFlash( ConfigWriteHandle_t *pxWriteHandle )
{
const uint32_t ulSector = GetSecNum( CONFIG_FLASH_AREA_START );

	if( pxWriteHandle->bits.bEraseConfig == pdTRUE )
	{
		vEraseConfig();
		pxWriteHandle->bits.bEraseConfig = pdFALSE;
	}

	__disable_irq();
	/* Buffer is full. Write it to flash. */
	if( Chip_IAP_PreSectorForReadWrite( ulSector, ulSector ) == IAP_CMD_SUCCESS )
	{
		Chip_IAP_CopyRamToFlash( pxWriteHandle->ulDestination + pxWriteHandle->usBytesWritten, (uint32_t *) pxWriteHandle->pucBuffer, (uint32_t) pxWriteHandle->usBufferSize );
		pxWriteHandle->usBytesWritten += pxWriteHandle->usBufferSize;
	}
	__enable_irq();
}
/*-----------------------------------------------------------*/

static void prvWriteConfig( ConfigWriteHandle_t *pxWriteHandle )
{
BaseType_t x;
uint16_t usBufferPos, usBytesCopied, usBytesAvailable, usBytesToCopy;

	/* Set the config header. */
	( (Config_t *) pxWriteHandle->pucBuffer )->ucVersion   = eConfigVersion1;
	( (Config_t *) pxWriteHandle->pucBuffer )->ucCount     = pxWriteHandle->ucCount;
	( (Config_t *) pxWriteHandle->pucBuffer )->usLength    = pxWriteHandle->usLength;
	( (Config_t *) pxWriteHandle->pucBuffer )->ulSignature = CONFIG_SIGNATURE;
	usBufferPos = offsetof( Config_t, pucTLVList );

	/* Iterate over all TLVs to write config into buffer. */
	for( x = 0; x < eConfigNumberOfTags; x++ )
	{
		if( ( ppxConfigCacheTLVList[ x ] != NULL ) && ( ppxConfigCacheTLVList[ x ]->usLength > 0 ) )
		{
			usBytesCopied = 0;
			usBytesAvailable = prvSizeOfTLV( ppxConfigCacheTLVList[ x ]->usLength );

			while( usBytesCopied < usBytesAvailable )
			{
				usBytesToCopy = usBytesAvailable - usBytesCopied;
				if( usBytesToCopy > ( pxWriteHandle->usBufferSize - usBufferPos ) )
				{
					usBytesToCopy = ( pxWriteHandle->usBufferSize - usBufferPos );
				}

				/* Copy TLV to buffer. */
				memcpy( ( pxWriteHandle->pucBuffer + usBufferPos ),
						( ( (uint8_t *) ppxConfigCacheTLVList[ x ] ) + usBytesCopied ),
						usBytesToCopy );

				usBufferPos += usBytesToCopy;

				if( usBufferPos == pxWriteHandle->usBufferSize )
				{
					/* Buffer is full, write it to flash. */
					prvWriteToFlash( pxWriteHandle );
					usBufferPos = 0;
				}

				usBytesCopied += usBytesToCopy;
			}
		}
	}

	/* Zero fill end of buffer. */
	memset( pxWriteHandle->pucBuffer + usBufferPos, 0, pxWriteHandle->usBufferSize - usBufferPos );

	/* Write buffer to flash. */
	prvWriteToFlash( pxWriteHandle );
}
/*-----------------------------------------------------------*/

void *pvGetConfig( eConfigTag_t xTag, uint16_t *pusLength )
{
eConfigTagIndex_t eCacheIndex;
void *pvReturn = NULL;

	eCacheIndex = prvFindTLVInCache( xTag );
	if( eCacheIndex != eConfigTagNotFound )
	{
		if( ppxConfigCacheTLVList[ eCacheIndex ] != NULL )
		{
			if( pusLength != NULL )
			{
				/* Return length, if wanted. */
				*pusLength = ppxConfigCacheTLVList[ eCacheIndex ]->usLength;
			}
			pvReturn = (void *) ppxConfigCacheTLVList[ eCacheIndex ]->pucValue;
		}
	}

	return pvReturn;
}
/*-----------------------------------------------------------*/

void *pvSetConfig( eConfigTag_t xTag, uint16_t usLength, const void * const pvValue )
{
eConfigTagIndex_t eCacheIndex;
ConfigTLV_t *pxTLV;
uint16_t usTLVLength;
void *pvReturn = NULL;

	eCacheIndex = prvFindTLVInCache( xTag );

	/* Check if TLV is already in RAM and free it. */
	if( (void *) ppxConfigCacheTLVList[ eCacheIndex ] > (void *) 0x00080000 )
	{
		vPortFree( (void *) ppxConfigCacheTLVList[ eCacheIndex ] );
	}

	/* If value is NULL, set length to zero, so the TLV gets deleted from config. */
	if( pvValue == NULL )
	{
		usLength = 0;
	}

	usTLVLength = prvSizeOfTLV( usLength );
	pxTLV = pvPortMalloc( usTLVLength );
	if( pxTLV != NULL )
	{
		pxTLV->ucTag = (uint8_t) xTag;
		pxTLV->usLength = usLength;
		/* Copy value. */
		memcpy( pxTLV->pucValue, pvValue, usLength );
		/* Zero fill the padding. */
		memset( pxTLV->pucValue + usLength, 0, ( usTLVLength - offsetof( ConfigTLV_t, pucValue ) - usLength ) );

		/* Store a pointer to the TLV in cache. */
		ppxConfigCacheTLVList[ eCacheIndex ] = pxTLV;
		pvReturn = (void *) pxTLV->pucValue;
	}

	return pvReturn;
}
/*-----------------------------------------------------------*/

void vEraseConfig( void )
{
const uint32_t ulSector = GetSecNum( CONFIG_FLASH_AREA_START );

	DEBUGOUT( "Erase config\r\n" );
	/* Reset config. */
	prvCleanCache();
	pxConfigInFlash = NULL;

	/* Verify sector is not blank already. */
	if( Chip_IAP_BlankCheckSector( ulSector, ulSector ) == IAP_SECTOR_NOT_BLANK )
	{
		/* Erase config from flash. */
		__disable_irq();
		if( Chip_IAP_PreSectorForReadWrite( ulSector, ulSector ) == IAP_CMD_SUCCESS )
		{
			Chip_IAP_EraseSector( ulSector, ulSector );
		}
		__enable_irq();
	}
	else
	{
		DEBUGOUT( "Config flash sector is already blank.\r\n" );
	}
}
/*-----------------------------------------------------------*/

BaseType_t xReadConfig( void )
{
BaseType_t xReturn = pdFAIL;

	DEBUGOUT( "Read config\r\n" );

	pxConfigInFlash = prvFindConfig();
	xReturn = prvUpdateCache( pxConfigInFlash );

	if( pxConfigInFlash != NULL )
	{
		DEBUGOUT( "Found config version %d, count %d, length %d\r\n", pxConfigInFlash->ucVersion, pxConfigInFlash->ucCount, pxConfigInFlash->usLength );
	}
	else
	{
		DEBUGOUT( "No valid config found.\r\n" );
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

BaseType_t xWriteConfig( void )
{
ConfigWriteHandle_t xWriteHandle = { 0 };
uint16_t usConfigSize;
BaseType_t x, xConfigChanged = pdFALSE, xReturn = pdFAIL;

	DEBUGOUT( "Write config to flash\r\n" );

	/* Iterate over all TLVs to determine total size of config. */
	for( x = 0; x < eConfigNumberOfTags; x++ )
	{
		if( ppxConfigCacheTLVList[ x ] != NULL )
		{
			if( ppxConfigCacheTLVList[ x ]->usLength > 0 )
			{
				xWriteHandle.usLength += prvSizeOfTLV( ppxConfigCacheTLVList[ x ]->usLength );
			}

			/* No need to rewrite config, if all TLVs are still in flash. */
			if( prvTLVInRAM( ppxConfigCacheTLVList[ x ] ) )
			{
				xConfigChanged = pdTRUE;
			}
		}
	}

	/* Only write config if necessary. */
	if( xConfigChanged != pdTRUE )
	{
		DEBUGOUT( "Config write: Config was not changed, no rewrite\r\n");
	}
	else
	{
		usConfigSize = prvSizeOfConfig( xWriteHandle.usLength );

		/* Check if a config exists and there is enough space left in flash sector. */
		if( ( pxConfigInFlash != NULL ) &&
			( ( (uint8_t *) pxConfigInFlash + prvSizeOfConfig( pxConfigInFlash->usLength ) + usConfigSize ) < (uint8_t *) CONFIG_FLASH_AREA_END ) )
		{
			/* Smallest possible buffer size is 256. */
			xWriteHandle.usBufferSize = 256;

			/* Write config behind last one. */
			xWriteHandle.ulDestination = (uint32_t) ( (uint8_t *) pxConfigInFlash + prvSizeOfConfig( pxConfigInFlash->usLength ) );
			xWriteHandle.ucCount = pxConfigInFlash->ucCount + 1;
		}
		else
		{
			/* Flash sector needs to be erased before it can be written again. */
			xWriteHandle.bits.bEraseConfig = pdTRUE;

			/* The addresses should be a 256 byte boundary and the number of bytes
			 *			should be 256 | 512 | 1024 | 4096 */
			if(      usConfigSize <=  256 ) xWriteHandle.usBufferSize =  256;
			else if( usConfigSize <=  512 ) xWriteHandle.usBufferSize =  512;
			else if( usConfigSize <= 1024 ) xWriteHandle.usBufferSize = 1024;
			else if( usConfigSize <= 4096 ) xWriteHandle.usBufferSize = 4096;

			/* _ML_ TODO: If config gets to big to be copied into RAM, need to use a second flash sector to rewrite config. */

			/* Write to the start of the flash sector. */
			xWriteHandle.ulDestination = CONFIG_FLASH_AREA_START;
			xWriteHandle.ucCount = 0;
		}

		xWriteHandle.pucBuffer = pvPortMalloc( xWriteHandle.usBufferSize );
		if( xWriteHandle.pucBuffer != NULL ) {

			prvWriteConfig( &xWriteHandle );

			vPortFree( xWriteHandle.pucBuffer );

			/* Verify that write was successful. */
			if( prvVerifyConfig( (Config_t *) xWriteHandle.ulDestination ) == pdPASS )
			{
				/* Reread config from flash. */
				pxConfigInFlash = (Config_t *) xWriteHandle.ulDestination;
				prvUpdateCache( pxConfigInFlash );

				xReturn = pdPASS;
				DEBUGOUT("Config write: Success\r\n");
			}
		}
		else
		{
			DEBUGOUT("Config write: Failed to allocate %d bytes of memory\r\n", xWriteHandle.usBufferSize);
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

