#include "uv_click.h"
#include "string.h"
#include "FreeRTOS.h"
#include "task.h"
#include "debug.h"
#include "lpc17xx_ssp.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "gpio.h"
#include "stdbool.h"
#include "types.h"
#include "queue.h"

#define sensor_TASK_PRIORITY            ( tskIDLE_PRIORITY + 1 )
#define BUFFER_SIZE 0x2

xTaskHandle xHandle_uv = NULL;
bool uv_isactive = false;

uint16_t Rx_Buf[BUFFER_SIZE];
int mW = 0;
int uvport;
int CS_PIN_NUM = 2;
int RST_PIN_NUM = 26;

int getmW() {
	return mW;
}

void setup_spi() {

    SSP_CFG_Type SSP_ConfigStruct;
	PINSEL_CFG_Type PinCfg;

	if (uvport == 1) {
		CS_PIN_NUM = 2;
		RST_PIN_NUM = 26;

	} else if (uvport == 2) {
		CS_PIN_NUM = 7;
		RST_PIN_NUM = 28;
	}

	/*Set Pins for SPI*/
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	/*SPI_Clock*/
	PinCfg.Funcnum = 2;
	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 7;
	PINSEL_ConfigPin(&PinCfg);
	/*SPI_MISO*/
	PinCfg.Pinnum = 8;
	PINSEL_ConfigPin(&PinCfg);
	/*SPI_SEL*/
	PinCfg.Funcnum = 0;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = CS_PIN_NUM;
	PINSEL_ConfigPin(&PinCfg);
	/*RST_PIN*/
	PinCfg.Portnum = 1;
	PinCfg.Pinnum = RST_PIN_NUM;
	PINSEL_ConfigPin(&PinCfg);

	/*Initialize GPIO Function of PIN2.2*/
	GPIO_SetDir(2, (1 << CS_PIN_NUM), 1);
	GPIO_SetValue(2, (1 << CS_PIN_NUM));

	/*Select UV Sensor*/
	GPIO_SetDir(1, (1 << RST_PIN_NUM), 1);
	GPIO_SetValue(1, (1 << RST_PIN_NUM));

	SSP_ConfigStruct.CPHA = SSP_CPHA_FIRST;
	SSP_ConfigStruct.ClockRate = 1600000;
	SSP_ConfigStructInit_UV(&SSP_ConfigStruct);
	SSP_Init_UV(LPC_SSP1, &SSP_ConfigStruct);
	SSP_Cmd(LPC_SSP1, ENABLE);

}

bool getuv_isactive() {
	return uv_isactive;
}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

unsigned int getADC() {

	SSP_DATA_SETUP_Type xferConfig;
	uint16_t adc = 0;
	uint16_t temp1 = 0;
	float uvIntensity = 0;
	int i;

	for (i = 0; i<16 ; i++) {
	/*Set ADC_SPI to low to start communication*/
	GPIO_ClearValue(2, (1 << CS_PIN_NUM));
	/*Read Data*/
	xferConfig.tx_data = NULL;
	xferConfig.rx_data = Rx_Buf;
	xferConfig.length = BUFFER_SIZE;
	SSP_ReadWrite(LPC_SSP1, &xferConfig, SSP_TRANSFER_POLLING);
	/*Set ADC_SPI to high to end communication*/
	GPIO_SetValue(2, (1 << CS_PIN_NUM));

	/*Convert ADC Values*/
	temp1 = (Rx_Buf[0] & 0x1FFF);
	adc = (temp1 >> 1);
	uvIntensity += adc * 3300 / 4096;
	}
	uvIntensity /= 16;

	uvIntensity = mapfloat(uvIntensity, 1000, 3000, 0.0, 15.0);

	return uvIntensity;
}

static void UV_Sensor_Task(void *pvParameters) {

    extern xQueueHandle MQTT_Queue;
    struct MQTT_Queue_struct uv_struct;
    float mW_last = 0;
	printToUart("UV_Sensor_Task running..\r\n");

	strcpy(uv_struct.meaning, "UV");
    uv_struct.type = JSON_REAL;

    setup_spi();

	for (;;) {

	    taskENTER_CRITICAL();
		uv_struct.f_val = getADC();
		taskEXIT_CRITICAL();
		mW = uv_struct.f_val;
		if (uv_struct.f_val != mW_last) {
		    xQueueSend(MQTT_Queue, &uv_struct, 5000);
		    mW_last = uv_struct.f_val;
		}
		vTaskDelay(1000);
	}
}

void init_uv_sensor(int i) {

	uvport = i;
	uv_isactive = true;

	xTaskCreate(UV_Sensor_Task, (signed char * )"UV_Sensor", 240, NULL,
			sensor_TASK_PRIORITY, &xHandle_uv);
}

void deinit_uv_sensor() {

	uv_isactive = false;
	SSP_Cmd(LPC_SSP1, DISABLE);
	SSP_DeInit(LPC_SSP1);
	vTaskDelete(xHandle_uv);
}
