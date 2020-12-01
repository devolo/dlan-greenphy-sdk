/*
 * mqtt_old.h
 *
 *  Created on: 20.09.2017
 *      Author: CHristoph.Domnik
 */

#ifndef MQTT_OLD_H_
#define MQTT_OLD_H_

#include "MQTTClient.h"
#include "MQTTFreeRTOS.h"

/*********************************************************************************
 *	Defines
 * *******************************************************************************/

#define mqttRECV_BUFFER_SIZE 32 /* Size of MQTT Receive Buffer in Bytes */
#define mqttSEND_BUFFER_SIZE 192 /* Size of MQTT Send Buffer in Bytes */

#define mqttRECEIVE_DELAY 1000 /* Delay of MQTT Task in milliseconds */

#define CLIENT_BLOCKED 1
#define CLIENT_FREE 0

#define netconfigMQTT_BROKER "mqtt.relayr.io"
#define netconfigMQTT_PORT 1883
#define netconfigMQTT_CLIENT "TCdqKfhZeQriWr/7PNmy4mw"
#define netconfigMQTT_USER "09da8a7e-165e-42b8-96af-fecf366cb89b"
#define netconfigMQTT_PASSWORT "wy5t6n.SQBQx"
#define netconfigMQTT_WILL_QOS 2
#define netconfigMQTT_WILL_RETAIN 1

/*********************************************************************************
 *	Structs and typedefs
 * *******************************************************************************/
typedef struct{
	char *pcBrokername;
	BaseType_t port;
	MQTTPacket_connectData *connectData;
}xMqttCredentials_t;

/*********************************************************************************
 *	function prototypes
 * *******************************************************************************/
void vInitMqttTask();
BaseType_t xDeinitMqtt();
BaseType_t xPublishMessage( char *pucMessage, char *pucTopic, unsigned char ucQos, unsigned char ucRetained );
BaseType_t xIsActive();


#endif /* MQTT_OLD_H_ */
