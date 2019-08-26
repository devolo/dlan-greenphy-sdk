/*
 * MQTTEcho_Test.c
 *
 *  Created on: 10.08.2017
 *      Author: CHristoph.Domnik
 *
 *  This program is only used to try out the MQTT functionalitie
 *  and will not beimplementet in the later builds
 */


/* Standard includes. */
#include <stdint.h>
#include <stdlib.h>

#include "MQTTClient.h"
#include "MQTTEcho_Test.h"
#include "MQTTFreeRTOS.h"


void messageArrived(MessageData* data)
{
	printf("Message arrived on topic %.*s: %.*s\n", data->topicName->lenstring.len, data->topicName->lenstring.data,
		data->message->payloadlen, data->message->payload);
}


static void prvMQTTEchoTask(void *pvParameters)
{
	vTaskDelay(10000); // Wait 10s to don't interrupt the IP Stack StartUp

	MQTTClient client;
	Network network;
	unsigned char sendbuf[100], readbuf[10];
	int rc = 0,
		count = 0;
	MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

	pvParameters = 0;
	NetworkInit(&network);
	MQTTClientInit(&client, &network, 20000, sendbuf, sizeof(sendbuf), readbuf, sizeof(readbuf));
#if defined(MQTT_TASK)
	client.thread.task = NULL;
#endif
	// char* address = MQTT_SERVER;
	char *address = "broker.hivemq.com";
	rc = NetworkConnect( &network, address, 1883 );
	printf("Return code from network connect is %d\n", rc);
	if ( rc != 0 )
		goto exit;

#if defined(MQTT_TASK)
	if ((rc = MQTTStartTask(&client)) != pdPASS)
		printf("Return code from start tasks is %d\n", rc);
#endif

	connectData.MQTTVersion = 4;
	connectData.clientID.cstring = MQTT_CLIENTID;
	connectData.keepAliveInterval = 300;
	connectData.cleansession = 1;
	connectData.willFlag = 0;
	//connectData.username.cstring = MQTT_USER;
	//connectData.password.cstring = MQTT_PASSWORD;

	rc = MQTTConnect(&client, &connectData);
	printf("Return code from MQTT connect is %d\n", rc);
	if ( rc != 0 ){
		MQTTDisconnect( &client );
		goto exit;
	}
	else
		printf("MQTT Connected\n");

	while (++count)
	{
		MQTTMessage message;
		char payload[30];

		message.qos = 0;
		message.retained = 0;
		message.payload = payload;
		//sprintf(payload, "{\"meaning\":\"TestValue\", \"value\":%d}", count);
		sprintf(payload, "Message:%d", count);
		message.payloadlen = strlen(payload);

		//rc = MQTTPublish(&client, MQTT_TOPIC, &message);
		rc = MQTTPublish(&client, "testtopic/5", &message);
		printf("Return code from MQTT publish is %d\n", rc);
		if ( rc != 0 ) {
			MQTTDisconnect( &client );
			goto exit;
		}
#if !defined(MQTT_TASK)
		rc = MQTTYield(&client, 1000);
		printf("Return code from MQTT yield is %d\n", rc);
		if ( rc != 0 ) {
			MQTTDisconnect( &client );
			goto exit;
		}
#endif

		vTaskDelay(1000);
	}

exit:
	FreeRTOS_shutdown( network.my_socket, FREERTOS_SHUT_RDWR );

	rc = 0;
	while( FreeRTOS_recv( network.my_socket, readbuf, 0, 0 ) >= 0 || ( rc == 20 ) )
    {
        vTaskDelay( 250 );
        rc++;
    }

    /* The socket has shut down and is safe to close. */
    FreeRTOS_closesocket( network.my_socket );


	printf("could not connect\n");
#if defined(MQTT_TASK)
	if( client.thread.task != NULL )
		vTaskDelete( client.thread.task );
#endif
	vTaskDelete( NULL );
}


void vStartMQTTTasks(uint16_t usTaskStackSize, UBaseType_t uxTaskPriority)
{
	BaseType_t x = 0L;

	xTaskCreate(prvMQTTEchoTask,	/* The function that implements the task. */
			"MQTTEcho0",			/* Just a text name for the task to aid debugging. */
			usTaskStackSize,	/* The stack size is defined in FreeRTOSIPConfig.h. */
			(void *)x,		/* The task parameter, not used in this case. */
			uxTaskPriority,		/* The priority assigned to the task is defined in FreeRTOSConfig.h. */
			NULL);				/* The task handle is not used. */
}
/*-----------------------------------------------------------*/
