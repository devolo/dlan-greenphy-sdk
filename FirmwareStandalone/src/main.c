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
#include <cr_section_macros.h>

/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include "task.h"
#include "queue.h"

/* FreeRTOS +TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

/* GreenPHY SDK includes. */
#include "GreenPhySDKConfig.h"
#include "network.h"
#include "save_config.h"
#include "clickboard_config.h"
#include "mqtt.h"

//#define TESTMQTT

/*-----------------------------------------------------------*/
static void prvTestTask( void *pvParameters )
{
#ifdef TESTMQTT
	QueueHandle_t xMqttQueue;
	MqttJob_t xMqttJob;
	MqttPublishMsg_t xPublish;
	char buffer[16];
	char topic [14] = "testtopic/5";
	int i = 0;
	vTaskDelay(5000);

	DEBUGOUT( "Test task running. -> Test MQTT\n" );

	/* Add endless loop here to prevent task deletion. */
	xMqttQueue = xInitMQTT();
	xMqttJob.eJobType = eConnect;
	xQueueSendToBack( xMqttQueue, &xMqttJob, 0 );

	xMqttJob.eJobType = ePublish;
	xMqttJob.data = (void *) &xPublish;

	xPublish.pucTopic = topic;
	xPublish.xMessage.qos = 0;
	xPublish.xMessage.retained = 0;
	for(;;)
	{
		if(xPublish.xMessage.payload == NULL)
		{
			xPublish.xMessage.payload = buffer;
			sprintf(buffer, "Message %d", i++ );
			xPublish.xMessage.payloadlen = strlen(buffer);
			xQueueSendToBack( xMqttQueue, &xMqttJob, 0 );
		}

		vTaskDelay(2000);
	}
	xMqttJob.eJobType = eDisconnect;
	xQueueSendToBack( xMqttQueue, &xMqttJob, 0 );
#endif

	vTaskDelete( NULL );

}

void vStartEthTasks( void )
{
	xTaskCreate( prvTestTask, "Test", 240, NULL,  1, NULL );
}


int main(void) {
	SystemCoreClockUpdate();
	Board_Init();

	DEBUGSTR("\r\n\r\nSTANDALONE ");
	{
		uint32_t reset_reason = LPC_SYSCTL->RSID;
		DEBUGOUT("RSID:0x%x", reset_reason);
		if (!reset_reason)
			DEBUGSTR("->Bootloader");
		if (reset_reason & 0x1)
			DEBUGSTR("->Power On");
		if (reset_reason & 0x2)
			DEBUGSTR("->Reset");
		if (reset_reason & 0x4)
			DEBUGSTR("->Watchdog");
		if (reset_reason & 0x8)
			DEBUGSTR("->BrownOut Detection");
		if (reset_reason & 0x10)
			DEBUGSTR("->JTAG/restart");
		DEBUGSTR("\r\n");
		LPC_SYSCTL->RSID = reset_reason;
	}
	DEBUGOUT( "Firmware-Version: %s\n", BUILD_STRING );

	xReadConfig();

	vNetworkInit();

	xClickboardsInit();

	//xTaskCreate( prvTestTask, "Test", 240, NULL,  1, NULL );

	vTaskStartScheduler();

	return 0;
}
