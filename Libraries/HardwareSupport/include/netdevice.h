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
 * netdevice.h
 *
 */

#ifndef NETDEVICE_H_
#define NETDEVICE_H_

#include <buffer.h>

/* netdevice interface used for ETH like devices
 *
 * Before opening the device with (*open)(), it has to be initialized
 * with (*init)() to work properly.
 * (*rxWithTimeout)() can then be used to receive Ethernet frames stored in
 * netdeviceQueueElement from the device. Use timeout_t (unsigned long) to specify the time
 * waited until the call returns.
 * Before shutting down the device with (*exit)(), it has to be closed with
 * (*close)().
 * If necessary, (*reset)() can be used at any time.*/

struct netDeviceInterface {
	int 							(* init)			(struct netDeviceInterface *);
	int 							(* open)			(struct netDeviceInterface *);
	int 							(* close)			(struct netDeviceInterface *);
	int 							(* exit)			(struct netDeviceInterface **);

	void							(* reset)			(struct netDeviceInterface *);
	int 							(* tx)				(struct netDeviceInterface *,struct netdeviceQueueElement *);
	struct netdeviceQueueElement * 	(* rxWithTimeout)	(struct netDeviceInterface *,unsigned long );
};


/*====================================================================*
 *
 *   netdeviceReturnBuffer
 *
 *   Default tx method and shall be used by all devices until (*open)()
 *   is called. It returns properly the netdeviceQueueElement.
 *
 * Return:   0 on success.
 *
 *--------------------------------------------------------------------*/

int netdeviceReturnBuffer(struct netDeviceInterface *, struct netdeviceQueueElement *);

#endif /* NETDEVICE_H_ */
