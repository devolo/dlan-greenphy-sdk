/* Standard includes. */
#include <stdint.h>
#include <stdio.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOS_IP.h"

BaseType_t xApplicationGetRandomNumber( uint32_t * pulNumber )
{
	*pulNumber = rand();
	return pdTRUE;
}

/*
* Callback that provides the inputs necessary to generate a randomized TCP
* Initial Sequence Number per RFC 6528.  In this case just a psuedo random
* number is used so THIS IS NOT RECOMMENDED FOR PRODUCTION SYSTEMS.
*/
extern uint32_t ulApplicationGetNextSequenceNumber( uint32_t ulSourceAddress,
													uint16_t usSourcePort,
													uint32_t ulDestinationAddress,
													uint16_t usDestinationPort )
{
     ( void ) ulSourceAddress;
     ( void ) usSourcePort;
     ( void ) ulDestinationAddress;
     ( void ) usDestinationPort;

     return rand();
}
