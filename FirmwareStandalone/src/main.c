/*
 1 tab == 4 spaces!

 */

/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include "task.h"


#include "FreeRTOS_IP.h"

#include "clocking.h"
#include "greenPhyModuleApplication.h"
#include "string.h"
#include "uart.h"
#include "queue.h"
#include "netConfig.h"

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

/* The default IP and MAC address used by the demo.  The address configuration
defined here will be used if ipconfigUSE_DHCP is 0, or if ipconfigUSE_DHCP is
1 but a DHCP server could not be contacted.  See the online documentation for
more information. */
static const uint8_t ucIPAddress[ 4 ] = { configIP_ADDR0, configIP_ADDR1, configIP_ADDR2, configIP_ADDR3 };
static const uint8_t ucNetMask[ 4 ] = { configNET_MASK0, configNET_MASK1, configNET_MASK2, configNET_MASK3 };
static const uint8_t ucGatewayAddress[ 4 ] = { configGATEWAY_ADDR0, configGATEWAY_ADDR1, configGATEWAY_ADDR2, configGATEWAY_ADDR3 };
static const uint8_t ucDNSServerAddress[ 4 ] = { configDNS_SERVER_ADDR0, configDNS_SERVER_ADDR1, configDNS_SERVER_ADDR2, configDNS_SERVER_ADDR3 };

/* Set the following constant to pdTRUE to log using the method indicated by the
name of the constant, or pdFALSE to not log using the method indicated by the
name of the constant.  Options include to standard out (xLogToStdout), to a disk
file (xLogToFile), and to a UDP port (xLogToUDP).  If xLogToUDP is set to pdTRUE
then UDP messages are sent to the IP address configured as the echo server
address (see the configECHO_SERVER_ADDR0 definitions in FreeRTOSConfig.h) and
the port number set by configPRINT_PORT in FreeRTOSConfig.h. */
const BaseType_t xLogToStdout = pdTRUE, xLogToFile = pdFALSE, xLogToUDP = pdFALSE;

/* Default MAC address configuration.  The demo creates a virtual network
connection that uses this MAC address by accessing the raw Ethernet data
to and from a real network connection on the host PC.  See the
configNETWORK_INTERFACE_TO_USE definition for information on how to configure
the real network connection to use. */
const uint8_t ucMACAddress[ 6 ] = { configMAC_ADDR0, configMAC_ADDR1, configMAC_ADDR2, configMAC_ADDR3, configMAC_ADDR4, configMAC_ADDR5 };

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

    FreeRTOS_IPInit( ucIPAddress, ucNetMask, ucGatewayAddress, ucDNSServerAddress, ucMACAddress );

//#if HTTP_SERVER == ON || COMMAND_LINE_INTERFACE == ON
//	initRelay();
//#endif
//
//#if IP_STACK == ON
//	void * taskArgument = NULL;
//#endif
//
//#if USE_ETHERNET_OVER_SPI == ON
//	struct netDeviceInterface * greenPhyDevice = greenPhyInitNetdevice();
//#if IP_STACK == ON
//	taskArgument = greenPhyDevice;
//#endif
//
//#if COMMAND_LINE_INTERFACE == ON
//	netdeviceAdd(greenPhyDevice,"GreenPHY");
//#endif
//#endif
//
//#if USE_ETHERNET == ON
//	struct netDeviceInterface * ethernetDevice = ethInitNetdevice();
//#if IP_STACK == ON
//	taskArgument = ethernetDevice;
//#endif
//#if COMMAND_LINE_INTERFACE == ON
//	netdeviceAdd(ethernetDevice,"ETH");
//#endif
//#endif
//
//#if IP_STACK == ON
//	xTaskCreate( vuIP_Task, ( signed char * ) "uIP", mainBASIC_WEB_STACK_SIZE, taskArgument, mainUIP_TASK_PRIORITY, NULL );
//#endif
//
//#if ETHERNET_OVER_SPI_TO_ETHERNET_BRIDGE == ON
//	static struct bridgePath path;
//	path.name = "ETH->GreenPHY";
//	path.bridgeSource = ethernetDevice;
//	path.bridgeDestination = greenPhyDevice;
//	initBridgeTask(&path);
//#endif
//
//	size_t mem = xPortGetFreeHeapSize();
//	printToUart("free heap: %d(0x%x)\r\n", mem, mem);
//
#if COMMAND_LINE_INTERFACE == ON
	cliInit();
//#else
//	printToUart("OK\r\n");
#endif

	/*Initialize Queue*/
//	MQTT_Queue = xQueueCreate(3, 24);

	/*Override json malloc functions*/
//	json_set_alloc_funcs(pvPortMalloc, vPortFree);

	vTaskStartScheduler();

	// the application will be only started after the scheduler is ended by the bootloader calling vTaskEndScheduler()

	startApplication();

	// shall never be reached ...

	for (;;)
		;

	return 0;
}
