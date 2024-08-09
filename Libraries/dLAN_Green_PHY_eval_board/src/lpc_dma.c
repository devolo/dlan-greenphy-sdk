/*
 * Copyright (c) 2016-2024, devolo solutions GmbH, Aachen, Germany.
 * All rights reserved.
 *
 * This Software is part of the devolo GreenPHY-SDK.
 *
 * Usage in source form and redistribution in binary form, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Usage in source form is subject to a current end user license agreement
 *    with the devolo solutions GmbH.
 * 2. Neither the name of the devolo solutions GmbH nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 3. Redistribution in binary form is limited to the usage on the GreenPHY
 *    module of the devolo solutions GmbH.
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
 *
 */

/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include "task.h"

/* GreenPHY SDK includes. */
#include "lpc_dma.h"

struct interruptHandlerDMA {
	interruptHandlerFunc func;
};

struct interruptDispatcherDMA {
	struct interruptHandlerDMA handler[GPDMA_NUMBER_CHANNELS];
	int used;
};

static struct interruptDispatcherDMA dispatcher = {
		.handler = {{NULL}},
		.used = 0
};


Status registerInterruptHandlerDMA( uint8_t ChannelNum, interruptHandlerFunc func )
{
Status rv = ERROR;

	taskENTER_CRITICAL();
	if( func && !dispatcher.handler[ChannelNum].func ) {
		if(dispatcher.used == 0)
		{
			NVIC_SetPriority(DMA_IRQn, configDMA_INTERRUPT_PRIORITY);
			NVIC_EnableIRQ(DMA_IRQn);
		}
		dispatcher.handler[ChannelNum].func = func;
		dispatcher.used += 1;
		rv = SUCCESS;
	}
	taskEXIT_CRITICAL();

	return rv;
}

Status unregisterInterruptHandlerDMA( uint8_t ChannelNum )
{
uint32_t interrupts;
Status rv = ERROR;

	interrupts = taskENTER_CRITICAL_FROM_ISR();
	if( dispatcher.handler[ChannelNum].func )
	{
		dispatcher.handler[ChannelNum].func = NULL;
		dispatcher.used -= 1;
		if(dispatcher.used == 0)
		{
			NVIC_DisableIRQ(DMA_IRQn);
		}
		rv = SUCCESS;
	}
	taskEXIT_CRITICAL_FROM_ISR(interrupts);

	return rv;
}

void DMA_IRQHandler( void )
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	int ChannelNum;

	for( ChannelNum = 0; ChannelNum < GPDMA_NUMBER_CHANNELS; ++ChannelNum )
	{
		if( dispatcher.handler[ChannelNum].func )
		{
			if( Chip_GPDMA_Interrupt(LPC_GPDMA, ChannelNum) == SUCCESS )
			{
				dispatcher.handler[ChannelNum].func( &xHigherPriorityTaskWoken );
				unregisterInterruptHandlerDMA( ChannelNum );
			}
		}
	}

	portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}
