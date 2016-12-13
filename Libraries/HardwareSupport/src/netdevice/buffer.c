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
 * buffer.c
 *
 */

#include "FreeRTOS.h"
#include "task.h"
#include "buffer.h"
#include "debug.h"

/*-----------------------------------------------------------*/
/*---------------private,-do-not-use-anymore-----------------*/
/*-----------------------------------------------------------*/

/*
 * Search the pool of buffers to find one that is free. If a buffer is found
 * mark it as in use before returning its address.
 */
static uint8_t * prvGetNextBuffer( void );

/*-----------------------------------------------------------*/


struct netdeviceQueueElement {
	uint8_t * pData;			   /* pointer to data stored in the queue element */
	uint16_t length;		   /* size of the data */
	uint16_t removedFromFront; /* */
	unsigned char inUse;	   /* flag */
};

#define GREEN_PHY_NUM_BUFFERS 		ETH_NUM_BUFFERS
#define NUMBER_OF_BUFFERS 			((ETH_NUM_BUFFERS)+(GREEN_PHY_NUM_BUFFERS))
#define TX_ELEMENT_WAIT_DELAY		( 3 / portTICK_RATE_MS )
#define TX_ELEMENT_WAIT_ATTEMPTS 	( 30 )

static struct netdeviceQueueElement queueElement[NUMBER_OF_BUFFERS] = {{ 0,0,0 }};

//#define WAIT_FOR_BUFFER
#define BUFFER_CRITICAL

#ifdef DEBUG
static void checkElement(struct netdeviceQueueElement * element)
{
	if(element)
	{
		int found = 0;
		long x;
		for( x = 0; x < NUMBER_OF_BUFFERS; x++ )
		{
			if(element == &queueElement[ x ])
			{
				found = 1;
				break;
			}
		}
		if(!found)
		{
			DEBUG_PRINT(DEBUG_ERR," element 0x%x not found\r\n",element);
			for( ;; );
		}
	}
}
#endif

struct netdeviceQueueElement * getQueueElementByBuffer(	uint8_t * pData, uint16_t length)
{
	struct netdeviceQueueElement * rv = NULL;

	long x;
#ifdef WAIT_FOR_BUFFER
	unsigned long ulAttempts = 0;
#endif

#ifdef BUFFER_CRITICAL
        portENTER_CRITICAL();
#endif


#ifdef WAIT_FOR_BUFFER
	while( rv == NULL )
#endif
	{
		/* Look through the buffers to find one that is not in use by
			anything else. */
		for( x = 0; x < NUMBER_OF_BUFFERS; x++ )
		{
			if( queueElement[ x ].inUse == pdFALSE )
			{
				queueElement[ x ].inUse = pdTRUE;
				queueElement[ x ].pData = pData;
				queueElement[ x ].length = length;
				queueElement[ x ].removedFromFront = 0;
				rv = &queueElement[ x ];
				DEBUG_PRINT(DEBUG_BUFFER," put 0x%x to queueElement 0x%x (%d)\r\n",pData,rv,x);
				break;
			}
		}

#ifdef WAIT_FOR_BUFFER
		/* Was a buffer found? */
		if( rv == NULL )
		{
			ulAttempts++;

			if( ulAttempts >= TX_ELEMENT_WAIT_ATTEMPTS )
			{
				break;
			}

			/* Wait then look again. */
			vTaskDelay( TX_ELEMENT_WAIT_DELAY );
		}
#endif
	}

#ifdef BUFFER_CRITICAL
        portEXIT_CRITICAL();
#endif

	return rv;
}

uint8_t * getDataFromQueueElement(struct netdeviceQueueElement * element)
{
	uint8_t * rv = NULL;
	if(element)
	{
		DEBUG_EXECUTE(checkElement(element));
		uint16_t offset = element->removedFromFront;
		if(offset > element->length)
		{
			offset = 0;
		}
		rv = element->pData+offset;
	}

	return rv;
}

uint8_t * removeDataFromQueueElement(struct netdeviceQueueElement ** ppelement)
{
	uint8_t * rv = NULL;
	if(ppelement)
	{
		struct netdeviceQueueElement * pelement = *ppelement;
		if(pelement)
		{
#ifdef BUFFER_CRITICAL
	portENTER_CRITICAL();
#endif
			rv = pelement->pData;
			pelement->pData = NULL;
			DEBUG_PRINT(DEBUG_BUFFER,"[#I#]\r\n");
			returnQueueElement(ppelement);
#ifdef BUFFER_CRITICAL
	portEXIT_CRITICAL();
#endif
		}
		else
		{
			ppelement = NULL;
		}
	}
	return rv;
}

uint16_t getLengthFromQueueElement(struct netdeviceQueueElement * element)
{
	uint16_t rv = 0x0;
	if(element)
	{
		DEBUG_EXECUTE(checkElement(element));
		rv = element->length-element->removedFromFront;
		if(rv>element->length)
		{
			rv = 0;
		}
	}

	return rv;
}

void returnQueueElement(struct netdeviceQueueElement ** ppElement)
{
	if(ppElement)
	{
		struct netdeviceQueueElement * eElement = *ppElement;
		*ppElement = NULL;
		if(eElement)
		{
#ifdef BUFFER_CRITICAL
	portENTER_CRITICAL();
#endif
	        DEBUG_EXECUTE(checkElement(eElement));
			DEBUG_PRINT(DEBUG_BUFFER," returning queueElement 0x%x %d\r\n",eElement);
			eElement->inUse = pdFALSE;
			if(eElement->pData)
			{
				prvReturnBuffer(eElement->pData);
			}
			eElement->pData = NULL;
			eElement->length = 0;
			eElement->removedFromFront = 0;
#ifdef BUFFER_CRITICAL
	portEXIT_CRITICAL();
#endif
		}
	}
}


void removeBytesFromFrontOfQueueElement(struct netdeviceQueueElement * pElement, uint16_t toRemove)
{
	pElement->removedFromFront += toRemove;
}

/* Each ucBufferInUse index corresponds to a position in the pool of buffers.
If the index contains a 1 then the buffer within pool is in use, if it
contains a 0 then the buffer is free. */
static unsigned char ucBufferInUse[ NUMBER_OF_BUFFERS ] = { pdFALSE };
static uint8_t * prvGetNextBuffer( void )
{
long x;
uint8_t * pucReturn = NULL;
#ifdef WAIT_FOR_BUFFER
unsigned long ulAttempts = 0;
#endif

#ifdef BUFFER_CRITICAL
	portENTER_CRITICAL();
#endif

#ifdef WAIT_FOR_BUFFER
	while( pucReturn == NULL )
#endif
	{
		/* Look through the buffers to find one that is not in use by
		anything else. */
		for( x = 0; x < NUMBER_OF_BUFFERS; x++ )
		{
			if( ucBufferInUse[ x ] == pdFALSE )
			{
				ucBufferInUse[ x ] = pdTRUE;
				pucReturn = ( uint8_t * ) ETH_BUF( x );
				break;
			}
		}

#ifdef WAIT_FOR_BUFFER
		/* Was a buffer found? */
		if( pucReturn == NULL )
		{
			ulAttempts++;

			if( ulAttempts >= emacBUFFER_WAIT_ATTEMPTS )
			{
				break;
			}

			/* Wait then look again. */
			vTaskDelay( emacBUFFER_WAIT_DELAY );
		}
#endif
	}

#ifdef BUFFER_CRITICAL
	portEXIT_CRITICAL();
#endif

	return pucReturn;
}
/*-----------------------------------------------------------*/

void prvReturnBuffer( uint8_t * pucBuffer )
{
	if (pucBuffer)
	{
#ifdef BUFFER_CRITICAL
		portENTER_CRITICAL();
#endif

		int found = 0;
		long x;

		/* Return a buffer to the pool of free buffers. */
		for( x = 0; x < NUMBER_OF_BUFFERS; x++ )
		{
			if (( uint8_t * ) ETH_BUF( x ) == pucBuffer )
			{
				ucBufferInUse[ x ] = pdFALSE;
				found = 1;
				break;
			}
		}

		if (!found)
		{
#ifdef BUFFER_CRITICAL
			portEXIT_CRITICAL();
#endif
			DEBUG_PRINT(DEBUG_ERR," buffer 0x%x not found\r\n",pucBuffer);
			for( ;; );
		}

#ifdef BUFFER_CRITICAL
		portEXIT_CRITICAL();
#endif
	}
}
/*-----------------------------------------------------------*/

struct netdeviceQueueElement * getQueueElement(void)
{
	struct netdeviceQueueElement * rv = NULL;
	uint8_t * pData = prvGetNextBuffer();
	if (pData)
	{
		rv = getQueueElementByBuffer(pData,0);
		if (!rv) {
			DEBUG_PRINT(DEBUG_ERR|DEBUG_BUFFER,"no queue element, returning 0x%x\r\n",pData);
			prvReturnBuffer(pData);
			pData = NULL;
		}
	}
	return rv;
}

/*-----------------------------------------------------------*/

void setLengthOfQueueElement(struct netdeviceQueueElement * element, uint16_t length)
{
	element->length = length;
}
/*-----------------------------------------------------------*/
