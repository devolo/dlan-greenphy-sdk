/*
 * motion_click.c
 *
 *  Created on: 04.02.2016
 *      Author: Sebastian Sura
 */

#include "FreeRTOS.h"
#include "task.h"
#include "debug.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "stdbool.h"
#include "queue.h"
#include "types.h"

bool motion_isactive = false;
int relayport = 1;

extern xQueueHandle MQTT_Queue;
static struct MQTT_Queue_struct motion_struct;

void EINT3_IRQHandler(void) {
	if ((LPC_GPIOINT->IO2IntStatR >> 3) & 0x1) {
		LPC_GPIOINT->IO2IntClr = 1 << 3;
	}
	if ((LPC_GPIOINT->IO2IntStatR >> 6) & 0x1) {
		LPC_GPIOINT->IO2IntClr = 1 << 6;
	}
	xQueueSendFromISR(MQTT_Queue, &motion_struct, 0);
}

void setup_motion(int i) {

	PINSEL_CFG_Type PinCfg;
	/*Set Pins for Relay*/
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	/*Motion Enable*/
	PinCfg.Funcnum = 0;
	PinCfg.Portnum = 1;
	if (i == 1)
		PinCfg.Pinnum = 26;
	if (i == 2)
		PinCfg.Pinnum = 28;
	PINSEL_ConfigPin(&PinCfg);
	/*Motion Detect*/
	PinCfg.Portnum = 2;
	if (i == 1)
		PinCfg.Pinnum = 3;
	if (i == 2)
		PinCfg.Pinnum = 6;
	PINSEL_ConfigPin(&PinCfg);
	/*Initialize GPIO Function of PINs*/
	if (i == 1)
		GPIO_SetDir(1, (1 << 26), 1);
	if (i == 2)
		GPIO_SetDir(1, (1 << 28), 1);
	if (i == 1) {
		GPIO_SetDir(2, (1 << 3), 0);
		LPC_GPIOINT->IO2IntEnR |= 1 << 3;
	}
	if (i == 2) {
		GPIO_SetDir(2, (1 << 6), 0);
		LPC_GPIOINT->IO2IntEnR |= 1 << 6;
	}
	NVIC_EnableIRQ(EINT3_IRQn);
}

void init_motion(int i) {

	setup_motion(i);
	strcpy(motion_struct.meaning, "motion");
	motion_struct.type = JSON_TRUE;
	motion_isactive = true;
}

void deinit_relay() {
	motion_isactive = false;
	NVIC_DisableIRQ(EINT3_IRQn);
}
