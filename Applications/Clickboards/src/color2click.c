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
#include "color2click.h"
#include "save_config.h"

/* MQTT includes */
#include "mqtt.h"

/* Task-Delay in ms, change to your preference */
#define TASKWAIT_COLOR2 1000 /* 1s */

/*****************************************************************************/

uint8_t _addr = 0x44; //ISL29125 default ID;

struct Colors {
	int red;
	int green;
	int blue;
	int status;
} color;

uint8_t read8(uint8_t reg);
void write8(uint8_t reg, uint8_t data);

uint16_t read16(uint8_t reg);
void write16(uint8_t reg, uint16_t data);

/* Reset all registers - returns true if successful */
bool reset_ISL29125() {
	uint8_t data = 0x00;
	/* Reset registers */
	write8(DEVICE_ID, 0x46);
	/* Check reset */
	data = read8(CONFIG_1);
	data |= read8(CONFIG_2);
	data |= read8(CONFIG_3);
	data |= read8(STATUS);
	if (data != 0x00) {
		return false;
	}
	return true;
}

/* Setup Configuration registers (three registers) - returns true if successful
 // Use CONFIG1 variables from ISL29125_SOFT.h for first parameter config1, CONFIG2 for config2, 3 for 3
 // Use CFG_DEFAULT for default configuration for that register */
bool config_ISL29125(uint8_t config1, uint8_t config2, uint8_t config3) {
	bool ret = true;
	uint8_t data = 0x00;

	/* Set 1st configuration register */
	write8(CONFIG_1, config1);
	/* Set 2nd configuration register */
	write8(CONFIG_2, config2);
	/* Set 3rd configuration register */
	write8(CONFIG_3, config3);

	/* Check if configurations were set correctly */
	data = read8(CONFIG_1);
	if (data != config1) {
		ret &= false;
	}
	data = read8(CONFIG_2);
	if (data != config2) {
		ret &= false;
	}
	data = read8(CONFIG_3);
	if (data != config3) {
		ret &= false;
	}
	return ret;
}

/* Verifies sensor is there by checking its device ID
 / Resets all registers/configurations to factory default
 / Sets configuration registers for the common use case */
bool ISL29125_SOFT_init(void) {
	bool ret = true;
	uint8_t data = 0x00;
	data = read8(DEVICE_ID);
	if (data != 0x7D) {
		ret &= false;
	}
	/* Reset registers */
	ret &= reset_ISL29125();
	/* Set to RGB mode, 10k lux, and high IR compensation */
	ret &= config_ISL29125(CFG1_MODE_RGB | CFG1_10KLUX, CFG2_IR_ADJUST_HIGH,
	CFG_DEFAULT);
	return ret;
}

/* Sets upper threshold value for triggering interrupts */
void setUpperThreshold(uint16_t data) {
	write16(THRESHOLD_HL, data);
}

/* Sets lower threshold value for triggering interrupts */
void setLowerThreshold(uint16_t data) {
	write16(THRESHOLD_LL, data);
}

/* Check what the upper threshold is, 0xFFFF by default */
uint16_t readUpperThreshold() {
	return read16(THRESHOLD_HL);
}

/* Check what the upper threshold is, 0x0000 by default */
uint16_t readLowerThreshold() {
	return read16(THRESHOLD_LL);
}

/* Read the latest Sensor ADC reading for the color Red */
uint16_t readRed() {
	return read16(RED_L);
}

/* Read the latest Sensor ADC reading for the color Green */
uint16_t readGreen() {
	return read16(GREEN_L);
}

/* Read the latest Sensor ADC reading for the color Blue */
uint16_t readBlue() {
	return read16(BLUE_L);
}

/* Check status flag register that allows for checking for interrupts, brownouts, and ADC conversion completions */
uint8_t readStatus() {
	return read8(STATUS);
}

/* Generic I2C read register (single byte) */
uint8_t read8(uint8_t reg) {
	I2C_XFER_T xfer;
	xfer.slaveAddr = _addr;
	uint8_t tmp_data[2];
	tmp_data[0] = reg;
	xfer.txBuff = tmp_data;
	xfer.rxBuff = tmp_data;
	xfer.txSz = 1;
	xfer.rxSz = 2;
	/*transfer data to T-chip via I2C and read new data, check if error has occurred*/
	if (Chip_I2C_MasterTransfer(I2C1, &xfer) != I2C_STATUS_DONE)
		return 0xaf;
	return tmp_data[0];
}

/* Generic I2C write data to register (single byte) */
void write8(uint8_t reg, uint8_t data) {
	I2C_XFER_T xfer;
	xfer.slaveAddr = _addr;
	uint8_t tmp_data[2];
	tmp_data[0] = reg;
	tmp_data[1] = data;
	xfer.txBuff = tmp_data;
	xfer.txSz = 2;
	Chip_I2C_MasterSend(I2C1, xfer.slaveAddr, xfer.txBuff, xfer.txSz);
	return;
}

/* Generic I2C read registers (two bytes, LSB first) */
uint16_t read16(uint8_t reg) {
	I2C_XFER_T xfer;
	xfer.slaveAddr = _addr;
	uint8_t tmp_data[3];
	tmp_data[0] = reg;
	xfer.txBuff = tmp_data;
	xfer.rxBuff = tmp_data;
	xfer.txSz = 1;
	xfer.rxSz = 3;
	/*transfer data to T-chip via I2C and read new data, check if error has occurred*/
	if (Chip_I2C_MasterTransfer(I2C1, &xfer) != I2C_STATUS_DONE)
		return 0xaf;

	return ((tmp_data[1] << 8) | tmp_data[0]);

}

/* Generic I2C write data to registers (two bytes, LSB first) */
void write16(uint8_t reg, uint16_t data) {
	I2C_XFER_T xfer;
	xfer.slaveAddr = _addr;
	uint8_t tmp_data[3];
	tmp_data[0] = reg;
	tmp_data[1] = data;
	tmp_data[2] = (data >> 8);
	xfer.txBuff = tmp_data;
	xfer.txSz = 3;
	Chip_I2C_MasterSend(I2C1, xfer.slaveAddr, xfer.txBuff, xfer.txSz);
	return;
}

void PrintStatus(void) {
	DEBUGOUT(" Status: ");
	if (color.status & FLAG_INT)
		DEBUGOUT("FLAG_INT ");
	if (color.status & FLAG_CONV_DONE)
		DEBUGOUT("CONV_DONE ");
	if (color.status & FLAG_BROWNOUT)
		DEBUGOUT("FLAG_BROWNOUT ");
	if ((color.status & FLAG_CONV_B) == FLAG_CONV_B)
		DEBUGOUT("B");
	else if (color.status & FLAG_CONV_R)
		DEBUGOUT("R");
	else if (color.status & FLAG_CONV_G)
		DEBUGOUT("G");

}

void ReadColors(void) {
	color.red = readRed();
	color.green = readGreen();
	color.blue = readBlue();
	color.status = readStatus();
}

/*****************************************************************************/

/* Task handle used to identify the clickboard's task and check if the
clickboard is activated. */
static TaskHandle_t xClickTaskHandle = NULL;

/*-----------------------------------------------------------*/

static void vClickTask(void *pvParameters) {
const TickType_t xDelay = pdMS_TO_TICKS( TASKWAIT_COLOR2 );
BaseType_t xTime = ( portGET_RUN_TIME_COUNTER_VALUE() / 10000UL );

#if( netconfigUSEMQTT != 0 )
	char buffer[60];
	QueueHandle_t xMqttQueue = xGetMQTTQueueHandle();
	MqttJob_t xJob;
	MqttPublishMsg_t xPublish;

	/* Set all connection details only once */
	xJob.eJobType = ePublish;
	xJob.data = (void *) &xPublish;

	xPublish.xMessage.qos = 0;
	xPublish.xMessage.retained = 0;
	xPublish.xMessage.payload = NULL;
#endif /* #if( netconfigUSEMQTT != 0 ) */

	for(;;)
	{
		/* Obtain Mutex. If not possible after 1 Second, write Debug and proceed */
		if( xSemaphoreTake( xI2C1_Mutex, xDelay ) == pdTRUE )
		{
			/* I2C is now usable for this Task. Read Color Values. */
			ReadColors();

			/* Give Mutex back, so other Tasks can use I2C */
			xSemaphoreGive( xI2C1_Mutex );

			/* Print a debug message once every 10 s. */
			if( ( portGET_RUN_TIME_COUNTER_VALUE() / 10000UL ) > xTime + 10 )
			{
				DEBUGOUT("Color2click - red: %d; green: %d; blue: %d", color.red,
						color.green, color.blue);
				PrintStatus();
				DEBUGOUT("\r\n");
			#if( netconfigUSEMQTT != 0 )
				xMqttQueue = xGetMQTTQueueHandle();
				if( xMqttQueue != NULL )
				{
					if(xPublish.xMessage.payload == NULL)
					{
						/* _CD_ set payload each time, because mqtt task set payload to NULL, so calling task knows package is sent.*/
						xPublish.xMessage.payload = buffer;
						xPublish.pucTopic = (char *)pvGetConfig( eConfigColorTopic, NULL );
						sprintf(buffer, "{\"meaning\":\"color\",\"value\":\"r:%d,g:%d,b:%d\"}", color.red, color.green, color.blue);
						xPublish.xMessage.payloadlen = strlen(buffer);
						xQueueSendToBack( xMqttQueue, &xJob, 0 );
					}
				}
			#endif /* #if( netconfigUSEMQTT != 0 ) */
				xTime = ( portGET_RUN_TIME_COUNTER_VALUE() / 10000UL );
			}

			vTaskDelay( xDelay );
		}
		else
		{
			/* The mutex could not be obtained within xDelay. Write debug message. */
			DEBUGOUT( "Color2 - Error: Could not take I2C1 mutex within %d ms.\r\n", TASKWAIT_COLOR2 );
		}
	}
}
/*-----------------------------------------------------------*/

#if( includeHTTP_DEMO != 0 )
	static BaseType_t xClickHTTPRequestHandler( char *pcBuffer, size_t uxBufferLength, QueryParam_t *pxParams, BaseType_t xParamCount )
	{
		BaseType_t xCount = 0;
		QueryParam_t *pxParam;

		pxParam = pxFindKeyInQueryParams( "ctopic", pxParams, xParamCount );
		if( pxParam != NULL )
			pvSetConfig( eConfigColorTopic, strlen(pxParam->pcValue) + 1, pxParam->pcValue );


		xCount += sprintf( pcBuffer, "{\"r\":%d,\"g\":%d,\"b\":%d}",
				color.red, color.green, color.blue );

	#if( netconfigUSEMQTT != 0 )
		char buffer[50];
		char *pcTopic = (char *)pvGetConfig( eConfigColorTopic, NULL );
		if( pcTopic != NULL )
		{
			strcpy( buffer, pcTopic );
			vCleanTopic( buffer );
			xCount += sprintf( pcBuffer + ( xCount -1 ), ",\"ctopic\":\"%s\"}", buffer );
		}
	#endif /* #if( netconfigUSEMQTT != 0 ) */

		return xCount;
	}
#endif
/*-----------------------------------------------------------*/

BaseType_t xColor2Click_Init ( const char *pcName, BaseType_t xPort )
{
BaseType_t xReturn = pdFALSE;

	/* Use the task handle to guard against multiple initialization. */
	if( xClickTaskHandle == NULL )
	{
		DEBUGOUT( "Initialize Color2Click on port %d.\r\n", xPort );
		/* Configure GPIOs depending on the microbus port. */
		if( xPort == eClickboardPort1 )
		{
			/* Set interrupt pin. */
			Chip_GPIO_SetPinDIRInput( LPC_GPIO, CLICKBOARD1_INT_GPIO_PORT_NUM, CLICKBOARD1_INT_GPIO_BIT_NUM );
		}
		else if( xPort == eClickboardPort2 )
		{
			/* Set interrupt pin. */
			Chip_GPIO_SetPinDIRInput( LPC_GPIO, CLICKBOARD2_INT_GPIO_PORT_NUM, CLICKBOARD2_INT_GPIO_BIT_NUM );
		}


		/* Initialize I2C. Both microbus ports are connected to the same I2C bus. */
		Board_I2C_Init( I2C1 );
		if( xSemaphoreTake( xI2C1_Mutex, portMAX_DELAY ) == pdTRUE )
		{
			/* Initialze the Color2Click chip. */
			ISL29125_SOFT_init();
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

BaseType_t xColor2Click_Deinit ( void )
{
BaseType_t xReturn = pdFALSE;

	if( xClickTaskHandle != NULL )
	{
		DEBUGOUT( "Deinitialize Color2Click.\r\n" );

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
