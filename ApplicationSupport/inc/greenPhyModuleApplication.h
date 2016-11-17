/*
 * greenPhyModuleApplication.h
 *
 *  Created on: 14.09.2012
 */

#ifndef GREENPHYMODULEAPPLICATION_H_
#define GREENPHYMODULEAPPLICATION_H_

#include "greenPhyModuleConfig.h"

/*----------------------------------INCLUDES-BASED-ON-THE-CONFIGURATION------------------------------------*/

#include <debug.h>

#if IP_STACK == ON

/* Task priorities. */
#define mainUIP_TASK_PRIORITY				( tskIDLE_PRIORITY + 3 )

/* The WEB server has a larger stack as it utilises stack hungry string
handling library calls. */
#define mainBASIC_WEB_STACK_SIZE            ( configMINIMAL_STACK_SIZE * 5 )

#include <uIP_Task.h>
#endif

#if USE_ETHERNET_OVER_SPI == ON
#include <GreenPHY.h>
#endif

#if USE_ETHERNET == ON
#include <EthDev_LPC17xx.h>
#include <emac.h>
#endif

#if ETHERNET_OVER_SPI_TO_ETHERNET_BRIDGE == ON
#include <bridgeTask.h>
#endif

#if COMMAND_LINE_INTERFACE == ON
#include <CLI.h>
#endif

#include <netdevice.h>


#endif /* GREENPHYMODULEAPPLICATION_H_ */
