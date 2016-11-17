/*
 1 tab == 4 spaces!

 */

#include "FreeRTOS.h"
#include "task.h"
#include "clocking.h"
#include "greenPhyModuleApplication.h"
#include "string.h"
#include "uart.h"
#include "queue.h"

#if HTTP_SERVER == ON || COMMAND_LINE_INTERFACE == ON
#include "relay.h"
#endif

#include "bootloaderapp.h"

#include "MQTTClient.h"
#include "uip.h"
#include "jansson.h"
#include "lcd_click.h"

//Queue
xQueueHandle MQTT_Queue = 0;
uint8_t cloudactive;

/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook(void) {
	printToUart("Malloc failed!\r\n");
	for (;;)
		;
}

/*-----------------------------------------------------------*/

void vApplicationTickHook(void) {
	/* Called from every tick interrupt */

	DEBUG_EXECUTE(
			{ static size_t old_mem = 0; size_t mem = xPortGetFreeHeapSize(); if(old_mem != mem) { DEBUG_PRINT(DEBUG_INFO,"application free heap: %d(0x%x)\r\n",mem,mem); old_mem = mem; } });
}

/*-----------------------------------------------------------*/

void vApplicationIdleHook(void) {
	/* Called from idle task */
}

/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook(xTaskHandle pxTask, signed char *pcTaskName) {
	/* This function will get called if a task overflows its stack. */

	(void) pxTask;
	(void) pcTaskName;

	for (;;)
		;
}

/*-----------------------------------------------------------*/

void vConfigureTimerForRunTimeStats(void) {
	const unsigned long TCR_COUNT_RESET = 2, CTCR_CTM_TIMER = 0x00,
			TCR_COUNT_ENABLE = 0x01;

	/* This function configures a timer that is used as the time base when
	 collecting run time statistical information - basically the percentage
	 of CPU time that each task is utilising.  It is called automatically when
	 the scheduler is started (assuming configGENERATE_RUN_TIME_STATS is set
	 to 1). */

	/* Power up and feed the timer. */
	LPC_SC->PCONP |= 0x02UL;
	LPC_SC->PCLKSEL0 = (LPC_SC->PCLKSEL0 & (~(0x3 << 2))) | (0x01 << 2);

	/* Reset Timer 0 */
	LPC_TIM0->TCR = TCR_COUNT_RESET;

	/* Just count up. */
	LPC_TIM0->CTCR = CTCR_CTM_TIMER;

	/* Prescale to a frequency that is good enough to get a decent resolution,
	 but not too fast so as to overflow all the time. */
	LPC_TIM0->PR = ( configCPU_CLOCK_HZ / 10000UL) - 1UL;

	/* Start the counter. */
	LPC_TIM0->TCR = TCR_COUNT_ENABLE;
}

/*-----------------------------------------------------------*/

int main(void) {
	LPC_UseOscillator(LPC_MAIN_OSC);
	LPC_SetPLL0(100000000);

	printInit(UART0);
	DEBUG_INIT(UART0);

	printToUart("\r\n\r\nSTANDALONE ");
	{
		uint32_t reset_reason = LPC_SC->RSID;
		printToUart("RSID:0x%x", reset_reason);
		if (!reset_reason)
			printToUart("->Bootloader");
		if (reset_reason & 0x1)
			printToUart("->Power On");
		if (reset_reason & 0x2)
			printToUart("->Reset");
		if (reset_reason & 0x4)
			printToUart("->Watchdog");
		if (reset_reason & 0x8)
			printToUart("->BrownOut Detection");
		if (reset_reason & 0x10)
			printToUart("->JTAG/restart");
		printToUart("\r\n");
		LPC_SC->RSID = reset_reason;
	}

	printToUart("UART0 %s(%s)\r\n", features, version);

#if HTTP_SERVER == ON || COMMAND_LINE_INTERFACE == ON
	initRelay();
#endif

#if IP_STACK == ON
	void * taskArgument = NULL;
#endif

#if USE_ETHERNET_OVER_SPI == ON
	struct netDeviceInterface * greenPhyDevice = greenPhyInitNetdevice();
#if IP_STACK == ON
	taskArgument = greenPhyDevice;
#endif

#if COMMAND_LINE_INTERFACE == ON
	netdeviceAdd(greenPhyDevice,"GreenPHY");
#endif
#endif

#if USE_ETHERNET == ON
	struct netDeviceInterface * ethernetDevice = ethInitNetdevice();
#if IP_STACK == ON
	taskArgument = ethernetDevice;
#endif
#if COMMAND_LINE_INTERFACE == ON
	netdeviceAdd(ethernetDevice,"ETH");
#endif
#endif

#if IP_STACK == ON
	xTaskCreate( vuIP_Task, ( signed char * ) "uIP", mainBASIC_WEB_STACK_SIZE, taskArgument, mainUIP_TASK_PRIORITY, NULL );
#endif

#if ETHERNET_OVER_SPI_TO_ETHERNET_BRIDGE == ON
	static struct bridgePath path;
	path.name = "ETH->GreenPHY";
	path.bridgeSource = ethernetDevice;
	path.bridgeDestination = greenPhyDevice;
	initBridgeTask(&path);
#endif

	size_t mem = xPortGetFreeHeapSize();
	printToUart("free heap: %d(0x%x)\r\n", mem, mem);

#if COMMAND_LINE_INTERFACE == ON
	cliInit();
#else
	printToUart("OK\r\n");
#endif

	/*Initialize Queue*/
	MQTT_Queue = xQueueCreate(3, 24);

	/*Override json malloc functions*/
	json_set_alloc_funcs(pvPortMalloc, vPortFree);

	vTaskStartScheduler();

	// the application will be only started after the scheduler is ended by the bootloader calling vTaskEndScheduler()

	startApplication();

	// shall never be reached ...

	for (;;)
		;

	return 0;
}
