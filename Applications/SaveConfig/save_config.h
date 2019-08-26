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

/***********************************************
 *            CONFIG VARIABLES
 *
 * Add new variables like below with:
 *     X( ID, NAME ) \
 *
 * Keep IDs backwards compatible!
 *
 * IDs 0 - 127 are reserved for use by devolo.
 * IDs 128-255 are free for customer's use.
 ***********************************************/
#define LIST_OF_CONFIG_TAGS           \
	X(   0, eConfigNetworkIp )        \
	X(   1, eConfigNetworkNetmask )   \
	X(   2, eConfigNetworkGateway )   \
	X(   3, eConfigNetworkHostname )  \
	X(   4, eConfigNetworkMqttOnPwr ) \
	X(  10, eConfigClickConfPort1 )   \
	X(  11, eConfigClickConfPort2 )   \
	X(  20, eConfigMqttBroker )       \
	X(  21, eConfigMqttPort )         \
	X(  22, eConfigMqttClientID )     \
	X(  23, eConfigMqttUser )         \
	X(  24, eConfigMqttPassWD )       \
	X(  25, eConfigMqttWill )         \
	X(  26, eConfigMqttWillTopic )    \
	X(  27, eConfigMqttWillMsg )      \
	X(  30, eConfigExpandPin1 )       \
	X(  31, eConfigExpandPin2 )       \
	X(  32, eConfigExpandMult )       \
	X(  33, eConfigExpandTopic1 )     \
	X(  34, eConfigExpandTopic2 )     \
	X(  41, eConfigThermoTopic )      \
	X(  51, eConfigColorTopic )       \
	/* ADD YOUR OWN TAGS BELOW */     \
	// X( 128, eConfigCustom )

/* Storage location of the config in flash.
Config must not exceed boundaries of a 32kb sector. */
#define CONFIG_FLASH_AREA_START 	 ( 0x78000 ) /* Sector 29 */
#define CONFIG_FLASH_AREA_SIZE       ( 0x08000 ) /* 32kb */
#define CONFIG_FLASH_AREA_END        ( CONFIG_FLASH_AREA_START + CONFIG_FLASH_AREA_SIZE )

#define CONFIG_SIGNATURE             ( 0xAAAA5555 )
#define LPC_FLASH_SIZE_512KB         ( 0x00080000 )

/* X-Macro definition of config tags.
 * Use LIST_OF_CONFIG_TAGS macro above to add new tags. */
typedef enum eCONFIG_TAGS
{
#define X( id, name ) name = id,
	LIST_OF_CONFIG_TAGS
#undef X
} eConfigTag_t;

/* Read and verify latest config and store pointers to the flash location of
 * the found config variables. */
BaseType_t xReadConfig( void );

/* (Re-)writes config to flash. Afterwards config is reread from flash, all cached
 * config variables are freed and set to the new flash location. */
BaseType_t xWriteConfig( void );

/* Erases the whole config sector. */
void vEraseConfig( void );

/* Returns pointer to a config variable matching xTag with returned length pusLength. */
void *pvGetConfig( eConfigTag_t xTag, uint16_t *pusLength );

/* Stores pvValue of length usLength in config cache. Use xWriteConfig() to save permanently.
 * Previously cached config variables are freed. Returns pointer to config variable in cache. */
void *pvSetConfig( eConfigTag_t xTag, uint16_t usLength, const void * const pvValue );
