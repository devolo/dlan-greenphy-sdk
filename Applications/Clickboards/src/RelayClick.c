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

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include <string.h>
#include <stdlib.h>
#include "clickboardIO.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "debug.h"
#include "RelayClick.h"






void Relay_Click_Task(void *pvParameters){

	int iValue = 0;


	/* Init */
	if ((int)pvParameters == SLOT1)
		{
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD1_CS_GPIO_PORT_NUM, CLICKBOARD1_CS_GPIO_BIT_NUM, GPIO_OUTPUT);
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD1_PWM_GPIO_PORT_NUM, CLICKBOARD1_PWM_GPIO_BIT_NUM, GPIO_OUTPUT);
	}
	else if ((int)pvParameters == SLOT2)
		{
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD2_CS_GPIO_PORT_NUM, CLICKBOARD2_CS_GPIO_BIT_NUM, GPIO_OUTPUT);
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD2_PWM_GPIO_PORT_NUM, CLICKBOARD2_PWM_GPIO_BIT_NUM, GPIO_OUTPUT);
	}


	/* Task Work */
	while (1){


		if ((int)pvParameters == SLOT1)
					{
					Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD1_CS_GPIO_PORT_NUM, CLICKBOARD1_CS_GPIO_BIT_NUM, iValue & 1);
					Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD1_PWM_GPIO_PORT_NUM, CLICKBOARD1_PWM_GPIO_BIT_NUM, iValue & 2);
					}
				else if ((int)pvParameters == SLOT2)
					{
					Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD2_CS_GPIO_PORT_NUM, CLICKBOARD2_CS_GPIO_BIT_NUM, iValue & 1);
					Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD2_PWM_GPIO_PORT_NUM, CLICKBOARD2_PWM_GPIO_BIT_NUM, iValue & 2);
					}
				vTaskDelay(1000);

	}
}






