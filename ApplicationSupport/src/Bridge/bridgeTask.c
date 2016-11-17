/*
 * bridgeTask.c
 *
 *  Created on: 13.04.2012
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "debug.h"

#include "bridgeTask.h"

#define mainBRIDGE_TASK_PRIORITY			( tskIDLE_PRIORITY + 2 )

static void bridgeReceiverTask( void *pvParameters )
{
	struct bridgePath * path = (struct bridgePath *) pvParameters;
	struct netDeviceInterface * source = path->bridgeSource;
	struct netDeviceInterface * destination = path->bridgeDestination;

	DEBUG_PRINT(DEBUG_NOTICE,"init %s (0x%x)\r\n",path->name,source);
	source->init(source);
	DEBUG_PRINT(DEBUG_NOTICE,"open %s (0x%x)\r\n",path->name,source);
	source->open(source);
	DEBUG_PRINT(DEBUG_NOTICE,"run %s (0x%x)\r\n",path->name,source);

	for( ;; )
	{
		struct netdeviceQueueElement * element = source->rxWithTimeout(source, portMAX_DELAY);
		if(element)
		{
			destination->tx(destination, element );
			element = NULL;
		}
	}
}

void initBridgeTask( struct bridgePath *bridgePath )
{
	static struct bridgePath inverseBridgePath;

	inverseBridgePath.bridgeSource = bridgePath->bridgeDestination;
	inverseBridgePath.bridgeDestination = bridgePath->bridgeSource;
	inverseBridgePath.name = "GreenPHY->ETH";

	DEBUG_PRINT(DEBUG_NOTICE,"init bridge task\r\n");

	xTaskCreate( bridgeReceiverTask, ( signed char * )"GreenRX", 240, &inverseBridgePath, mainBRIDGE_TASK_PRIORITY, NULL );
	DEBUG_PRINT(DEBUG_NOTICE,"create GreenRX task done\r\n");
	xTaskCreate( bridgeReceiverTask, ( signed char * )"ethRX  ", 240, bridgePath        , mainBRIDGE_TASK_PRIORITY, NULL );
	DEBUG_PRINT(DEBUG_NOTICE,"create ethRX task done\r\n");
}
