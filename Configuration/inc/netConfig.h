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
 * netConfig.h
 *
 */

#ifndef NETCONFIG_H_
#define NETCONFIG_H_

/* MAC address configuration. */
#define configMAC_ADDR0	0x00
#define configMAC_ADDR1	0x0b
#define configMAC_ADDR2	0x3b
#define configMAC_ADDR3	0x7f
#define configMAC_ADDR4	0x7d
#define configMAC_ADDR5	0x9a

#define UIP_ETHADDR0    configMAC_ADDR0  /**< The first octet of the Ethernet
				 address if UIP_FIXEDETHADDR is
				 1. \hideinitializer */
#define UIP_ETHADDR1    configMAC_ADDR1 /**< The second octet of the Ethernet
				 address if UIP_FIXEDETHADDR is
				 1. \hideinitializer */
#define UIP_ETHADDR2    configMAC_ADDR2  /**< The third octet of the Ethernet
				 address if UIP_FIXEDETHADDR is
				 1. \hideinitializer */
#define UIP_ETHADDR3    configMAC_ADDR3  /**< The fourth octet of the Ethernet
				 address if UIP_FIXEDETHADDR is
				 1. \hideinitializer */
#define UIP_ETHADDR4    configMAC_ADDR4  /**< The fifth octet of the Ethernet
				 address if UIP_FIXEDETHADDR is
				 1. \hideinitializer */
#define UIP_ETHADDR5    configMAC_ADDR5  /**< The sixth octet of the Ethernet
				 address if UIP_FIXEDETHADDR is
				 1. \hideinitializer */

/* IP address configuration. */
#define configIP_ADDR0		192
#define configIP_ADDR1		168
#define configIP_ADDR2		1
#define configIP_ADDR3		200

/* Netmask configuration. */
#define configNET_MASK0		255
#define configNET_MASK1		255
#define configNET_MASK2		255
#define configNET_MASK3		0

/* default router ip address */
#define configGATEWAY_ADDR0		192
#define configGATEWAY_ADDR1		168
#define configGATEWAY_ADDR2		1
#define configGATEWAY_ADDR3		1

/* default DNS SERVER ip address */
#define configDNS_SERVER_ADDR0		192
#define configDNS_SERVER_ADDR1		168
#define configDNS_SERVER_ADDR2		1
#define configDNS_SERVER_ADDR3		1

/* TFTP server address */
#define configTFTP_IP_ADDR0		192
#define configTFTP_IP_ADDR1		168
#define configTFTP_IP_ADDR2		0
#define configTFTP_IP_ADDR3		5

#endif /* NETCONFIG_H_ */
