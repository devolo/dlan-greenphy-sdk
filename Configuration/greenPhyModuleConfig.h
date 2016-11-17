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
 * config.h
 *
 */

#ifndef GREEN_PHY_MODULE_CONFIG_H_
#define GREEN_PHY_MODULE_CONFIG_H_

extern const char const * version;
extern const char const * features;

/*-------------------------------VERSION-DEFINITIONS------------------------------------*/

#define FW_VERSION "1.0.16"
#define SDK_VERSION "2.2.0"

/* version string of the firmware */
#define VERSION_STRING "SDK V" SDK_VERSION " FW V" FW_VERSION

/*-------------------------------BASIC-DEFINITIONS------------------------------------*/

#define OFF 0
#define ON 1
#define GREEN_PHY 2
#define LPC1758   3

/*--------------------------CLEAR-ALL-FEATURE-DEFINITIONS--------------------------*/

#undef ETHERNET_OVER_SPI_TO_ETHERNET_BRIDGE
#undef ETHERNET_LPC1758_FLOW_CONTROL
#undef COMMAND_LINE_INTERFACE
#undef HTTP_SERVER
#undef TFTP_CLIENT_IAP
#undef IP_STACK_DEVICE
#undef GREEN_PHY_SIMPLE_QOS
#undef WATCHDOG

/*---------------HERE-USE-THE-DEFINITIONS-FOR-CONFIGURATION----------------------*/

#define ETHERNET_OVER_SPI_TO_ETHERNET_BRIDGE OFF
#define ETHERNET_LPC1758_FLOW_CONTROL ON
#define COMMAND_LINE_INTERFACE OFF
#define HTTP_SERVER ON
#define TFTP_CLIENT_IAP OFF
#define IP_STACK_DEVICE GREEN_PHY // LPC1758
/* GREEN_PHY_SIMPLE_QOS is disabled on purpose; it is currently ALPHA and not yet in release state */
#define GREEN_PHY_SIMPLE_QOS OFF
#define WATCHDOG OFF
#define DHCP_CLIENT OFF

/*------------------------SOME-CHECKS-DONT-TOUCH---------------------------------*/

#undef USE_ETHERNET_OVER_SPI
#undef USE_ETHERNET
#undef IP_STACK

#if HTTP_SERVER == ON || TFTP_CLIENT_IAP == ON
#define IP_STACK ON
#if HTTP_SERVER == ON && TFTP_CLIENT_IAP == ON
#error 'HTTP server and TFTP client can not be used together!'
#endif
#if HTTP_SERVER == ON
typedef struct httpd_state uip_tcp_appstate_t;
#endif
#if TFTP_CLIENT_IAP == ON
#if WATCHDOG == ON
#error 'Watchdog and TFTP client can not be used together!'
#endif
#if DHCP_CLIENT == ON
#error 'DHCP client and TFTP client can not be used together!'
#endif
typedef int uip_tcp_appstate_t;
#endif
#else
typedef int uip_tcp_appstate_t;
#endif

#if ETHERNET_OVER_SPI_TO_ETHERNET_BRIDGE == ON

#if IP_STACK == ON
#error 'Bridging is defined, no IP stack!'
#endif
/* use both 'interfaces' */

#define USE_ETHERNET_OVER_SPI ON
#define USE_ETHERNET ON
#endif

#if IP_STACK == ON

#if ETHERNET_OVER_SPI_TO_ETHERNET_BRIDGE == ON
#error 'IP stack is defined, no bridging!'
#endif

#if IP_STACK_DEVICE == GREEN_PHY
#define USE_ETHERNET_OVER_SPI ON
#endif

#if IP_STACK_DEVICE == LPC1758
#define USE_ETHERNET ON
#endif
#endif

#if COMMAND_LINE_INTERFACE == ON
#define FEATURE_CLI "CLI "
#else
#define FEATURE_CLI ""
#endif

#if IP_STACK == ON
#if HTTP_SERVER == ON
#define FEATURE_IP_STACK "HTTP "
#endif
#if TFTP_CLIENT_IAP == ON
#define FEATURE_IP_STACK "TFTPC "
#endif
#else
#define FEATURE_IP_STACK ""
#endif

#if USE_ETHERNET_OVER_SPI == ON
#define FEATURE_ETHERNET_OVER_SPI "GREEN_PHY "
#else
#define FEATURE_ETHERNET_OVER_SPI ""
#endif

#if USE_ETHERNET == ON
#if ETHERNET_LPC1758_FLOW_CONTROL == ON
#define FEATURE_ETHERNET "ETH_FLOW "
#else
#define FEATURE_ETHERNET "ETH "
#endif
#else
#define FEATURE_ETHERNET ""
#endif

#if GREEN_PHY_SIMPLE_QOS == ON
#define FEATURE_SIMPLE_QOS "sQoS "
#else
#define FEATURE_SIMPLE_QOS ""
#endif

#if WATCHDOG == ON
#define FEATURE_WATCHDOG "WDT "
#else
#define FEATURE_WATCHDOG ""
#endif

#if DHCP_CLIENT == ON
#define FEATURE_DHCP "DHCP "
#else
#define FEATURE_DHCP ""
#endif

#if DHCP_CLIENT == ON
typedef int uip_udp_appstate_t;
#endif

/* shall be the last feature */
#ifdef DEBUG
#define FEATURE_BUILD "DEBUG"
#else
#define FEATURE_BUILD "RELEASE"
#endif

#define FEATURE_STRING FEATURE_CLI FEATURE_IP_STACK FEATURE_ETHERNET_OVER_SPI FEATURE_ETHERNET FEATURE_SIMPLE_QOS FEATURE_WATCHDOG FEATURE_BUILD

/*------------------------------------------------------------------------------*/

#endif /* GREEN_PHY_MODULE_CONFIG_H_ */
