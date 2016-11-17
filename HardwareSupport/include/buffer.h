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
 * buffer.h
 *
 */

#ifndef BUFFER_H_
#define BUFFER_H_

/* Hardware specific includes. */
#include "EthDev_LPC17xx.h"

#include <lpc_types.h>

/* If no buffers are available, then wait this long before looking again.... */
#define emacBUFFER_WAIT_DELAY	( 1 / portTICK_RATE_MS )

/* ...and don't look more than this many times. */
#define emacBUFFER_WAIT_ATTEMPTS	( 30 )




/* todo clean up buffer handling and remove public usage of
 * prvGetNextBuffer() and prvReturnBuffer().*/




/* The netdeviceQueueElement is used in conjunction with queues to TX
 * and RX Ethernet frames. */
struct netdeviceQueueElement;

/**
 * Gets a new queue element.
 * This element must be returned with returnQueueElement() after usage.
 *
 * @param data of the queue element, which is a buffer got by prvGetNextBuffer()
 * @param length of data in the buffer
 * @return New queue element, NULL on failure.
 */
struct netdeviceQueueElement * getQueueElementByBuffer(data_t, length_t);

/*-----------------------------------------------------------*/

/**
 * Gets a new queue element, hiding prvGetNextBuffer() and prvReturnBuffer().
 * This element must be returned with returnQueueElement() after usage.
 *
 * @return New queue element, NULL on failure.
 */
struct netdeviceQueueElement * getQueueElement(void);

/*-----------------------------------------------------------*/

/**
 * Gets the data from a queue element.
 *
 * @param element itself
 * @return data.
 */
data_t getDataFromQueueElement(struct netdeviceQueueElement * element);


/**
 * Remove the data from a queue element.
 *
 * @param element itself
 * @return data.
 */
data_t removeDataFromQueueElement(struct netdeviceQueueElement ** element);

/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/

/**
 * Gets the length of data from a queue element.
 *
 * @param element itself
 * @return length.
 */
length_t getLengthFromQueueElement(struct netdeviceQueueElement * element);

/**
 * Sets the length of data of a queue element.
 *
 * @param element itself
 * @param length of the element
 */
 void setLengthOfQueueElement(struct netdeviceQueueElement * element, length_t length);


 /**
  * Removes length of data from the start of the queue element.
  *
  * @param element itself
  * @param length to be removed from the element
  */
void removeBytesFromFrontOfQueueElement(struct netdeviceQueueElement * pElement, length_t toRemove);

/*-----------------------------------------------------------*/

/**
 * Used to return a queue element to the pool. element will be destroyed.
 *
 * @param element to be returned.
 * @return Nothing.
 */
void returnQueueElement(struct netdeviceQueueElement ** element);

/*-----------------------------------------------------------*/

/*
 * Return an allocated buffer extracted by removeDataFromQueueElement()
 * to the pool of free buffers.
 */
void prvReturnBuffer( data_t );

/*-----------------------------------------------------------*/

#endif /* BUFFER_H_ */
