/*
 * relay_click.c
 *
 *  Created on: 17.08.2015
 *      Author: Sebastian Sura
 */

#include "FreeRTOS.h"
#include "task.h"
#include "debug.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "relay_click.h"
#include "stdbool.h"
#include "queue.h"
#include "types.h"
#include "string.h"

#define CS_PORT_NUM 2

bool relay_isactive = false;
int relayport = 1;
int Relay1PIN, Relay2PIN;
int oldstate1 = 0;
int oldstate2 = 0;

extern xQueueHandle MQTT_Queue;
static struct MQTT_Queue_struct relay1_struct, relay2_struct;

void setup_relay() {

	if (relayport == 1) {
		Relay1PIN = 4;
		Relay2PIN = 2;
	} else if (relayport == 2) {
		Relay1PIN = 5;
		Relay2PIN = 7;
	}
	PINSEL_CFG_Type PinCfg;
	/*Set Pins for Relay*/
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	/*P2.4 Relay1*/
	PinCfg.Funcnum = 0;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = Relay1PIN;
	PINSEL_ConfigPin(&PinCfg);
	/*P2.2 Relay2*/
	PinCfg.Pinnum = Relay2PIN;
	PINSEL_ConfigPin(&PinCfg);
	/*Initialize GPIO Function of PINs*/
	GPIO_SetDir(2, (1 << Relay1PIN), 1);
	GPIO_SetDir(2, (1 << Relay2PIN), 1);
}

void toggle_relay(int relay) {

	/*Relay1*/
	switch (relay) {
	case 1:
		/*IF Relay1 is on, shut down*/
		if (oldstate1 == ON) {
			GPIO_ClearValue(2, (1 << Relay1PIN));
			oldstate1 = OFF;
		} else {
			/*if relay1 is down, set up*/
			if (oldstate1 == OFF) {
				GPIO_SetValue(2, (1 << Relay1PIN));
				oldstate1 = ON;
			}
		}
		break;
	case 2:
		/*Relay2*/
		/*if relay2 is on, shut down*/
		if (oldstate2 == ON) {
			GPIO_ClearValue(2, (1 << Relay2PIN));
			oldstate2 = OFF;
		} else {
			if (oldstate2 == OFF) {
				GPIO_SetValue(2, (1 << Relay2PIN));
				oldstate2 = ON;
			}
		}
		break;
	}
}

void relay_on(int relay) {
	switch (relay) {
	case 1:
		GPIO_SetValue(2, (1 << Relay1PIN));
		oldstate1 = ON;
		relay1_struct.type = JSON_TRUE;
		xQueueSend(MQTT_Queue, &relay1_struct, 5000);
		break;
	case 2:
		GPIO_SetValue(2, (1 << Relay2PIN));
		oldstate2 = ON;
		relay2_struct.type = JSON_TRUE;
		xQueueSend(MQTT_Queue, &relay2_struct, 5000);
		break;
	}
}

void relay_off(int relay) {
	switch (relay) {
	case 1:
		GPIO_ClearValue(2, (1 << Relay1PIN));
		oldstate1 = OFF;
		relay1_struct.type = JSON_FALSE;
		xQueueSend(MQTT_Queue, &relay1_struct, 10000);
		break;
	case 2:
		GPIO_ClearValue(2, (1 << Relay2PIN));
		oldstate2 = OFF;
		relay2_struct.type = JSON_FALSE;
		xQueueSend(MQTT_Queue, &relay2_struct, 10000);
		break;
	}
}

bool getrelay_isactive() {
	return relay_isactive;
}

void init_relay(int i) {

	relayport = i;
    setup_relay();
    strcpy(relay1_struct.meaning, "relay1");
    strcpy(relay2_struct.meaning, "relay2");
    relay_on(Relay1PIN);
    relay_on(Relay2PIN);
	relay_isactive = true;
}

void deinit_relay() {
	relay_isactive = false;
}
