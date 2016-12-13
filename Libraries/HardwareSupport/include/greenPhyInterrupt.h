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
 * greenPhyInterrupt.h
 *
 */

#ifndef GREENPHYINTERRUPT_H_
#define GREENPHYINTERRUPT_H_

/*-----------------------------------------------------------
 * GreenPHY defines
 *-----------------------------------------------------------*/

/* P0[22] (pin 44) is ssp0_int, connected to mspi_intr */
#define GREEN_PHY_INTERRUPT_PORT 0
#define GREEN_PHY_INTERRUPT_PIN  22

/* All GPIO interrupts are connected to to EINT3 interrupt source. */
#define GREEN_PHY_INTERRUPT_ID			(EINT3_IRQn)
#define GREEN_PHY_INTERRUPT_PRIORITY 	(configSPI_INTERRUPT_PRIORITY)
#define GREEN_PHY_INTERRUPT_HANDLER_TASK_PRIORITY 	(configSPI_INTERRUPT_TASK_PRIORITY)

/*-----------------------------------------------------------
 * GreenPHY Interupt handling routines.
 *-----------------------------------------------------------*/

/*
 * Checks if there are unhandled GreenPhy interrupt
 * events available.
 *
 * Returns 1 on events available, 0 on all events handled.
 */
int greenPhyInterruptAvailable(void);

/*
 * Marks all GreenPhy interrupt events as handled.
 */
void greenPhyClearInterrupt(void);

#endif /* GREENPHYINTERRUPT_H_ */
