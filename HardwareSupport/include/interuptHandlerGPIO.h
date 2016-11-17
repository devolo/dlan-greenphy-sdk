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
 * interuptHandlerGPIO.h
 *
 */

#ifndef INTERUPTHANDLERGPIO_H_
#define INTERUPTHANDLERGPIO_H_

/* These are GPIO interrupt handler functions. The handler must take
 * care of the GPIO and interrupt status by itself. */
typedef void (*interuptHandlerFunc) (portBASE_TYPE *);

/**
* Registers an interrupt handler function. The first registration
* will enable the global GPIO interrupt handler.
* @param func pointer to the handler being registered.
* @Param port to be bound to.
* @Param pin to be bound to.
* @return 0 on success, otherwise error code.
*/
int registerInterruptHandlerGPIO(interuptHandlerFunc func, int port, int pin);

/**
* Unegisters an interrupt handler function. If the last interrupt
* handler is unregistered, global GPIO interrupt handling is disabled
* @param func  pointer to the handler being registered
* @return 0 on success, otherwise error code.
*/
int unregisterInterruptHandlerGPIO(interuptHandlerFunc func);

#endif /* INTERUPTHANDLERGPIO_H_ */
