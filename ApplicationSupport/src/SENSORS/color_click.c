/*
 * color_click.c
 *
 *  Created on: 18.08.2015
 *      Author: Sebastian Sura
 */

#include "color_click.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "busyWait.h"
#include "string.h"
#include "debug.h"
#include "stdbool.h"

#define sensor_TASK_PRIORITY            ( tskIDLE_PRIORITY + 1 )
#define BUFFER_SIZE	0x3 //16

// Find maximal floating point value
#define MAX_FLOAT(a, b) (((a) > (b)) ? (a) : (b))

// Find minimal floating point value
#define MIN_FLOAT(a, b) (((a) < (b)) ? (a) : (b))

// Color flags
#define PURPLE_FLAG 1
#define BLUE_FLAG   2
#define CYAN_FLAG   3
#define GREEN_FLAG  4
#define PINK_FLAG   5
#define RED_FLAG    6
#define ORANGE_FLAG 7
#define YELLOW_FLAG 8

xTaskHandle xHandle_color = NULL;
bool color_isactive = false;
int colorport = 1;
int LED_R_PIN = 31;
int LED_R_PORT = 1;
int LED_G_PIN = 2;
int LED_B_PIN = 4;
int COLOR_INT_PIN = 3;
I2C_M_SETUP_Type transferMCfg;
uint8_t Master_Buf[BUFFER_SIZE];

void setup_color() {

	if (colorport == 1) {
		LED_R_PIN = 31;
		LED_R_PORT = 1;
		LED_G_PIN = 2;
		LED_B_PIN = 4;
		COLOR_INT_PIN = 3;

	} else if (colorport == 2) {
		LED_R_PIN = 25;
		LED_R_PORT = 0;
		LED_G_PIN = 7;
		LED_B_PIN = 5;
		COLOR_INT_PIN = 6;
	}

	PINSEL_CFG_Type PinCfg;
	/*Set Pins for I2C*/
	PinCfg.Funcnum = 3;
	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 1;
	PINSEL_ConfigPin(&PinCfg);
	/*Set LED PINS*/
	PinCfg.Funcnum = 0;
	PinCfg.Portnum = LED_R_PORT;
	PinCfg.Pinnum = LED_R_PIN;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = LED_G_PIN;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = LED_B_PIN;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = COLOR_INT_PIN;
	PINSEL_ConfigPin(&PinCfg);

	I2C_Init(LPC_I2C1, 400000);
	I2C_Cmd(LPC_I2C1, I2C_MASTER_MODE, ENABLE);
	busyWaitMs(100);

	/*Set LED PINS as GPIO Output*/
	GPIO_SetDir(LED_R_PORT, (1 << LED_R_PIN), 1);
	GPIO_SetDir(2, (1 << LED_G_PIN), 1);
	GPIO_SetDir(2, (1 << LED_B_PIN), 1);
}

void i2c_write(int length) {
	transferMCfg.sl_addr7bit = COLOR_I2C_ADDR;
	transferMCfg.tx_data = Master_Buf;
	transferMCfg.tx_length = length;
	transferMCfg.rx_data = NULL;
	transferMCfg.rx_length = 0;
	transferMCfg.retransmissions_max = 2;
	I2C_MasterTransferData(LPC_I2C1, &transferMCfg, I2C_TRANSFER_POLLING);
}

uint8_t i2c_read(int length) {

	//uint8_t value = 0;
	transferMCfg.sl_addr7bit = COLOR_I2C_ADDR;
	transferMCfg.tx_data = NULL;
	transferMCfg.tx_length = 0;
	transferMCfg.rx_data = Master_Buf;
	transferMCfg.rx_length = length;
	transferMCfg.retransmissions_max = 2;
	I2C_MasterTransferData(LPC_I2C1, &transferMCfg, I2C_TRANSFER_POLLING);
	return Master_Buf[0];

}

void ledOn() {
	GPIO_SetValue(LED_R_PORT, (1 << LED_R_PIN));
	GPIO_SetValue(2, (1 << LED_G_PIN));
	GPIO_SetValue(2, (1 << LED_B_PIN));
}

void ledOff() {
	GPIO_ClearValue(LED_R_PORT, (1 << LED_R_PIN));
	GPIO_ClearValue(2, (1 << LED_G_PIN));
	GPIO_ClearValue(2, (1 << LED_B_PIN));
}

float RGB_To_HSL(float red, float green, float blue) {
	float volatile fmax, fmin, hue, saturation, luminance;

	fmax = MAX_FLOAT(MAX_FLOAT(red, green), blue);
	fmin = MIN_FLOAT(MIN_FLOAT(red, green), blue);

	luminance = fmax;
	if (fmax > 0)
		saturation = (fmax - fmin) / fmax;
	else
		saturation = 0;

	if (saturation == 0)
		hue = 0;
	else {
		if (fmax == red)
			hue = (green - blue) / (fmax - fmin);
		else if (fmax == green)
			hue = 2 + (blue - red) / (fmax - fmin);
		else
			hue = 4 + (red - green) / (fmax - fmin);
		hue = hue / 6;

		if (hue < 0)
			hue += 1;
	}
	return hue;
}

void convertcolor(float color_value) {

	char color_detected = 0, color_flag = 0;

	if ((color_value >= 0.992) && (color_value <= 0.999)) {
		color_detected = 1;
		if (color_flag != ORANGE_FLAG) {
			color_flag = ORANGE_FLAG;
			printToUart("Orange\r\n");
		}
	}

	// Red color range
	else if ((color_value >= 0.9750) && (color_value <= 0.9919)) {
		color_detected = 1;
		if (color_flag != RED_FLAG) {
			color_flag = RED_FLAG;
			printToUart("Red\r\n");
		}
	}

	// Pink color range
	else if ((color_value >= 0.920) && (color_value <= 0.9749)) {
		color_detected = 1;
		if (color_flag != PINK_FLAG) {
			color_flag = PINK_FLAG;
			printToUart("Pink\r\n");
		}
	}

	// Purple color range
	else if ((color_value >= 0.6201) && (color_value <= 0.919)) {
		color_detected = 1;
		if (color_flag != PURPLE_FLAG) {
			color_flag = PURPLE_FLAG;
			printToUart("Purple\r\n");
		}
	}

	// Blue color range
	else if ((color_value >= 0.521) && (color_value <= 0.6200)) {
		color_detected = 1;
		if (color_flag != BLUE_FLAG) {
			color_flag = BLUE_FLAG;
			printToUart("Blue\r\n");
		}
	}

	// Cyan color range
	else if ((color_value >= 0.470) && (color_value < 0.520)) {
		color_detected = 1;
		if (color_flag != CYAN_FLAG) {
			color_flag = CYAN_FLAG;
			printToUart("Cyan\r\n");
		}
	}

	// Green color range
	else if ((color_value >= 0.210) && (color_value <= 0.469)) {
		color_detected = 1;
		if (color_flag != GREEN_FLAG) {
			color_flag = GREEN_FLAG;
			printToUart("Green\r\n");
		}
	}

	// Yellow color range
	else if ((color_value >= 0.0650) && (color_value <= 0.1800)) {
		color_detected = 1;
		if (color_flag != YELLOW_FLAG) {
			color_flag = YELLOW_FLAG;
			printToUart("Yellow\r\n");
		}
	}

	// Color not in range
	else {
		if (color_detected == 0) {
			color_flag = 0;
			busyWaitMs(500);
			printToUart("Color not in range.\r\n");
		} else {
			color_detected = 0;
		}
	}

}

void Color_Init() {
	Master_Buf[0] = 0x80;
	Master_Buf[1] = 0x1B;
	i2c_write(2);
}

void setGAIN2() {
	Master_Buf[0] = 0x8F;
	Master_Buf[1] = 0x10;
	i2c_write(2);
}

void setAcquisition() {
	Master_Buf[0] = 0x81;
	Master_Buf[1] = 0x00;
	i2c_write(2);
}

bool rgbcvalid() {
	Master_Buf[0] =0x93;
	i2c_write(1);
	i2c_read(1);
	if ((Master_Buf[0] & 0x01) == 1)
		return true;
	return false;
}

uint8_t Color_Read(uint8_t address) {

	Master_Buf[0] = address;
	i2c_write(1);
	return i2c_read(2);

}

uint8_t getDeviceID2() {

	Master_Buf[0] = 0x92;
	i2c_write(1);
	return i2c_read(1);
}

unsigned int Color_Read_value(char reg) {
	unsigned short low_byte;
	unsigned int Out_color;

	switch (reg) {
	case 'C':
		low_byte = Color_Read(_CDATA);
		Out_color = Color_Read(_CDATAH);
		Out_color = (Out_color << 8);
		Out_color = (Out_color | low_byte);
		return Out_color;
		break;

	case 'R':
		low_byte = Color_Read(_RDATA);
		Out_color = Color_Read(_RDATAH);
		Out_color = (Out_color << 8);
		Out_color = (Out_color | low_byte);
		return Out_color;
		break;

	case 'G':
		low_byte = Color_Read(_GDATA);
		Out_color = Color_Read(_GDATAH);
		Out_color = (Out_color << 8);
		Out_color = (Out_color | low_byte);
		return Out_color;
		break;

	case 'B':
		low_byte = Color_Read(_BDATA);
		Out_color = Color_Read(_BDATAH);
		Out_color = (Out_color << 8);
		Out_color = (Out_color | low_byte);
		return Out_color;
		break;

	default:
		return 0;
	}
}

static void Color_Sensor_Task(void *pvParameters) {

	uint8_t deviceID = 0;
	unsigned int Clear, Red, Green, Blue;
	float color_value, color_value_sum;
	float Red_Ratio, Green_Ratio, Blue_Ratio;
	printToUart("Color_Sensor_Task running..\r\n");

	setup_color();

	Color_Init();
	setGAIN2();
	setAcquisition();
	ledOn();

	for (;;) {
		color_value_sum = 0;

		deviceID = getDeviceID2();

		//for (i = 0; i < 16; i++) {
		if(rgbcvalid()) {
			// Read Clear, Red, Green and Blue channel register values
			Clear = Color_Read_value('C');
			Red = Color_Read_value('R');
			Green = Color_Read_value('G');
			Blue = Color_Read_value('B');

			// Divide Red, Green and Blue values with Clear value
			Red_Ratio = ((float) Red / (float) Clear);
			Green_Ratio = ((float) Green / (float) Clear);
			Blue_Ratio = ((float) Blue / (float) Clear);

			// Convert RGB values to HSL values
			color_value = RGB_To_HSL(Red_Ratio, Green_Ratio, Blue_Ratio);

			// Sum the color values
			color_value_sum = color_value_sum + color_value;
		}

		color_value = color_value_sum / 16.0;
		convertcolor(color_value);
		vTaskDelay(700);
	}
}

void init_color_sensor(int i) {

	color_isactive = true;
	colorport = i;

	xTaskCreate(Color_Sensor_Task, (signed char * )"Color_Sensor", 240, NULL,
			sensor_TASK_PRIORITY, &xHandle_color);
}

void deinit_color_sensor() {

	ledOff();
	color_isactive = false;
	I2C_Cmd(LPC_I2C1, I2C_MASTER_MODE, DISABLE);
	I2C_DeInit(LPC_I2C1);
	vTaskDelete(xHandle_color);
}
