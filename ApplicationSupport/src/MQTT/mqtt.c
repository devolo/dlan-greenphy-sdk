/*
 * mqtt.c
 *
 *  Created on: 25.11.2015
 *      Author: Sebastian Sura
 */

#include "mqtt.h"
#include "string.h"
#include "stdbool.h"
#include "FreeRTOS.h"
#include "task.h"
#include "uip.h"
#include "uipopt.h"
#include "psock.h"
#include "queue.h"
#include "MQTTClient-C/src/MQTTClient.h"
#include "types.h"
#include "lpc17xx_pinsel.h"
#include "jansson.h"

#include "relay_click.h"
#include "lcd_click.h"

#define mqtt_TASK_PRIORITY          ( tskIDLE_PRIORITY + 2 )

#define STATE_CONNECTING 0
#define STATE_CONNECTED_TCP 1
#define STATE_CONNECTED_MQTT 2
#define STATE_SUBSCRIBED 3
#define STATE_CLOSED 4
#define STATE_WAITING 5
#define STATE_POLLED 6
#define STATE_HEARTBEAT 7
#define STATE_CLOSING 8

#define MQTT_HEADER_CON 0x10
#define MQTT_HEADER_PUB 0x30
#define MQTT_HEADER_CONACK 0x20
#define MQTT_HEADER_SUB 0x80
#define MQTT_HEADER_SUBACK 0x90
#define MQTT_HEADER_PINGREQ 0xc0
#define MQTT_HEADER_PINGACK 0xd0

xTaskHandle xHandle_mqtt = NULL;
char mqtt_buffer[150];
int mqtt_buffer_len;
//int subscribe;
//char c;
static struct mqtt_state mqtt;
struct MQTT_Queue_struct mqtt_data;
config_t *gpconfig;
uint8_t USRLEDstate = 0;
uip_ipaddr_t mqtt_addr;
extern xQueueHandle MQTT_Queue;

/********************************************************************//**
 * @brief       Creates the payload for the MQTT Publish Packet using
 *              JSON and writes it to payload
 * @param[in]   Pointer to payload
 * @return      None
 *********************************************************************/
void createPayloadString(char *payload) {

    char *result = NULL;

    json_t *root = json_object();
    json_object_set_new(root, "meaning", json_string(mqtt_data.meaning));
    printToUart("Send MQTT %s of type %d ", mqtt_data.meaning, mqtt_data.type);
    if( mqtt_data.type == JSON_TRUE ) {
        json_object_set_new(root, "value", json_true());
    } else if( mqtt_data.type == JSON_FALSE ) {
        json_object_set_new(root, "value", json_false());
    } else if( mqtt_data.type == JSON_REAL ) {
        // %f, %g in sprintf not implemented
        //json_object_set_new(root, "value", json_real(mqtt_data.f_val));
        json_object_set_new(root, "value", json_integer((int) (mqtt_data.f_val*10)));
    } else if( mqtt_data.type == JSON_INTEGER ) {
        json_object_set_new(root, "value", json_integer(mqtt_data.i_val));
    } else if( mqtt_data.type == JSON_STRING ){
        json_object_set_new(root, "value", json_string(mqtt_data.s_val));
    }
    result = json_dumps(root, 0);
    sprintf(payload, result);
    vPortFree(result);
    json_decref(root);
}

/********************************************************************//**
 * @brief       Creates a MQTT Connect with provided credentials and
 *              writes it to mqtt_buffer
 * @param[in]   None
 * @return      None
 *********************************************************************/
void createMQTTConnect() {
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

    data.clientID.cstring = gpconfig->clientid;
    data.username.cstring = gpconfig->user;
    data.password.cstring = gpconfig->password;
    data.keepAliveInterval = 60;
    data.cleansession = 1;
    data.MQTTVersion = 4;
    mqtt_buffer_len = MQTTSerialize_connect(mqtt_buffer, sizeof(mqtt_buffer), &data);
}

/********************************************************************//**
 * @brief       Creates the payload for the MQTT Pubslish Packet using
 *              JSON and writes it to payload. Retain Flag is set!
 * @param[in]   Pointer to payload
 * @return      None
 *********************************************************************/
void createMQTTPublish() {

    MQTTString topicString = MQTTString_initializer;
    char payload[200];
    int payloadlen;
    topicString.cstring = gpconfig->topic;
    createPayloadString(payload);
    payloadlen = strlen(payload);
    mqtt_buffer_len = MQTTSerialize_publish(mqtt_buffer, sizeof(mqtt_buffer), 0, 0, 1, 0, topicString, payload,payloadlen);
}

/********************************************************************//**
 * @brief       Creates a MQTTSubscribe Packet to subscribe to relayrs
 *              ./cmd topic
 * @param[in]   None
 * @return      None
 *********************************************************************/
void createMQTTSubscribe() {
    MQTTString topicString = MQTTString_initializer;
    char topic[45];
    strcpy(topic, "/v1/");
    strncat(topic, gpconfig->user, 36);
    strcat(topic, "/cmd");
    topic[44] = '\0';
    topicString.cstring = topic;
    mqtt_buffer_len = MQTTSerialize_subscribe(mqtt_buffer, sizeof(mqtt_buffer), 0 ,3 , 1, &topicString, 0);
}

/********************************************************************//**
 * @brief       Tell uIP stack to send a syn to remote host and establish
 *              new connection, IP is read from flash
 *              ./cmd topic
 * @param[in]   None
 * @return      None
 *********************************************************************/
void setup_mqtt(void) {
    //extern config_t gpconfig;
    //uip_ipaddr(&mqtt_addr, gpconfig.IP[0], gpconfig.IP[1],gpconfig.IP[2], gpconfig.IP[3]);
    //uip_ipaddr(&ipaddr, 52,17,197,41);
    //uip_ipaddr(&ipaddr, 54,154,53,255);
    //uip_ipaddr(&mqtt_addr, 54, 77, 125, 59);
    //uip_ipaddr(&mqtt_addr, 192,168,0,12);
    mqtt.conn = uip_connect(&mqtt_addr, HTONS(1883));
    if (mqtt.conn == NULL)
        uip_abort();
}

/********************************************************************//**
 * @brief       Toggles the USR LED on Eval Board
 * @param[in]   None
 * @return      None
 *********************************************************************/
void toggleUSRLED() {
    taskENTER_CRITICAL();
    PINSEL_CFG_Type PinCfg;
    PinCfg.Pinmode = 0;
    PinCfg.Funcnum = 0;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);
    GPIO_SetDir(0, (1 << 6), 1);
    if (USRLEDstate == 0) {
        GPIO_ClearValue(0, (1 << 6));
        USRLEDstate = 1;
    } else {
        GPIO_SetValue(0, (1 << 6));
        USRLEDstate = 0;
    }
    //Reset to Func3 for I2C
    PinCfg.Funcnum = 3;
    PINSEL_ConfigPin(&PinCfg);
    taskEXIT_CRITICAL();
}

/********************************************************************//**
 * @brief       Turns the USR LED on Eval Board on
 * @param[in]   None
 * @return      None
 *********************************************************************/
void USRLED_on() {
    taskENTER_CRITICAL();
    PINSEL_CFG_Type PinCfg;
    PinCfg.Pinmode = 0;
    PinCfg.Funcnum = 0;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);
    GPIO_SetDir(0, (1 << 6), 1);

    GPIO_ClearValue(0, (1 << 6));
    USRLEDstate = 1;

    //Reset to Func3 for I2C
    PinCfg.Funcnum = 3;
    PINSEL_ConfigPin(&PinCfg);
    taskEXIT_CRITICAL();
}

/********************************************************************//**
 * @brief       Turns the USR LED on Eval Board off
 * @param[in]   None
 * @return      None
 *********************************************************************/
void USRLED_off() {
    taskENTER_CRITICAL();
    PINSEL_CFG_Type PinCfg;
    PinCfg.Pinmode = 0;
    PinCfg.Funcnum = 0;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);
    GPIO_SetDir(0, (1 << 6), 1);

    GPIO_SetValue(0, (1 << 6));
    USRLEDstate = 0;

    //Reset to Func3 for I2C
    PinCfg.Funcnum = 3;
    PINSEL_ConfigPin(&PinCfg);
    taskEXIT_CRITICAL();
}

/********************************************************************//**
 * @brief       Looks in the received MQTTPublish Packet for the status of
 *              the LED  and checks if correct deviceID is given
 * @param[in]   Pointer to the payload to look in, Length of payload
 * @return      None
 *********************************************************************/
void checkpayload(unsigned char *payload_in, int payloadlen_in) {
    json_t *root;
    json_error_t error;
    json_t *name;
    json_t *value;
    const char *name_text;
    const char *value_text;

    payload_in[payloadlen_in] = '\0';
    root = json_loads((char *)payload_in, 0, &error); //HardFault
    if(!root) { return; }

    name = json_object_get(root, "name");
    name_text = json_string_value(name);

    if( name_text ) {
        value = json_object_get(root, "value");
        // value_text = json_string_value(value);

        if(!strcmp(name_text, "led")) {
            if( json_is_true(value) )
                USRLED_on();
            if( json_is_true(value) )
                USRLED_off();
        }
        else if (!strcmp(name_text, "relay1")) {
            if( json_is_true(value) )
                relay_on(1);
            if( json_is_false(value) )
                relay_off(1);
        }
        else if (!strcmp(name_text, "relay2")) {
            if( json_is_true(value) )
                relay_on(2);
            if( json_is_false(value) )
                relay_off(2);
        }
        else if (!strcmp(name_text, "DALI")) {
            if(getdali_isactive())
                MQTTtoDALI(value);
        }
        else if (!strcmp(name_text, "LCD_1")) {
            value_text = json_string_value(value);
            LCD_Print(FIRST_LINE, "                    ");
            LCD_Print(FIRST_LINE, value_text);
        }
        else if (!strcmp(name_text, "LCD_2")) {
            value_text = json_string_value(value);
            LCD_Print(SECOND_LINE, "                    ");
            LCD_Print(SECOND_LINE, value_text);
        }
        else if (!strcmp(name_text, "LCD_3")) {
            value_text = json_string_value(value);
            LCD_Print(THIRD_LINE, "                    ");
            LCD_Print(THIRD_LINE, value_text);
        }
        else if (!strcmp(name_text, "LCD_4")) {
            value_text = json_string_value(value);
            LCD_Print(FOURTH_LINE, "                    ");
            LCD_Print(FOURTH_LINE, value_text);
            set_custom_line4();
        }
    }

    json_decref(root);
}

/********************************************************************//**
 * @brief       Checks if incomming packet is a correct MQTTPublish packet
 *              ./cmd topic
 * @param[in]   Pointer to incomming data on current connection
 * @return      None
 *********************************************************************/
void checkMQTTPublish(void *appdata) {
    MQTTString topicString;
    unsigned char* payload_in;
    int payloadlen_in;
    unsigned char dup, retained;
    int qos;
    unsigned short packetid;
    if (MQTTDeserialize_publish(&dup,&qos,&retained,&packetid, &topicString, &payload_in, &payloadlen_in, appdata, uip_len)) {
        if (payloadlen_in > 4)
            checkpayload(payload_in, payloadlen_in);
    }
}

/********************************************************************//**
 * @brief       Checks if valid credentials are given, since relayr
 *              generates random data, we can only check for length
 * @param[in]   None
 * @return      None
 *********************************************************************/
int checkcredentials() {

    if (strlen(gpconfig->user) != 36)
        return 0;
    if (strlen(gpconfig->password) != 12)
        return 0;
    if (strlen(gpconfig->clientid) != 23)
        return 0;
    if (strlen(gpconfig->topic) != 45 )
        return 0;

    return 1;
}

/********************************************************************//**
 * @brief       Creates a MQTTPingreq packet to keep connection alive
 * @param[in]   None
 * @return      None
 *********************************************************************/
void createMQTTPingreq() {
    memset(mqtt_buffer, '\0', sizeof(mqtt_buffer));
    mqtt_buffer_len = MQTTSerialize_pingreq(mqtt_buffer, sizeof(mqtt_buffer));
}

/********************************************************************//**
 * @brief       Checks incoming packet for MQTTHeader Type and returns
 *              the type
 * @param[in]   Pointer to the packet
 * @return      Type of MQTT Header
 *********************************************************************/
int check_header(const char *buf) {

    char c = *buf;

    if (c == MQTT_HEADER_CON)
        return 1;
    if (c == MQTT_HEADER_CONACK)
            return 2;
    if (c == MQTT_HEADER_PUB)
            return 3;
    if (c == MQTT_HEADER_SUB)
            return 8;
    if (c == MQTT_HEADER_SUBACK)
            return 9;
    if (c == MQTT_HEADER_PINGREQ)
        return 12;
    if (c == MQTT_HEADER_PINGACK)
        return 13;
    return 0;
}

/********************************************************************//**
 * @brief       sends and receives MQTT Packets depending on its state using
 *                 Protothreads
 * @param[in]   Pointer to the current connection
 * @return      None
 *********************************************************************/
static PT_THREAD(handle_mqtt_output(struct httpd_state *s)) {

    PSOCK_BEGIN(&s->sin);
    s->ticks = CLOCK_SECOND;

    //check for new Data from MQTT Broker
    if ((s->state == STATE_SUBSCRIBED) && (PSOCK_NEWDATA(&s->sin)) && (check_header(uip_appdata) == 3)) {
        checkMQTTPublish(uip_appdata);
        memset(uip_appdata, '\0', sizeof(uip_appdata));
    }

    //close connection when MQTT Task is deleted
    if (mqtt.state == STATE_CLOSING) {
        mqtt.state = STATE_CLOSED;
        s->state = STATE_CLOSED;
        PSOCK_CLOSE_EXIT(&s->sin);
    }

    //CONNECT
    if (s->state == STATE_CONNECTED_TCP) {
        createMQTTConnect();
        PSOCK_SEND(&s->sin,mqtt_buffer,mqtt_buffer_len);
        PSOCK_READTO(&s->sin, 0x20);
        if(s->inputbuf[0] != 0x20) {
            PSOCK_CLOSE_EXIT(&s->sin);
        }
        s->state = STATE_CONNECTED_MQTT;
        mqtt.state = STATE_CONNECTED_MQTT;
    }

    //SUBSCRIBE
    if (s->state == STATE_CONNECTED_MQTT) {
            createMQTTSubscribe();
            PSOCK_SEND(&s->sin,mqtt_buffer,mqtt_buffer_len);
            PSOCK_READTO(&s->sin, 0x90);
            s->state = STATE_SUBSCRIBED;
            mqtt.state = STATE_SUBSCRIBED;
    }

    //PUBLISH
    if ((s->state == STATE_SUBSCRIBED || s->state == STATE_CONNECTED_MQTT) && mqtt.state == STATE_POLLED) {
        createMQTTPublish();
        PSOCK_SEND(&s->sin,mqtt_buffer,mqtt_buffer_len);
        s->timer = 0;
        s->state = STATE_SUBSCRIBED;
        mqtt.state = STATE_SUBSCRIBED;
    }

    //PINGREQ
    if ((s->state == STATE_SUBSCRIBED || s->state == STATE_CONNECTED_MQTT) && mqtt.state == STATE_HEARTBEAT) {
        createMQTTPingreq();
        PSOCK_SEND(&s->sin,mqtt_buffer,mqtt_buffer_len);
        PSOCK_READTO(&s->sin, 0xd0);
        s->timer = 0;
        s->state = STATE_SUBSCRIBED;
        mqtt.state = STATE_SUBSCRIBED;
        }

    PSOCK_END(&s->sin);
}


/********************************************************************//**
 * @brief       Controls MQTT Connection, is called everytime we send
 *              or receive a Packet to/on Port 1883
 * @param[in]   None
 * @return      None
 *********************************************************************/
void mqtt_appcall() {

struct httpd_state *s = (struct httpd_state *)&(uip_conn->appstate);

if(uip_closed() || uip_aborted() || uip_timedout()) {
    s->state=STATE_CLOSED;
    mqtt.state=STATE_CLOSED;
  } else if(uip_connected()) {
    PSOCK_INIT(&s->sin, s->inputbuf, sizeof(s->inputbuf) - 1);
    s->state = STATE_CONNECTED_TCP;
    mqtt.state = STATE_CONNECTED_TCP;
    s->timer = 0;
  } else if (s != NULL) {
      if (uip_poll()) {
          ++s->timer;
          if (s->timer >= 75) {
              uip_close();
          }
      } else {
          s->timer = 0;
      }
      handle_mqtt_output(s);
  } else {
      uip_abort();
  }
}

/********************************************************************//**
 * @brief       MQTT_Task waits 30 seconds for new sensor and receives it
 *              using a Queue. If data is available it changes
 *              the status of the connection to tell it to send data.
 *              If connection is closed, it reconnects automatically
 * @param[in]   None
 * @return      None
 *********************************************************************/
void MQTT_Task(void *pvParameters) {
//    size_t heap;

    connect:
    mqtt.state = STATE_CONNECTING;
    do {
        setup_mqtt();
        vTaskDelay(6000);
        if (mqtt.state == STATE_CONNECTING)
            vTaskDelay(30000);
    } while (mqtt.state == STATE_CONNECTING);

       for (;;) {
//           heap = xPortGetFreeHeapSize();
           if (xQueueReceive(MQTT_Queue, &mqtt_data, 30000)) {
               if (checkcredentials()) {
                   if (mqtt.state == STATE_CLOSED) {
                       goto connect;
                   } else if (mqtt.state == STATE_SUBSCRIBED) {
                       mqtt.state = STATE_POLLED;
                       uip_poll_conn(mqtt.conn);
                       //vTaskDelay(5000);
                   }
               }
           } else { //send heartbeat
               if (checkcredentials()) {
                   if (mqtt.state == STATE_CLOSED) {
                       goto connect;
                   } else if (mqtt.state == STATE_SUBSCRIBED) {
                       mqtt.state = STATE_HEARTBEAT;
                       uip_poll_conn(mqtt.conn);
                   } else {
                       mqtt.state = STATE_HEARTBEAT;
                   }
               }
          }
       }
   }

/********************************************************************//**
 * @brief       Start MQTT Task
 * @param[in]   None
 * @return      None
 *********************************************************************/
void init_mqtt() {

    gpconfig->active = 1;
    xTaskCreate(MQTT_Task, (signed char *) "MQTT", 500, NULL, mainUIP_TASK_PRIORITY, &xHandle_mqtt);
}

/********************************************************************//**
 * @brief       Stop MQTT Task
 * @param[in]   None
 * @return      None
 *********************************************************************/
void deinit_mqtt() {
    gpconfig->active = 0;
    mqtt.state = STATE_CLOSING;
    uip_poll_conn(mqtt.conn);
    if (xHandle_mqtt != NULL) {
        vTaskDelete(xHandle_mqtt);
    }
}

/********************************************************************//**
 * @brief       Activate/deactivate MQTT Task
 * @param[in]   Pointer to Input String from webserver task
 * @return      None
 *********************************************************************/
void mqtt_activate(char *c) {

    if (strstr(c, "?activate_mqtt=yes")) {
        if (!gpconfig->active) {
            init_mqtt();
            writeflash();
        }

    } else if(strstr(c, "?activate_mqtt=no")) {
        if (gpconfig->active) {
            deinit_mqtt();
            writeflash();
        }
    }
}
