/*
 * Copyright (c) 2012, devolo AG, Aachen, Germany.
 * All rights reserved.
 *
 * This Software is part of the devolo GreenPHY-SDK.
 *
 * Usage in source form and redistribution in binary form, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Usage in source form is subject to a current end user license agreement
 *    with the devolo AG.
 * 2. Neither the name of the devolo AG nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 3. Redistribution in binary form is limited to the usage on the GreenPHY
 *    module of the devolo AG.
 * 4. Redistribution in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>

/* FreeRTOS.org includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <registerGreenPHY.h>
#include "spiGreenPHY.h"

#include "dma.h"

#include "debug.h"

#include "interuptHandlerGPIO.h"

#include "busyWait.h"
#define INIT_DELAY_CALL busyWaitMs

/* Self includes. */
#include "GreenPHY.h"

/*-----------------------------------------------------------
 * GreenPHY variables.
 *-----------------------------------------------------------*/

struct greenPhyNetdevice {
	/* device interface structure at first */
	struct netDeviceInterface dev;
	/* everything which is used for SPI communication */
	struct qcaspi    spi;
	/* This is used to reference the semaphore that is used to
	 * synchronize the FreeRTOS handler task with the QCA7k
	 * interrupt. */
	xSemaphoreHandle greenPhyInterruptBinarySemaphore;
	/* Count the interrupt events ... */
	volatile unsigned int intReq;
	/* Remember the serviced interrupt; if this is equal to
	 * intReq, no  other interrupt has occurred */
	volatile unsigned int intSvc;
};

/* device GreenPHY */
static struct greenPhyNetdevice grn0 = {
		.dev = { 0 },
		.spi = { 0 },
		.greenPhyInterruptBinarySemaphore = NULL,
		.intReq = 0,
		.intSvc = 0
};

/*-----------------------------------------------------------
 * GreenPHY routines.
 *-----------------------------------------------------------*/

int greenPhyInterruptAvailable(void)
{
	int rv;
	if (grn0.intSvc != grn0.intReq) {
		rv = 1;
		DEBUG_PRINT(GREEN_PHY_INTERUPT,"'%s' yes!\r\n",__func__);
	}
	else
	{
		rv = 0;
		DEBUG_PRINT(GREEN_PHY_INTERUPT,"'%s' no\r\n",__func__);
	}
	return rv;
}

/*-----------------------------------------------------------*/

void greenPhyClearInterrupt(void)
{
	DEBUG_PRINT(GREEN_PHY_INTERUPT,"'%s' intSvc  0x%x != intReq 0x%x !\r\n",__func__,grn0.intSvc,grn0.intReq);
	grn0.intSvc = grn0.intReq;
}

/*-----------------------------------------------------------*/

static void greenPhyInterruptHandlerTask ( void * parameters)
{
	struct greenPhyNetdevice * greenPhy = (struct greenPhyNetdevice *) parameters;

	DEBUG_PRINT(GREEN_PHY_INTERUPT,"'%s' eat up the first event ...\r\n",__func__);
	if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
	{
		xSemaphoreTake( greenPhy->greenPhyInterruptBinarySemaphore, 0 );
	}
	else
	{
		DEBUG_PRINT(DEBUG_ALL,"Scheduler not running: %s\r\n",__func__);
	}

	for (;;)
	{
		if(xTaskGetSchedulerState() != taskSCHEDULER_RUNNING)
		{
			DEBUG_PRINT(DEBUG_ALL,"Scheduler stopped: %s\r\n",__func__);
		}

		DEBUG_PRINT(GREEN_PHY_INTERUPT,"'%s' taking ...\r\n",__func__);
		if( xSemaphoreTake( greenPhy->greenPhyInterruptBinarySemaphore, portMAX_DELAY ) == pdTRUE )
		{
			DEBUG_PRINT(GREEN_PHY_INTERUPT,"'%s' ok!\r\n",__func__);
			/* To get here the interrupt must have occurred.  Process the interrupt. */
			greenPhySpiWorker(&greenPhy->spi);
		}
	}
}

/*-----------------------------------------------------------*/

void greenPhyIrqHandler (portBASE_TYPE * xHigherPriorityTaskWoken)
{
	DEBUG_PRINT(GREEN_PHY_INTERUPT,"{");

	grn0.intReq+=1;
	DEBUG_PRINT(GREEN_PHY_INTERUPT,"0x%x",grn0.intReq);

	/* wake up the handler task */
	if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
	{
		xSemaphoreGiveFromISR( grn0.greenPhyInterruptBinarySemaphore, xHigherPriorityTaskWoken );
	}
	else
	{
		DEBUG_PRINT(DEBUG_ALL,"Scheduler not running:(1)%s\r\n",__func__);
	}

	/* Clear the GreenPHY interrupt bit */
	GPIO_ClearInt(GREEN_PHY_INTERRUPT_PORT,GPIO_MAP_PIN(GREEN_PHY_INTERRUPT_PIN));

	DEBUG_PRINT(GREEN_PHY_INTERUPT,"}");
}

/*-----------------------------------------------------------*/

void greenPhyNetdeviceReset (struct netDeviceInterface * dev)
{
	struct greenPhyNetdevice * greenPhy = (struct greenPhyNetdevice *) dev;
	struct qcaspi *qca = &greenPhy->spi;
	qca->reset_count += 1;
	if(qca->reset_count>=QCA7K_MAX_RESET_COUNT)
	{
		qca->reset_count = 0;
		DEBUG_PRINT(GREEN_PHY_INTERUPT,"GreenPhy hard reset\r\n");
		/* reset is normally active low, so reset ... */
		GPIO_ClearValue(GREEN_PHY_RESET_PORT,GPIO_MAP_PIN(GREEN_PHY_RESET_PIN));
		DEBUG_PRINT(GREEN_PHY_INTERUPT,"GreenPhy reset low\r\n");
		GPIO_SetValue(GREEN_PHY_RESET_PORT,GPIO_MAP_PIN(GREEN_PHY_RESET_PIN));
		GPIO_ClearValue(GREEN_PHY_RESET_PORT,GPIO_MAP_PIN(GREEN_PHY_RESET_PIN));
		/*  ... for 100 ms ... */
		INIT_DELAY_CALL( 1000 / portTICK_RATE_MS);
		/* ... and release QCA7k from reset */
		GPIO_SetValue(GREEN_PHY_RESET_PORT,GPIO_MAP_PIN(GREEN_PHY_RESET_PIN));
		DEBUG_PRINT(GREEN_PHY_INTERUPT,"GreenPhy reset high\r\n");
	}
	else
	{
		DEBUG_PRINT(GREEN_PHY_INTERUPT,"GreenPhy soft reset\r\n");
		uint16_t spi_config = qcaspi_read_register(qca, SPI_REG_SPI_CONFIG);
		qcaspi_write_register(qca, SPI_REG_SPI_CONFIG, spi_config | QCASPI_SLAVE_RESET_BIT);
	}
	startSyncGuard(qca);
}

/*-----------------------------------------------------------*/

static int greenPhyNetdeviceInit (struct netDeviceInterface * dev)
{
	int rv = 1;

	if(xTaskGetSchedulerState() != taskSCHEDULER_RUNNING)
	{
		DEBUG_PRINT(DEBUG_ALL,"Scheduler not running: %s\r\n",__func__);
	}

	struct greenPhyNetdevice * greenPhy = (struct greenPhyNetdevice *) dev;
	if( dev && greenPhy->greenPhyInterruptBinarySemaphore == NULL)
	{
		vSemaphoreCreateBinary( greenPhy->greenPhyInterruptBinarySemaphore );

		if(greenPhy->greenPhyInterruptBinarySemaphore)
		{
			struct qcaspi *qca = &greenPhy->spi;
			rv = greenPhyHandlerInit(qca,dev);

			if (!rv)
			{
				DMA_Init();

				static const signed char * const name = (const signed char * const) "GreenPhyIntHander";
				xTaskCreate( greenPhyInterruptHandlerTask, name, 240, greenPhy, tskIDLE_PRIORITY+4, NULL);

				GPIO_SetDir(GREEN_PHY_INTERRUPT_PORT,GPIO_MAP_PIN(GREEN_PHY_INTERRUPT_PIN),GPIO_IN);
				DEBUG_PRINT(GREEN_PHY_INTERUPT,"->R");
				GPIO_IntCmd(GREEN_PHY_INTERRUPT_PORT,GPIO_MAP_PIN(GREEN_PHY_INTERRUPT_PIN),GPIO_INTERRUPT_RISING_EDGE);
				DEBUG_PRINT(GREEN_PHY_INTERUPT,"->F");
				GPIO_IntCmd(GREEN_PHY_INTERRUPT_PORT,GPIO_MAP_PIN(GREEN_PHY_INTERRUPT_PIN),GPIO_INTERRUPT_FALLING_EDGE);

				/* QCA7000 reset pin set up */
				DEBUG_PRINT(GREEN_PHY_INTERUPT,"GreenPhy reset set to output\r\n");
				GPIO_SetDir(GREEN_PHY_RESET_PORT,GPIO_MAP_PIN(GREEN_PHY_RESET_PIN),GPIO_OUT);
			}
			else
			{
				DEBUG_PRINT(GREEN_PHY_INTERUPT,"'%s' deleting sema!\r\n",__func__);
				vSemaphoreDelete(greenPhy->greenPhyInterruptBinarySemaphore);
				greenPhy->greenPhyInterruptBinarySemaphore = NULL;
			}

		}
	}

	return rv;
}

/*-----------------------------------------------------------*/

static int greenPhyNetdeviceTX(struct netDeviceInterface * dev, struct netdeviceQueueElement * txBuffer)
{
	int rv=0;

	struct greenPhyNetdevice * greenPhy = (struct greenPhyNetdevice *) dev;
	struct qcaspi *qca = &greenPhy->spi;

	uint32_t qid;
#if GREEN_PHY_SIMPLE_QOS == ON
	if (qca->driver_state == SERIAL_OVER_ETH_VER1_MODE)
	{
		qid = qcaspi_get_qid_from_eth_frame(txBuffer);
	}
	else
#endif
	{
		qid = QCAGP_DEFAULT_QUEUE;
	}

	DEBUG_PRINT(GREEN_PHY_TX,"[GreenPHY] QID %d\r\n",qid);

	if(xQueueSend(qca->tx_priority_q[qid],&txBuffer,0) == pdTRUE)
	{
		rv = 1;
	}
	else
	{
		DEBUG_PRINT(GREEN_PHY_INTERUPT,"%s FULL!\r\n",__func__);
		DEBUG_PRINT(DEBUG_BUFFER,"[#C#]\r\n");
		returnQueueElement(&txBuffer);
	}

	if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
	{
		DEBUG_PRINT(GREEN_PHY_INTERUPT,"'%s' giving ...\r\n",__func__);
		xSemaphoreGive(greenPhy->greenPhyInterruptBinarySemaphore);
	}
	else
	{
		DEBUG_PRINT(DEBUG_ALL,"Scheduler not running:(a)%s\r\n",__func__);
	}

	return rv;
}

/*-----------------------------------------------------------*/

static int greenPhyNetdeviceOpen (struct netDeviceInterface * dev)
{
	int rv = 0;

	DEBUG_PRINT(DEBUG_NOTICE,"greenphy open\r\n");

	struct greenPhyNetdevice * greenPhy = (struct greenPhyNetdevice *)  dev;
	greenPhy->dev.tx = greenPhyNetdeviceTX;

	registerInterruptHandlerGPIO(greenPhyIrqHandler,GREEN_PHY_INTERRUPT_PORT,GREEN_PHY_INTERRUPT_PIN);

	/* reset the QCA7k */
	greenPhyNetdeviceReset(dev);

#if GREEN_PHY_SIMPLE_QOS == ON
	struct qcaspi *qca = &greenPhy->spi;

	/* clear the Simpe QoS driver state members */
	qca->driver_state = INITIALIZED;

	qca->driver_state_count = 0;
	int i;
	for(i=0; i<QCAGP_NO_OF_QUEUES; i++)
		qca->max_queue_size[i] = 20;
	memset(qca->target_mac_addr, 0, 6);
	qca->total_credits = 0;
#endif

	// todo wait for QCA7k sync!
	DEBUG_PRINT(DEBUG_NOTICE,"greenphy open done\r\n");

	return rv;
}

/*-----------------------------------------------------------*/

static int greenPhyNetdeviceClose (struct netDeviceInterface * dev)
{
	int rv = 0;
	unregisterInterruptHandlerGPIO(greenPhyIrqHandler);

	struct greenPhyNetdevice * greenPhy = (struct greenPhyNetdevice *)  dev;
	greenPhy->dev.tx = netdeviceReturnBuffer;

	return rv;
}

/*-----------------------------------------------------------*/

static int greenPhyNetdeviceExit (struct netDeviceInterface ** pdev)
{
	int rv = 1;

	if(pdev)
	{
		struct greenPhyNetdevice * greenPhy = (struct greenPhyNetdevice *) * pdev;
		DEBUG_PRINT(GREEN_PHY_INTERUPT,"'%s'!\r\n",__func__);

		if(greenPhy->greenPhyInterruptBinarySemaphore)
		{

			greenPhyNetdeviceClose(&(greenPhy->dev));

			vSemaphoreDelete( greenPhy->greenPhyInterruptBinarySemaphore );
			greenPhy->greenPhyInterruptBinarySemaphore = NULL;
		}
		* pdev = NULL;
		rv = 0;
	}

	return rv;
}

/*-----------------------------------------------------------*/

static struct netdeviceQueueElement * 	greenPhyNetdeviceRxWithTimeout (struct netDeviceInterface * dev,timeout_t timeout_ms)
{
	struct netdeviceQueueElement * rv = NULL;

	struct greenPhyNetdevice * greenPhy = (struct greenPhyNetdevice *) dev;

	struct qcaspi *qca = &greenPhy->spi;

	xQueueReceive(qca->rxQueue,&rv, timeout_ms);

	return rv;
}

/*-----------------------------------------------------------*/

struct netDeviceInterface * greenPhyInitNetdevice ( void )
{
	struct netDeviceInterface * rv = NULL;

	struct greenPhyNetdevice * greenPhy = &grn0;
	if( greenPhy != NULL )
	{
		greenPhy->dev.init = greenPhyNetdeviceInit;
		greenPhy->dev.open = greenPhyNetdeviceOpen;
		greenPhy->dev.close = greenPhyNetdeviceClose;
		greenPhy->dev.exit = greenPhyNetdeviceExit;
		greenPhy->dev.reset = greenPhyNetdeviceReset;
		greenPhy->dev.tx = netdeviceReturnBuffer;
		greenPhy->dev.rxWithTimeout = greenPhyNetdeviceRxWithTimeout;
		rv = &(greenPhy->dev);
	}

	return rv;
}
/*-----------------------------------------------------------*/
