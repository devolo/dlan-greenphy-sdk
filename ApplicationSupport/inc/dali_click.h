/*
 * dali_click.h
 *
 *  Created on: 20.08.2015
 *      Author: Sebastian Sura
 */

#ifndef INC_DALI_CLICK_H_
#define INC_DALI_CLICK_H_

#include "stdint.h"
#include <stdbool.h>

#define TX_PORT 1
#define RX_PORT 2
#define BUTTON_PORT 2

//#if DALIPORT == M1
//#define TX_PIN 26
//#define RX_PIN 3
//#define BUTTON_PIN 2
//#else
//#define TX_PIN 28
//#define RX_PIN 6
//#define BUTTON_PIN 7
//#endif

static volatile uint8_t usbBackwardFrame;
static volatile bool earlyAnswer;
//static volatile capturedFrameType capturedFrame;

int getmasterState();
void setmasterState(int s);
//int getBackwardFrame();
int getDALI_slaves();

#endif /* INC_DALI_CLICK_H_ */
