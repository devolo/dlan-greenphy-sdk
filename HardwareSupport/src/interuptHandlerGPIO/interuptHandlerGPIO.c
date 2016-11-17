#include "FreeRTOS.h"
#include "lpc17xx_gpio.h"
#include "debug.h"

#include "interuptHandlerGPIO.h"

struct interuptHandlerGPIO {
	interuptHandlerFunc func;
	int port;
	int pin;
};

#define NUMBER_OF_INTERRUPT_HANDLER 5

struct interruptDispatcher {
	struct interuptHandlerGPIO handler[NUMBER_OF_INTERRUPT_HANDLER];
	int used;
};

static struct interruptDispatcher dispatcher = {
		.handler = {{NULL,0,0}},
		.used = 0
};

int registerInterruptHandlerGPIO(interuptHandlerFunc func, int port, int pin)
{
	int rv=1;
	if(dispatcher.used < NUMBER_OF_INTERRUPT_HANDLER)
	{
		int i;
		int found = 0;
		portENTER_CRITICAL();
		for(i=0;i<dispatcher.used;i+=1)
		{
			if(dispatcher.handler[i].func == func)
			{
				found = 1;
			}
		}
		if(!found)
		{
			if(dispatcher.used == 0)
			{
				NVIC_SetPriority(EINT3_IRQn, configGPIO_INTERRUPT_PRIORITY);
				NVIC_EnableIRQ(EINT3_IRQn);
			}
			dispatcher.handler[dispatcher.used].func = func;
			dispatcher.handler[dispatcher.used].port = port;
			dispatcher.handler[dispatcher.used].pin  = pin;
			dispatcher.used += 1;
			rv = 0;
		}
		portEXIT_CRITICAL();
	}
	return rv;
}

int unregisterInterruptHandlerGPIO(interuptHandlerFunc func)
{
	int rv=1;
	int i;
	portENTER_CRITICAL();
	for(i=0;i<dispatcher.used;i+=1)
	{
		if(dispatcher.handler[i].func == func)
		{
			dispatcher.used -= 1;
			if(dispatcher.used == 0)
			{
				NVIC_DisableIRQ(EINT3_IRQn);
			}
			else
			{
				dispatcher.handler[i].func = dispatcher.handler[dispatcher.used].func;
				dispatcher.handler[i].port = dispatcher.handler[dispatcher.used].port;
				dispatcher.handler[i].pin = dispatcher.handler[dispatcher.used].pin;
			}
			dispatcher.handler[dispatcher.used].func = NULL;
			dispatcher.handler[dispatcher.used].port = 0;
			dispatcher.handler[dispatcher.used].pin  = 0;
			rv = 0;
			break;
		}
	}
	portEXIT_CRITICAL();
	return rv;
}

void EINT3_IRQHandler (void)
{
	DEBUG_PRINT(GPIO_INTERUPT,"(");

	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	int i;
	int handled = FALSE;

	for(i=0;i<NUMBER_OF_INTERRUPT_HANDLER;i+=1)
	{
		DEBUG_PRINT(GPIO_INTERUPT,"%d:",i);
		if(dispatcher.handler[i].func)
		{
			DEBUG_PRINT(GPIO_INTERUPT,"P%d[%d]:",dispatcher.handler[i].port,dispatcher.handler[i].pin);
			if (GPIO_GetIntStatus(dispatcher.handler[i].port, dispatcher.handler[i].pin, GPIO_INTERRUPT_RISING_EDGE)
					||
				GPIO_GetIntStatus(dispatcher.handler[i].port, dispatcher.handler[i].pin, GPIO_INTERRUPT_FALLING_EDGE)
				)
			{
				DEBUG_PRINT(GPIO_INTERUPT,"<");
				dispatcher.handler[i].func(&xHigherPriorityTaskWoken);
				DEBUG_PRINT(GPIO_INTERUPT,">");
				handled = TRUE;
			}
		}
		else
			break;
	}

	if(!handled)
	{
		/* Clear all possible interrupt sources at GPIOs. */
		struct gpios_to_check {
			int port;
			int start_pin;
			int end_pin;
		};

		/*
		 * For NXP1758 these are:
		 * P0[30:0] P0[14:12] NA
		 * P2[13:0]*/
		static const struct gpios_to_check GIPOs[] = {
				{0,0,12},
				{0,14,30},
				{2,0,13}
		};
		for(i=0;i<sizeof(GIPOs)/sizeof(struct gpios_to_check);i+=1)
		{
			int j;
			for(j=GIPOs[i].start_pin;j<GIPOs[i].end_pin;j+=1)
			{
				if (GPIO_GetIntStatus(GIPOs[i].port, j, GPIO_INTERRUPT_RISING_EDGE)
						||
					GPIO_GetIntStatus(GIPOs[i].port, j, GPIO_INTERRUPT_FALLING_EDGE)
					)
				{
					GPIO_ClearInt(GIPOs[i].port,GPIO_MAP_PIN(j));
					DEBUG_PRINT(GPIO_INTERUPT,"cP%d[%d]:",GIPOs[i].port,j);
				}
			}
		}
	}

	if(xHigherPriorityTaskWoken )
	{
		DEBUG_PRINT(GPIO_INTERUPT,"W");
	}

	DEBUG_PRINT(GPIO_INTERUPT,")\r\n");
	portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}

/*-----------------------------------------------------------*/
