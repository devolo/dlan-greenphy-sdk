/*
 * Copyright (c) 2017, devolo AG, Aachen, Germany.
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
 */

#ifndef GREENPHYSDKNETCONFIG_H_
#define GREENPHYSDKNETCONFIG_H_

/* Enumerate available network interfaces. */
#define netconfigETH_INTERFACE         1
#define netconfigPLC_INTERFACE         2
#define netconfigBRIDGE_INTERFACE      3

/* IP stack configuration. */
#define netconfigUSE_IP                1
#define netconfigUSE_DHCP              1
#define netconfigUSE_BRIDGE            1
#define netconfigIP_INTERFACE          netconfigBRIDGE_INTERFACE /* Select one of the three defines above. */
#define netconfigUSEMQTT               1

/* Hostname, used for DHCP */
#define netconfigUSE_DYNAMIC_HOSTNAME  1  /* If defined hostname will be "devolo-MAC" with MAC being the last
										   three characters of the hex formatted MAC address. */
#define netconfigHOSTNAME              "GreenPHY evalboard II"  /* Used, if dynamic hostname is disabled. */

/* MAC address configuration. */
#define netconfigMAC_ADDR0	           0x00
#define netconfigMAC_ADDR1	           0x0b
#define netconfigMAC_ADDR2	           0x3b
#define netconfigMAC_ADDR3	           0x7f
#define netconfigMAC_ADDR4	           0x7d
#define netconfigMAC_ADDR5	           0x9a

/* IP address configuration. */
#define netconfigIP_ADDR0              192
#define netconfigIP_ADDR1              168
#define netconfigIP_ADDR2              1
#define netconfigIP_ADDR3              100

/* Netmask configuration. */
#define netconfigNET_MASK0             255
#define netconfigNET_MASK1             255
#define netconfigNET_MASK2             255
#define netconfigNET_MASK3             0

/* default router ip address */
#define netconfigGATEWAY_ADDR0         192
#define netconfigGATEWAY_ADDR1         168
#define netconfigGATEWAY_ADDR2         1
#define netconfigGATEWAY_ADDR3         1

/* default DNS SERVER ip address */
#define netconfigDNS_SERVER_ADDR0      8
#define netconfigDNS_SERVER_ADDR1      8
#define netconfigDNS_SERVER_ADDR2      8
#define netconfigDNS_SERVER_ADDR3      8

/* MQTT Credentials */
/*
#define netconfigMQTT_BROKER           "mqtt.relayr.io"
#define netconfigMQTT_PORT             1883
#define netconfigMQTT_CLIENT           "TCdqKfhZeQriWr/7PNmy4mw"
#define netconfigMQTT_USER             "09da8a7e-165e-42b8-96af-fecf366cb89b"
#define netconfigMQTT_PASSWORT         "wy5t6n.SQBQx"*/
#define netconfigMQTT_TOPIC            "/v1/6c1e2951-47cf-458c-a8ba-221dbc8b8898/data"

#endif /* GREENPHYSDKNETCONFIG_H_ */
