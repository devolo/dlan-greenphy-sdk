/*
 * MQTTEcho_Test.h
 *
 *  Created on: 10.08.2017
 *      Author: CHristoph.Domnik
 */

#ifndef MQTT_INC_MQTTECHO_TEST_H_
#define MQTT_INC_MQTTECHO_TEST_H_

//define your mqtt credentials
#define DEVICE_ID "0874fad2-7995-4ab7-94e8-0cf32a3ff2ae"
#define MQTT_USER "eb5c7088-51f6-4ae0-b60e-e4951ee73e81"
#define MQTT_PASSWORD "U.igll0coE_N"
#define MQTT_CLIENTID "T61xwiFH2SuC2DuSVHuc+gQ" //can be anything else
#define MQTT_TOPIC "/v1/eb5c7088-51f6-4ae0-b60e-e4951ee73e81/"
#define MQTT_SERVER "mqtt.relayr.io"

void vStartMQTTTasks(uint16_t usTaskStackSize, UBaseType_t uxTaskPriority);
void vLookUpAddress();

#endif /* MQTT_INC_MQTTECHO_TEST_H_ */
