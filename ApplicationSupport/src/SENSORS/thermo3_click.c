/*
 * Sensors.c
 *
 *  Created on: 12.08.2015
 *      Author: Sebastian Sura
 */

#include "hdc1000_click.h"
#include "stdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "debug.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "stdbool.h"
#include "queue.h"
#include "types.h"
#include "math.h"
#include "string.h"
#include "lcd_click.h"

#define sensor_TASK_PRIORITY            ( tskIDLE_PRIORITY + 1 )
__IO FlagStatus complete;

xTaskHandle xHandle_thermo3 = NULL;
bool thermo3_isactive = false;
#define THERMO3_BUFFER_SIZE	0x4 //16
uint8_t Master_Buf[THERMO3_BUFFER_SIZE];
float thermo3temperature = 0;
int thermo3port;
int thermo3i2cint = 3;


static void I2C1_IRQHandler(void) {
	I2C_MasterHandler(LPC_I2C1);
	if (I2C_MasterTransferComplete(LPC_I2C1)) {
		complete = SET;
	}
}

float getThermo3Temperature() {
	return thermo3temperature;
}

bool getthermo3_isactive() {
	return thermo3_isactive;
}

static void setup_i2c() {

	if (thermo3port == 1) {
		thermo3i2cint = 3;
	} else if (thermo3port == 2) {
		thermo3i2cint = 6;

	}

	PINSEL_CFG_Type PinCfg;

	//NVIC_DisableIRQ(I2C1_IRQn);
	//NVIC_SetPriority(I2C1_IRQn, 9);

	/*Set Pins for I2C*/
	/*I2C SDA*/
	PinCfg.Funcnum = 3;
	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	/*I2C SCL*/
	PinCfg.Pinnum = 1;
	PINSEL_ConfigPin(&PinCfg);
	/*I2C INT*/
	PinCfg.Funcnum = 0;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = thermo3i2cint;
	PINSEL_ConfigPin(&PinCfg);
	/*Initialize I2C on LPC1758*/
	I2C_Init(LPC_I2C1, 400000);
	I2C_Cmd(LPC_I2C1, I2C_MASTER_MODE, ENABLE);
	//complete = RESET;

}

static void configure_thermo3() {

	Master_Buf[0] = 0x02;
	Master_Buf[1] = 0x10;
	Master_Buf[2] = 0x00;
	I2C_M_SETUP_Type transferMCfg;
	transferMCfg.sl_addr7bit = THERMO3_I2C_ADDR;
	transferMCfg.tx_data = NULL;
	transferMCfg.tx_length = 0;
	transferMCfg.rx_data = Master_Buf;
	transferMCfg.rx_length = 3;
	transferMCfg.retransmissions_max = 3;
	I2C_MasterTransferData(LPC_I2C1, &transferMCfg, I2C_TRANSFER_POLLING);
	busyWaitMs(20);
}

static void send_sensor(int type) {

	I2C_M_SETUP_Type transferMCfg;

	switch (type) {
	case TEMP:
		Master_Buf[0] = TEMP_REG;
		break;
	case HUM:
		Master_Buf[0] = HUM_REG;
		break;
	default:
		Master_Buf[0] = TEMP_REG;
		/*no break*/
	}

	Master_Buf[1] = 0x00;

	//complete = RESET;
	transferMCfg.sl_addr7bit = THERMO3_I2C_ADDR;
	transferMCfg.tx_data = Master_Buf;
	transferMCfg.tx_length = 1;
	transferMCfg.rx_data = NULL;
	transferMCfg.rx_length = 0;
	transferMCfg.retransmissions_max = 3;
	/*Send Data*/
	I2C_MasterTransferData(LPC_I2C1, &transferMCfg, I2C_TRANSFER_POLLING);
	//while (complete == RESET);
}

static float convert_data(int type) {

	unsigned int temp_value;
	float temperature, humidity;
    int TemperatureSum;

	if (type == TEMP) {

        TemperatureSum = ((Master_Buf[0] << 8) | Master_Buf[1]) >> 4;  // Justify temperature values

        if(TemperatureSum & (1 << 11))                             // Test negative bit
          TemperatureSum |= 0xF800;                                // Set bits 11 to 15 to logic 1 to get this reading into real two complement

        temperature = (float)TemperatureSum * 0.0625;              // Multiply temperature value with 0.0625 (value per bit)

        thermo3temperature = temperature;

		return temperature;

	} else {
		return -1;
	}

}

static void read_sensor() {

	I2C_M_SETUP_Type transferMCfg;

	Master_Buf[0] = TEMP_REG;
	transferMCfg.sl_addr7bit = THERMO3_I2C_ADDR;
	transferMCfg.tx_data = Master_Buf;
	transferMCfg.tx_length = 1;
	transferMCfg.rx_data = NULL;
	transferMCfg.rx_length = 0;
	transferMCfg.retransmissions_max = 3;
	/*Send Data*/
	I2C_MasterTransferData(LPC_I2C1, &transferMCfg, I2C_TRANSFER_POLLING);
	/*Wait for conversion*/
	busyWaitMs(20);

	/*Read Data*/
	transferMCfg.sl_addr7bit = THERMO3_I2C_ADDR;
	transferMCfg.tx_data = NULL;
	transferMCfg.tx_length = 0;
	transferMCfg.rx_data = Master_Buf;
	transferMCfg.rx_length = 2;
	transferMCfg.retransmissions_max = 3;
	I2C_MasterTransferData(LPC_I2C1, &transferMCfg, I2C_TRANSFER_POLLING);

}


static void printdata(int type, float value) {

	if (type == TEMP) {
		/*convert to in because UART doesnt print float values!*/
		value *= 10;
		printToUart("Temperature is: %i °C\r\n", (int) value);
		//printf("Temperature is: %d °C\r\n", (double)value);
	} else {
		printToUart("Unknown value\r\n");
	}
}

static void Temp_Sensor_Task(void *pvParameters) {

    extern xQueueHandle MQTT_Queue;
    extern config_t *gpconfig;
    struct MQTT_Queue_struct temperature_struct;
    float temperature_last = 0;
    float temp;
    printToUart("Temp_Sensor_Task running...\r\n");

    strcpy(temperature_struct.meaning, "temperature");
    temperature_struct.type = JSON_REAL;

	setup_i2c();
	configure_thermo3();
	//wait for mqtt_Task to start
	vTaskDelay(5000);

	for (;;) {
	    taskENTER_CRITICAL();
		read_sensor();
		taskEXIT_CRITICAL();

		temp = (int)((convert_data(TEMP)-4) *10); // 4 arbitrary offset
		temp /= 10;
		temperature_struct.f_val = temp;
//		if (temperature_struct.f_val != temperature_last) {
		    if (gpconfig->active) {
		        xQueueSend(MQTT_Queue, &temperature_struct, 5000);
		    }
//		    temperature_last = temperature_struct.f_val;
//		}

		if( !get_custom_line4() ) {
            char buffer[100];
            int x1 = temp;
            int x2 = (temp - x1) * 10;
            sprintf(buffer, "Temperature %d.%d oC", x1, x2);
            LCD_Print(FOURTH_LINE, buffer);
		}

		vTaskDelay(10000);
	}
}

void init_thermo3_sensor(int i) {

	thermo3_isactive = true;
	thermo3port = i;

	xTaskCreate(Temp_Sensor_Task, (signed char * )"THERMO3", 240, NULL,
			sensor_TASK_PRIORITY, &xHandle_thermo3);
}

void deinit_thermo3_sensor() {

	thermo3_isactive = false;
	NVIC_DisableIRQ(I2C1_IRQn);
	I2C_Cmd(LPC_I2C1, I2C_MASTER_MODE, DISABLE);
	I2C_DeInit(LPC_I2C1);
	vTaskDelete(xHandle_thermo3);

}

