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
 *
 * relay.c
 *
 */

#include "lpc17xx_gpio.h"
#include "greenPhyModuleConfig.h"
#include "busyWait.h"
#include "debug.h"
#include "relay.h"

/* definitions for the relay used on MT 2490 */
#define MT2490_RELAY_OFF_PORT	1
#define MT2490_RELAY_OFF_PIN	26
#define MT2490_RELAY_ON_PORT	1
#define MT2490_RELAY_ON_PIN		28

struct GPIO gpioRelayOn   = { MT2490_RELAY_ON_PORT  ,MT2490_RELAY_ON_PIN  };
struct GPIO gpioRelayOff  = { MT2490_RELAY_OFF_PORT ,MT2490_RELAY_OFF_PIN };
static int relayStatus = OFF;

//---------------------------------------------------------------------------------------------------------------

int getRelayStatus(void)
{
	return relayStatus;
}

//---------------------------------------------------------------------------------------------------------------

void setRelayOn(void)
{
	DEBUG_PRINT(DEBUG_NOTICE,"%s()\r\n",__func__);

	GPIO_ClearValue((gpioRelayOn.port), (GPIO_MAP_PIN(gpioRelayOn.pin)));
	busyWaitMs(100);
	GPIO_SetValue((gpioRelayOn.port), (GPIO_MAP_PIN(gpioRelayOn.pin)));
	relayStatus = ON;
}

//---------------------------------------------------------------------------------------------------------------

void setRelayOff(void)
{
	DEBUG_PRINT(DEBUG_NOTICE,"%s()\r\n",__func__);
	GPIO_ClearValue((gpioRelayOff.port), (GPIO_MAP_PIN(gpioRelayOff.pin)));
	busyWaitMs(100);
	GPIO_SetValue((gpioRelayOff.port), (GPIO_MAP_PIN(gpioRelayOff.pin)));
	relayStatus = OFF;
}

//---------------------------------------------------------------------------------------------------------------

void initRelay(void)
{
	DEBUG_PRINT(DEBUG_NOTICE,"%s()\r\n",__func__);
	GPIO_SetDir((gpioRelayOn.port),(GPIO_MAP_PIN(gpioRelayOn.pin)), GPIO_OUT);
	GPIO_SetDir((gpioRelayOff.port),(GPIO_MAP_PIN(gpioRelayOff.pin)), GPIO_OUT);
	GPIO_SetValue((gpioRelayOn.port), (GPIO_MAP_PIN(gpioRelayOn.pin)));
	GPIO_SetValue((gpioRelayOff.port), (GPIO_MAP_PIN(gpioRelayOff.pin)));
	setRelayOff();
}

//---------------------------------------------------------------------------------------------------------------

int printGpio(struct GPIO * gpio)
{
	printToUart("P%d[%d]",gpio->port,gpio->pin );
	return 0;
}

//---------------------------------------------------------------------------------------------------------------
