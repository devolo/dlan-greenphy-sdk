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
#include <stdlib.h>

/* LPCOpen Includes. */
#include "board.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* GreenPHY SDK includes. */
#include "GreenPhySDKConfig.h"
#include "http_query_parser.h"
#include "http_request.h"
#include "clickboard_config.h"
#include "expand2click.h"

/* Task-Delay in ms, change to your preference */
#define TASKWAIT_EXPAND2 100

/*****************************************************************************/

#define EXPAND_ADDR (0x00)   /*jumper A0;A1;A2 on expand2click board*/

//sbit EXPAND_RST at GPIOC_ODR.B2;

unsigned char i = 0, old_res = 0, res;

//extern sfr sbit EXPAND_RST;

char Expander_Read_Byte(char ModuleAddress, char RegAddress){
  I2C_XFER_T xfer;
  char temp;
  xfer.slaveAddr = AddressCode|ModuleAddress;
  uint8_t tmp_data[2];

  tmp_data[0] = RegAddress;
  xfer.txBuff = tmp_data;
  xfer.rxBuff = tmp_data;
  xfer.txSz = 1;
  xfer.rxSz = 1;

  Chip_I2C_MasterSend(I2C1,xfer.slaveAddr,  xfer.txBuff, xfer.txSz);
  Chip_I2C_MasterRead(I2C1,xfer.slaveAddr,  xfer.rxBuff, xfer.rxSz);

  temp   = tmp_data[0];
  return temp;
}

void Expander_Write_Byte(char ModuleAddress,char RegAddress, char Data_) {
	  I2C_XFER_T xfer;
	  xfer.slaveAddr = AddressCode|ModuleAddress;
	  uint8_t tmp_data[2];
	  tmp_data[0] = RegAddress;
	  tmp_data[1] = Data_;
	  xfer.txBuff = tmp_data;
	  //xfer.rxBuff = tmp_data;
	  xfer.txSz = 2;
	  //xfer.rxSz = 2;
	  Chip_I2C_MasterSend(I2C1,xfer.slaveAddr, xfer.txBuff, xfer.txSz);
}

/*****************************************************************************/

/* Task handle used to identify the clickboard's task and check if the
clickboard is activated. */
static TaskHandle_t xClickTaskHandle = NULL;

/* Output bits managed by the Expand2Click task.
If a bit is set to 1 the pin is low. */
static char oBits = 0;

/* Input bits managed by the Expand2Click task.
If a bit is set to 1 the pin is low. */
static char iBits = 0;

/* Configurate on witch pins the water meter is connected */
static char togglePins[2] = { 0, 0 };

/* Count how often the input bits were toggled. */
static int toggleCount[2] = { 0, 0 };

static int multiplicator = 250;

/*-----------------------------------------------------------*/

static char get_expand2click(void){
	return (Expander_Read_Byte(EXPAND_ADDR, GPIOA_BANK0));  // Read expander's PORTA
}
/*-----------------------------------------------------------*/

void set_expand2click(char pins){
	Expander_Write_Byte(EXPAND_ADDR, OLATB_BANK0, pins);    // Write pins to expander's PORTB
}
/*-----------------------------------------------------------*/

static void vClickTask(void *pvParameters)
{
const TickType_t xDelay = pdMS_TO_TICKS( TASKWAIT_EXPAND2 );
char lastBits = get_expand2click();
//char count = 0;

	for(;;)
	{
//		/* Toggle obits once per second - just for demo. */
//		if( count++ % 10 == 0 )
//			oBits ^= ( togglePins[0] | togglePins[1] );

		/* Obtain Mutex. If not possible after xDelay, write debug message. */
		if( xSemaphoreTake( xI2C1_Mutex, xDelay ) == pdTRUE )
		{
			/* I2C is now usable for this Task. Set new Output on Port */
			set_expand2click(oBits);
			/* Get iBits from board */
			iBits = get_expand2click();

			/* Give Mutex back, so other Tasks can use I2C */
			xSemaphoreGive( xI2C1_Mutex );

			/* First water meter pin toggled? */
			if( ( iBits ^ lastBits ) & togglePins[0] )
				toggleCount[0] += 1;

			/* Second water meter pin toggled? */
			if( ( iBits ^ lastBits ) & togglePins[1] )
				toggleCount[1] += 1;

			lastBits = iBits;

			vTaskDelay( xDelay );
		}
		else
		{
			/* The mutex could not be obtained within xDelay. Write debug message. */
			DEBUGOUT( "Expand2 - Error: Could not take I2C1 mutex within %d ms.\r\n", TASKWAIT_EXPAND2 );
		}
	}
}
/*-----------------------------------------------------------*/

#if( includeHTTP_DEMO != 0 )
	static BaseType_t xClickHTTPRequestHandler( char *pcBuffer, size_t uxBufferLength, QueryParam_t *pxParams, BaseType_t xParamCount )
	{
		BaseType_t xCount = 0;
		QueryParam_t *pxParam;

		// Search input object for 'input' Parameter to get the new Value of oBits
		pxParam = pxFindKeyInQueryParams( "output", pxParams, xParamCount );
		if( pxParam != NULL ) {
			oBits = strtol( pxParam->pcValue, NULL, 10 );
		}

		pxParam = pxFindKeyInQueryParams( "pin0", pxParams, xParamCount );
		if( pxParam != NULL ) {
			toggleCount[0] = 0;
			togglePins[0] = strtol( pxParam->pcValue, NULL, 10 );
		}

		pxParam = pxFindKeyInQueryParams( "pin1", pxParams, xParamCount );
		if( pxParam != NULL ) {
			toggleCount[1] = 0;
			togglePins[1] = strtol( pxParam->pcValue, NULL, 10 );
		}

		pxParam = pxFindKeyInQueryParams( "multi", pxParams, xParamCount );
		if( pxParam != NULL ) {
			multiplicator = strtol( pxParam->pcValue, NULL, 10 );
		}

		xCount += sprintf( pcBuffer, "{"
				"\"input\":"  "%d,"
				"\"output\":" "%d,"
				"\"count0\":" "%d,"
				"\"count1\":" "%d,"
				"\"pin0\":"   "%d,"
				"\"pin1\":"   "%d,"
				"\"multi\":"  "%d"
			"}",
			iBits,
			oBits,
			toggleCount[0],
			toggleCount[1],
			togglePins[0],
			togglePins[1],
			multiplicator
		);
		return xCount;
	}
#endif
/*-----------------------------------------------------------*/

BaseType_t xExpand2Click_Init ( const char *pcName, BaseType_t xPort )
{
BaseType_t xReturn = pdFALSE;

	/* Use the task handle to guard against multiple initialization. */
	if( xClickTaskHandle == NULL )
	{
		DEBUGOUT( "Initialize Expand2Click on port %d.\r\n", xPort );

		/* Configure GPIOs depending on the microbus port. */
		if( xPort == eClickboardPort1 )
		{
			/* Set interrupt pin. */
			Chip_GPIO_SetPinDIRInput(LPC_GPIO, CLICKBOARD1_INT_GPIO_PORT_NUM, CLICKBOARD1_INT_GPIO_BIT_NUM);
			/* Set reset pin. */
			Chip_GPIO_SetPinDIROutput(LPC_GPIO, CLICKBOARD1_RST_GPIO_PORT_NUM, CLICKBOARD1_RST_GPIO_BIT_NUM);
			/* Start  reset procedure. */
			Chip_GPIO_SetPinOutHigh(LPC_GPIO, CLICKBOARD1_RST_GPIO_PORT_NUM, CLICKBOARD1_RST_GPIO_BIT_NUM);
			Chip_GPIO_SetPinOutLow( LPC_GPIO, CLICKBOARD1_RST_GPIO_PORT_NUM, CLICKBOARD1_RST_GPIO_BIT_NUM);
			Chip_GPIO_SetPinOutHigh(LPC_GPIO, CLICKBOARD1_RST_GPIO_PORT_NUM, CLICKBOARD1_RST_GPIO_BIT_NUM);
		}
		else if( xPort == eClickboardPort2 )
		{
			/* Set interrupt pin. */
			Chip_GPIO_SetPinDIRInput(LPC_GPIO, CLICKBOARD2_INT_GPIO_PORT_NUM, CLICKBOARD2_INT_GPIO_BIT_NUM);
			/* Set reset pin. */
			Chip_GPIO_SetPinDIROutput(LPC_GPIO, CLICKBOARD2_RST_GPIO_PORT_NUM, CLICKBOARD2_RST_GPIO_BIT_NUM);
			/* Start  reset procedure. */
			Chip_GPIO_SetPinOutHigh(LPC_GPIO, CLICKBOARD2_RST_GPIO_PORT_NUM, CLICKBOARD2_RST_GPIO_BIT_NUM);
			Chip_GPIO_SetPinOutLow( LPC_GPIO, CLICKBOARD2_RST_GPIO_PORT_NUM, CLICKBOARD2_RST_GPIO_BIT_NUM);
			Chip_GPIO_SetPinOutHigh(LPC_GPIO, CLICKBOARD2_RST_GPIO_PORT_NUM, CLICKBOARD2_RST_GPIO_BIT_NUM);
		}

		/* Initialize I2C. Both microbus ports are connected to the same I2C bus. */
		Board_I2C_Init( I2C1 );
		if( xSemaphoreTake( xI2C1_Mutex, portMAX_DELAY ) == pdTRUE )
		{
			/* Initialize Expand2Click chip. */
			Expander_Write_Byte(EXPAND_ADDR, IODIRB_BANK0, 0x00);  // Set Expander's PORTB to be output
			Expander_Write_Byte(EXPAND_ADDR, IODIRA_BANK0, 0xFF);  // Set Expander's PORTA to be input
			Expander_Write_Byte(EXPAND_ADDR, GPPUA_BANK0, 0xFF);   // Set pull-ups to all of the Expander's PORTA pins

			/* Give Mutex back, so other Tasks can use I2C */
			xSemaphoreGive( xI2C1_Mutex );

			/* Create task. */
			xTaskCreate( vClickTask, pcName, 240, NULL, ( tskIDLE_PRIORITY + 1 ), &xClickTaskHandle );
		}

		if( xClickTaskHandle != NULL )
		{
			#if( includeHTTP_DEMO != 0 )
			{
				/* Add HTTP request handler. */
				xAddRequestHandler( pcName, xClickHTTPRequestHandler );
			}
			#endif

			xReturn = pdTRUE;
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

BaseType_t xExpand2Click_Deinit ( void )
{
BaseType_t xReturn = pdFALSE;

	if( xClickTaskHandle != NULL )
	{
		DEBUGOUT( "Deinitialize Expand2Click.\r\n" );

		#if( includeHTTP_DEMO != 0 )
		{
			/* Use the task's name to remove the HTTP Request Handler. */
			xRemoveRequestHandler( pcTaskGetName( xClickTaskHandle ) );
		}
		#endif

		/* Delete the task. */
		vTaskDelete( xClickTaskHandle );
		/* Set the task handle to NULL, so the clickboard can be reactivated. */
		xClickTaskHandle = NULL;

		/* TODO: Reset I2C and GPIOs. */
		xReturn = pdTRUE;
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

