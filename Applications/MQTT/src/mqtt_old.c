/*
 * mqtt.c
 *
 *  Created on: 31.08.2017
 *      Author: christoph.domnik
 *
 *  This Module is used to present all MQTT features to the system.
 *  Containing Functions for Initializing, Deinitializing, Connect, Publish functions
 *
 *  To use this module include mqtt.h
 */
/* LPCOpen Includes. */
#include "board.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* GreenPHY SDK and Configuration includes */
#include "GreenPhySDKConfig.h"
#include "GreenPhySDKNetConfig.h"
#include "save_config.h"
#include "http_query_parser.h"
#include "http_request.h"

/* Include MQTT header files */
#include "mqtt_old.h"
#include "MQTTClient.h"

BaseType_t xInitMqtt();

/* xIsUsed will be set to 1 each time an action including the Client will start
 * and will be reset when the action is finished. (Mutex don't work) */
static BaseType_t xIsUsed = 0;

/* save the initialization status, so publish could not be used untial the initialization */
static unsigned char ucInitialized = 0;

/* Client will be saved for Task an to check if it is already initialized */
MQTTClient xClient;

/* Network varialbe used for this connection contains read,
 * write and disconnect function and tcp socket */
Network xNetwork;

/* Task Handle is used to identify the Task which runs the Mqtt-Yield function */
static TaskHandle_t xMqttTaskHandle = NULL;

/* Global send buffer, _CD_ not shure of final solution */
unsigned char pcSendBuf[ mqttSEND_BUFFER_SIZE ];

/* Global receive buffer, _CD_ not shure of final solution */
unsigned char pcRecvBuf[ mqttRECV_BUFFER_SIZE ];



/********************************************************************************************************
 * FUNCTIONS:
 ********************************************************************************************************/

/***********************************************************************************************************
 *   function: xIsActive
 *   Purpose: returns the Active state of the Mqtt-Client
 *   Global Variables used: ucInitialized
 *   Returns: 0 - not active
 *   		  1 - active
 ***********************************************************************************************************/
BaseType_t xIsActive()
{
	BaseType_t xRet = 0;
	if( ( ucInitialized == 1 ) && ( xClient.isconnected == 1 ) )
	{
		xRet = 1;
	}
	return xRet;
}


/***********************************************************************************************************
 *   function: vUnblockClientHandle
 *   Purpose: Reset xIsUsed so other functions now can use the client handle
 *   Global Variables used: xIsUsed
 ***********************************************************************************************************/
void vUnblockClientHandle()
{
	xIsUsed = CLIENT_FREE;
}


/***********************************************************************************************************
 *   function: xBlockClientHandle
 *   Purpose: Check if client handle is already in use, to don't corrupt it
 *   Global Variables used: xIsUsed
 *   Returns: CLIENT_FREE - client handle is reserved for caller function, you could use it
 *   		  -CLIENT_BLOCKED - client handle is blocked by an other function, please try again later
 ***********************************************************************************************************/
BaseType_t xBlockClientHandle()
{
	int i = 0;

	/* Wait for client handle to become unblocked, but do not wait more than 1s */
	while(( xIsUsed != CLIENT_FREE ) && ( i < 5 )){
		vTaskDelay( 200 );
		i++;
	}

	if( xIsUsed == CLIENT_FREE ){
		xIsUsed = CLIENT_BLOCKED;
		return CLIENT_FREE;
	}
	return -CLIENT_BLOCKED;
}


/***********************************************************************************************************
 *   function: xPublishMessage
 *   Purpose: Publishes a text message to the connection initialized with xInitMqtt
 *   Global Variables used: xClientHandle
 *   Returns: 0 - Message send successful
 *   		  -1 - can not send message, client not initialized or blocked
 ***********************************************************************************************************/
BaseType_t xPublishMessage( char *pucMessage, char *pucTopic, unsigned char ucQos, unsigned char ucRetained )
{
	if(( xClient.isconnected ) && ( ucInitialized != 0 )){
		MQTTMessage xMessage;
		int ret = SUCCESS_MQTT;

		xMessage.qos = ucQos;
		xMessage.retained = ucRetained;
		xMessage.payload = pucMessage;
		xMessage.payloadlen = strlen(pucMessage);

		/* Check if client is free */
		if( xBlockClientHandle() == CLIENT_FREE ){
			ret = MQTTPublish( &xClient, pucTopic, &xMessage );
			if( ret != SUCCESS_MQTT ){
				DEBUGOUT("MQTT-ERROR 0x0120: Failure while publishing packet. Will end connection.\n");
				xDeinitMqtt();
			}
			/* done, now unblock client */
			vUnblockClientHandle();
			return ret;
		}
	}
	return -1;
}


/***********************************************************************************************************
 *   function: vMqttTask
 *   Purpose: This function checks if there are any mqtt packets received by calling MQTTYield.
 *   		  Task repeats every one second.
 *   Global Variables used: xClientHandle
 ***********************************************************************************************************/
void vMqttTask()
{
	int ret = SUCCESS_MQTT;

	for(;;){
		/* _CD_ wait as first step, because when task is calling nothing is to do */
		vTaskDelay( mqttRECEIVE_DELAY );

		/* Check if client is free */
		if(( xBlockClientHandle() == CLIENT_FREE ) && ( xClient.isconnected ) ){
			ret = MQTTYield( &xClient, 100 );
			/* done, now unblock client */
			vUnblockClientHandle();

			if( ret != SUCCESS_MQTT )
				DEBUGOUT("MQTT-ERROR 0x0110: Failure while reading packet. Will end connection.\n");
		}
	}

}


/***********************************************************************************************************
 *   function: xEndSocket
 *   Purpose: Close the TCP socket of the client (help-function)
 *   Global Variables used: xClientHandle
 *   Returns: 	0 - everything is ok
 *   			-1 - no socket available
 *   			something else - FreeRTOS_shutdown return value
 ***********************************************************************************************************/
BaseType_t xEndSocket()
{
	int ret = SUCCESS_MQTT;
	if(( xClient.ipstack->my_socket != FREERTOS_INVALID_SOCKET ) && ( xClient.ipstack != NULL )){
		/* Exit save */
		ret = FreeRTOS_shutdown( xClient.ipstack->my_socket, FREERTOS_SHUT_RDWR );
		while( FreeRTOS_recv( xClient.ipstack->my_socket, xClient.readbuf, 0, 0 ) >= 0 ){
			vTaskDelay( 250 );
		}
		/* The socket has shut down and is safe to close. */
		FreeRTOS_closesocket( xClient.ipstack->my_socket );
		return ret;
	}
	return -1;
}


/***********************************************************************************************************
 *   function: xReadCredentialsFromConfig
 *   Purpose: Get The Credentials from Config, when no Value is stored for a Value, do not touch the value. Please initialize xCred before using this function.
 *   Parameters: IN/OUT: xMqttCredentials_t *xCred
 *   Returns: 	X - Number of values obtained from Config, other fields not touched (max: 8, if will=0 -> max: 6)
 *   			0 - Error while Reading Config (all values are default)
 ***********************************************************************************************************/
BaseType_t xGetCredentials( xMqttCredentials_t *xCred )
{
	BaseType_t xReturn = pdFAIL;
	void *pvValue = NULL;

	/* ----------------------Set default credentials---------------------- */
	xCred->pcBrokername = netconfigMQTT_BROKER;
	xCred->port = netconfigMQTT_PORT;
	xCred->connectData->clientID.cstring = netconfigMQTT_CLIENT;
	xCred->connectData->username.cstring = netconfigMQTT_USER;
	xCred->connectData->password.cstring = netconfigMQTT_PASSWORT;

	/* ----------------------Now get all saved Fields---------------------- */
	/* Get Broker address */
	pvValue = pvGetConfig( eConfigMqttBroker, NULL );
	if( pvValue != NULL )
	{
		xCred->pcBrokername = (char *)pvValue;
		xReturn++;
	}

	/* Get Broker port */
	pvValue = pvGetConfig( eConfigMqttPort, NULL );
	if( pvValue != NULL )
	{
		xCred->port = strtol( (char *)pvValue, NULL, 6 );
		xReturn++;
	}

	/* Get CLientID */
	pvValue = pvGetConfig( eConfigMqttClientID, NULL );
	if( pvValue != NULL )
	{
		xCred->connectData->clientID.cstring = (char *)pvValue;
		xReturn++;
	}

	/* if no username is used, there will be no entry in configuration (TLV with length=0 will not be saved) */
	pvValue = pvGetConfig( eConfigMqttUser, NULL );
	if( pvValue != NULL )
	{
		xCred->connectData->username.cstring = (char *)pvValue;
		xReturn++;
	}

	/* if no password is used, there will be no entry in configuration (TLV with length=0 will not be saved) */
	pvValue = pvGetConfig( eConfigMqttUser, NULL );
	if( pvValue != NULL )
	{
		xCred->connectData->username.cstring = (char *)pvValue;
		xReturn++;
	}

	/* Get will flag, if it is set, read the other will options */
	pvValue = pvGetConfig( eConfigMqttWill, NULL );
	if( pvValue != NULL )
	{
		xCred->connectData->willFlag = (unsigned char)(strtol( (char *)pvValue, NULL, 2 ));
		xReturn++;
		if(xCred->connectData->willFlag == 1)
		{
			/* _CD_ QOS and retain will not be saved in config, so use default values */
			xCred->connectData->will.qos = netconfigMQTT_WILL_QOS;
			xCred->connectData->will.retained = netconfigMQTT_WILL_RETAIN;

			pvValue = pvGetConfig( eConfigMqttWillTopic, NULL );
			if( pvValue != NULL )
			{
				xCred->connectData->will.topicName.cstring = (char *)pvValue;
				xReturn++;
			}

			pvValue = pvGetConfig( eConfigMqttWillMsg, NULL );
			if( pvValue != NULL )
			{
				xCred->connectData->will.message.cstring = (char *)pvValue;
				xReturn++;
			}
		}
	}
	return xReturn;
}


#if( includeHTTP_DEMO != 0 )
	/***********************************************************************************************************
	 *   function: xMqttHTTPRequestHandler
	 *   Purpose: Handler for all requests incomming for "mqtt". Will output the configuration and recieve new
	 *   		  configuration from WebUI.
	 *   Global Variables used:
	 *   Returns: 	0 - everything is ok
	 *   			-1 - no socket available
	 *   			something else - FreeRTOS_shutdown return value
	 ***********************************************************************************************************/
	static BaseType_t xMqttHTTPRequestHandler( char *pcBuffer, size_t uxBufferLength, QueryParam_t *pxParams, BaseType_t xParamCount )
	{
		BaseType_t xCount = 0;
		QueryParam_t *pxParam;
		xMqttCredentials_t xConnectionConfig;
		MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

		xConnectionConfig.connectData = &connectData;

		/* _CD_ search for each of the credential parameters. If one is found update the config. Do not change the connection!! */
		/* Search for Broker address */
		pxParam = pxFindKeyInQueryParams( "broker", pxParams, xParamCount );
		if( pxParam != NULL ) {
			pvSetConfig( eConfigMqttBroker, strlen( pxParam->pcValue ), (void *) pxParam->pcValue );
		}
		/* Search for Broker Port */
		pxParam = pxFindKeyInQueryParams( "port", pxParams, xParamCount );
		if( pxParam != NULL ) {
			pvSetConfig( eConfigMqttPort, strlen( pxParam->pcValue ), (void *) pxParam->pcValue );
		}
		/* Search for Client ID */
		pxParam = pxFindKeyInQueryParams( "client", pxParams, xParamCount );
		if( pxParam != NULL ) {
			pvSetConfig( eConfigMqttClientID, strlen( pxParam->pcValue ), (void *) pxParam->pcValue );
		}
		/* Search for username */
		pxParam = pxFindKeyInQueryParams( "user", pxParams, xParamCount );
		if( pxParam != NULL ) {
			pvSetConfig( eConfigMqttUser, strlen( pxParam->pcValue ), (void *) pxParam->pcValue );
		}
		/* Search for password */
		pxParam = pxFindKeyInQueryParams( "password", pxParams, xParamCount );
		if( pxParam == NULL ) {
			pxParam = pxFindKeyInQueryParams( "password2", pxParams, xParamCount );
		}
		if( pxParam != NULL ){
			pvSetConfig( eConfigMqttPassWD, strlen( pxParam->pcValue ), (void *) pxParam->pcValue );
		}
		/* Search for will flag */
		pxParam = pxFindKeyInQueryParams( "will", pxParams, xParamCount );
		if( pxParam != NULL ) {
			pvSetConfig( eConfigMqttWill, strlen( pxParam->pcValue ), (void *) pxParam->pcValue );
		}
		/* Search for Will Topic */
		pxParam = pxFindKeyInQueryParams( "willtopic", pxParams, xParamCount );
		if( pxParam != NULL ) {
			pvSetConfig( eConfigMqttWillTopic, strlen( pxParam->pcValue ), (void *) pxParam->pcValue );
		}
		/* Search for Will Message */
		pxParam = pxFindKeyInQueryParams( "willmessage", pxParams, xParamCount );
		if( pxParam != NULL ) {
			pvSetConfig( eConfigMqttWillMsg, strlen( pxParam->pcValue ), (void *) pxParam->pcValue );
		}

		xGetCredentials( &xConnectionConfig );
		xCount += sprintf( pcBuffer, "{\"broker\":"   "\"%s\","
									  "\"port\":"       "%d,"
									  "\"client\":"   "\"%s\","
									  "\"user\":"     "\"%s\","
									  "\"password\":" "\"%s\","
									  "\"will\":"       "%d,"
									  "\"wtopic\":"   "\"%s\","
									  "\"wmessage\":" "\"%s\"}",
									  xConnectionConfig.pcBrokername,
									  xConnectionConfig.port,
									  xConnectionConfig.connectData->clientID.cstring,
									  xConnectionConfig.connectData->username.cstring,
									  xConnectionConfig.connectData->password.cstring,
									  xConnectionConfig.connectData->willFlag,
									  xConnectionConfig.connectData->will.topicName.cstring,
									  xConnectionConfig.connectData->will.message.cstring );
		return xCount;
	}
#endif /* #if( includeHTTP_DEMO != 0 ) */


/***********************************************************************************************************
 *   function: xInitMqtt
 *   Purpose: Initialize Mqtt Connection, will get Socket and create Connection to Broker and start Task
 *   Global Variables used: xClient, xNetwork, xMqttTaskHandle, pcSendBuf, pcRecvBuf, ucInitialized
 *   Returns: 	0 - everything is ok
 *   			something else - error (may contain return value of child function)
 ***********************************************************************************************************/
BaseType_t xInitMqtt()
{
	BaseType_t iStatus = 0;
	xMqttCredentials_t xCred;
	MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

	xCred.connectData = &connectData;
	xGetCredentials( &xCred );

	/* _CD_ Check whether the Client and Task is already created */
	if( ucInitialized == 0 )
	{
		NetworkInit( &xNetwork );
		MQTTClientInit( &xClient, &xNetwork, 2000, pcSendBuf, mqttSEND_BUFFER_SIZE, pcRecvBuf, mqttRECV_BUFFER_SIZE );
		if( ( iStatus = NetworkConnect( &xNetwork , netconfigMQTT_BROKER, netconfigMQTT_PORT )) != 0 ){
			DEBUGOUT("MQTT-Error 0x0010: Unable to Connect to Network. Broker %s on %d\n", netconfigMQTT_BROKER, netconfigMQTT_PORT);
			xEndSocket();
			return iStatus;
		}

		if( ( iStatus = MQTTConnect(&xClient, xCred.connectData)) != 0 ){
			DEBUGOUT("MQTT-Error 0x0020: Unable to Connect to Broker.");
			xEndSocket();
			return iStatus;
		}

		xTaskCreate(vMqttTask,						/* Task function */
					"MqttTask",						/* Task name */
					240,							/* Task stack size */
					NULL,							/* Task call parameters */
					( tskIDLE_PRIORITY + 2 ),		/* Task priority */
					&xMqttTaskHandle);				/* Task handle */
		if( xMqttTaskHandle == NULL ){
			DEBUGOUT("MQTT-Error 0x0030: Could not create Mqtt receiver task\n");
			xEndSocket();
			return -1;
		}

		#if( includeHTTP_DEMO != 0 )
			/* Add HTTP request handler */
			xAddRequestHandler( "mqtt", xMqttHTTPRequestHandler );
		#endif /* ( includeHTTP_DEMO != 0 ) */
		ucInitialized = 1;
	}
	return iStatus;
}

/*
void vMqttTest()
{
	for( int i = 0 ; i < 20 ; i++ )
		vTaskDelay(1000);

	xDeinitMqtt();
	vTaskDelete( NULL );
}
*/

/***********************************************************************************************************
 *   function: xInitMqttTask
 *   Purpose: Call InitMqtt after a timeout
 *   		  NOTE: Use Task so the normal starup of the Network could proceed and is not interfered
 ***********************************************************************************************************/
void vInitMqttTask()
{
	vTaskDelay( 4000 );
	xInitMqtt();
	//xTaskCreate(vMqttTest, "Test", 240, NULL, 1, NULL);
	vTaskDelete( NULL );
}


/********************************************************************************
 *   function: xDeinitMqtt
 *   Purpose: Clean up Mqtt Client, Task and Socket
 *   Global Variables used: xClientHandle, xMqttTaskHandle
 *   Returns: 	3 - every thing done
 *   			between 3 and -1 - either task, socket, client or connection dose not exist
 *   			-1 - nothing was done
 ********************************************************************************/
BaseType_t xDeinitMqtt()
{
	int ret = 0;

	/* first disconnect client */
	if( xClient.isconnected ){
		MQTTDisconnect( &xClient );
		ret++;
	}

	/* Then close TCP socket */
	xEndSocket();

	/* least stop the MQTT-Task, so all other tasks will be done when MqttTask calls Deinit*/
	if( xMqttTaskHandle != NULL ){
		#if( includeHTTP_DEMO != 0 )
			xRemoveRequestHandler( "mqtt" );
		#endif /* #if( includeHTTP_DEMO != 0 ) */
		vTaskDelete( xMqttTaskHandle );
		xMqttTaskHandle = NULL;
		ret++;
	}

	ucInitialized = 0;

	if( ret == 2 )
		return 0;

	return -1;
}
