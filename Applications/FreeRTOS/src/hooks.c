/*
 * Copyright (c) 2023, devolo GmbH, Aachen, Germany.
 * All rights reserved.
 *
 * This Software is part of the devolo GreenPHY-SDK.
 *
 * Usage in source form and redistribution in binary form, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Usage in source form is subject to a current end user license agreement
 *    with the devolo GmbH.
 * 2. Neither the name of the devolo GmbH nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 3. Redistribution in binary form is limited to the usage on the GreenPHY
 *    module of the devolo GmbH.
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

/* LPCOpen includes. */
#include "board.h"

/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include "task.h"

/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook(void) {
	DEBUGSTR("Malloc failed!\r\n");
	for (;;)
		;
}
/*-----------------------------------------------------------*/

void vApplicationTickHook(void) {
	/* Called from every tick interrupt */
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook(void) {
	/* Called from idle task */
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook(xTaskHandle pxTask, char *pcTaskName) {
	/* This function will get called if a task overflows its stack. */

	(void) pxTask;
	(void) pcTaskName;

	for (;;)
		;
}
/*-----------------------------------------------------------*/

void vAssertCalled( const char *pcFilename, int lLine )
{
	DEBUGOUT( "Assert failed: line %d in %s\r\n", lLine, pcFilename );
	for( ;; )
		;
}
/*-----------------------------------------------------------*/

void vConfigureTimerForRunTimeStats(void) {
	/* This function configures a timer that is used as the time base when
	 collecting run time statistical information - basically the percentage
	 of CPU time that each task is utilising.  It is called automatically when
	 the scheduler is started (assuming configGENERATE_RUN_TIME_STATS is set
	 to 1). */

	/* Power up and feed the timer. */
	Chip_TIMER_Init(LPC_TIMER0);

	Chip_Clock_SetPCLKDiv(SYSCTL_PCLK_TIMER0, SYSCTL_CLKDIV_1);

	/* Reset Timer 0 */
	Chip_TIMER_Reset(LPC_TIMER0);

	/* Just count up. */
	Chip_TIMER_TIMER_SetCountClockSrc(LPC_TIMER0, 0, 0);

	/* Prescale to a frequency that is good enough to get a decent resolution,
	 but not too fast so as to overflow all the time. */
	Chip_TIMER_PrescaleSet(LPC_TIMER0, ( configCPU_CLOCK_HZ / 10000UL) - 1UL);

	/* Start the counter. */
	Chip_TIMER_Enable(LPC_TIMER0);
}
/*-----------------------------------------------------------*/
