/*
    FreeRTOS V7.1.0 - Copyright (C) 2011 Real Time Engineers Ltd.


    ***************************************************************************
     *                                                                       *
     *    FreeRTOS tutorial books are available in pdf and paperback.        *
     *    Complete, revised, and edited pdf reference manuals are also       *
     *    available.                                                         *
     *                                                                       *
     *    Purchasing FreeRTOS documentation will not only help you, by       *
     *    ensuring you get running as quickly as possible and with an        *
     *    in-depth knowledge of how to use FreeRTOS, it will also help       *
     *    the FreeRTOS project to continue with its mission of providing     *
     *    professional grade, cross platform, de facto standard solutions    *
     *    for microcontrollers - completely free of charge!                  *
     *                                                                       *
     *    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
     *                                                                       *
     *    Thank you for using FreeRTOS, and thank you for your support!      *
     *                                                                       *
    ***************************************************************************


    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    >>>NOTE<<< The modification to the GPL is included to allow you to
    distribute a combined work that includes FreeRTOS without being obliged to
    provide the source code for proprietary components outside of the FreeRTOS
    kernel.  FreeRTOS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/

/* Standard includes. */
#include <string.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* uip includes. */
#include "uip.h"
#include "uIP_Task.h"
#include "uip_arp.h"
#include "httpd.h"
#include "timer.h"
#include "clock-arch.h"

#include "greenPhyModuleApplication.h"
#include "bootloaderapp.h"

/* Demo includes. */

/*-----------------------------------------------------------*/

/* How long to wait before attempting to connect the MAC again. */
#define uipINIT_WAIT    ( 100 / portTICK_RATE_MS )

/* Shortcut to the header within the Rx buffer. */
#define xHeader ((struct uip_eth_hdr *) &uip_buf[ 0 ])

/* Standard constant. */
#define uipTOTAL_FRAME_HEADER_SIZE	54

/*-----------------------------------------------------------*/

/*
 * Setup the MAC address in the MAC itself, and in the uIP stack.
 */
static void prvSetMACAddress( void );

/*
 * Port functions required by the uIP stack.
 */
void clock_init( void );
clock_time_t clock_time( void );

/*-----------------------------------------------------------*/

void clock_init(void)
{
	/* This is done when the scheduler starts. */
}
/*-----------------------------------------------------------*/

clock_time_t clock_time( void )
{
	return xTaskGetTickCount();
}
/*-----------------------------------------------------------*/

void sendBuffer(struct netDeviceInterface * netDevice)
{
	struct netdeviceQueueElement * element = NULL;

	while ( ( uip_len > 0 ) && (uip_buf != NULL) )
	{
		element = getQueueElementByBuffer(uip_buf, uip_len);
		if (element)
		{
			DEBUG_PRINT(DEBUG_BUFFER," sending uip_buf 0x%x\r\n",uip_buf);
			netDevice->tx(netDevice,element);
			uip_buf = NULL;
			uip_len = 0;
		}
		else
		{
			DEBUG_PRINT(DEBUG_ERR|DEBUG_BUFFER," not sending uip_buf 0x%x, waiting ....\r\n",uip_buf);
		}
	}
}

/*-----------------------------------------------------------*/

void vuIP_Task( void *pvParameters )
{
portBASE_TYPE i;
uip_ipaddr_t xIPAddr;
struct timer periodic_timer, arp_timer;

	/* Initialise the uIP stack. */
	timer_set( &periodic_timer, configTICK_RATE_HZ / 2 );
	timer_set( &arp_timer, configTICK_RATE_HZ * 10 );
	uip_init();

#if DHCP_CLIENT == ON
    struct uip_eth_addr mac = { 0x00, 0x0b, 0x3b, 0x7f, 0x7d, 0x9a};
    uip_setethaddr(mac);
    dhcpc_init(&mac, 6);
#else
	uip_ipaddr( xIPAddr, configIP_ADDR0, configIP_ADDR1, configIP_ADDR2, configIP_ADDR3 );
	uip_sethostaddr( xIPAddr );
	uip_ipaddr( xIPAddr, configNET_MASK0, configNET_MASK1, configNET_MASK2, configNET_MASK3 );
	uip_setnetmask( xIPAddr );
	uip_ipaddr(xIPAddr, configDRTR_IP_ADDR0, configDRTR_IP_ADDR1, configDRTR_IP_ADDR2, configDRTR_IP_ADDR3);
	uip_setdraddr(xIPAddr);
	uip_ipaddr(xIPAddr, configDNS_IP_ADDR0, configDNS_IP_ADDR1, configDNS_IP_ADDR2, configDNS_IP_ADDR3);
	//resolv_conf(xIPAddr);
	readflash();
#endif

#if HTTP_SERVER == ON
	httpd_init();
#endif

	prvSetMACAddress();

#if TFTP_CLIENT_IAP == ON
	bl_init();
#endif
	struct netDeviceInterface * netDevice = (struct netDeviceInterface *) pvParameters;

	netDevice->init(netDevice);
	netDevice->open(netDevice);

	for( ;; )
	{
		if(uip_buf == NULL)
		{

			struct netdeviceQueueElement * element = getQueueElement();
			uip_buf = removeDataFromQueueElement(&element);
			uip_len = 0;

			if(uip_buf == NULL)
			{
				DEBUG_PRINT(DEBUG_ERR|DEBUG_BUFFER," %s really no uip_buf 0x%x\r\n",__func__,uip_buf);
				continue;
			}

		}

#if TFTP_CLIENT_IAP == ON
		diag_appcall();
		bl_appcall();
#endif
		struct netdeviceQueueElement * element = netDevice->rxWithTimeout(netDevice, (configTICK_RATE_HZ / 2));

		if(element)
		{
			while (uip_buf)
			{
				struct netdeviceQueueElement * toReturn = getQueueElementByBuffer(uip_buf, uip_len);
				if (toReturn)
				{
					returnQueueElement(&toReturn);
					uip_buf = NULL;
				}
			}

			uip_len = getLengthFromQueueElement(element);
			uip_buf = removeDataFromQueueElement(&element);
			DEBUG_PRINT(DEBUG_BUFFER," received uip_buf 0x%x\r\n",uip_buf);
			if(uip_buf == NULL)
			{
				DEBUG_PRINT(DEBUG_ERR|DEBUG_BUFFER," %s no uip_buf 0x%x\r\n",__func__,uip_buf);
			}
		}

		if( ( uip_len > 0 ) && ( uip_buf != NULL ) )
		{
			/* Standard uIP loop taken from the uIP manual. */
			if( xHeader->type == htons( UIP_ETHTYPE_IP ) )
			{
				uip_arp_ipin();
				uip_input();

				/* If the above function invocation resulted in data that
				should be sent out on the network, the global variable
				uip_len is set to a value > 0. */
				if( uip_len > 0 )
				{
					uip_arp_out(); //ARP_ANSWER

					sendBuffer(netDevice);
				}
			}
			else if( xHeader->type == htons( UIP_ETHTYPE_ARP ) )
			{
				uip_arp_arpin();

				/* If the above function invocation resulted in data that
				should be sent out on the network, the global variable
				uip_len is set to a value > 0. */
				if( uip_len > 0 )
				{
					sendBuffer(netDevice);
				}
			}
			else
			{
				/* Mark the received buffer with unknown ethertype as 'handled' ... */
				uip_len = 0;
			}
		}
		/* Check the timer even if a buffer was received. sendBuffer() will exchange uip_buf. */
		{
			if( timer_expired( &periodic_timer ) && ( uip_buf != NULL ) )
			{
				timer_reset( &periodic_timer );
				for( i = 0; i < UIP_CONNS; i++ )
				{
					uip_periodic( i );

					/* If the above function invocation resulted in data that
					should be sent out on the network, the global variable
					uip_len is set to a value > 0. */
					if( uip_len > 0 )
					{
						uip_arp_out();//ARP BEFORE SEND

						sendBuffer(netDevice);
					}
				}

//#if TFTP_CLIENT_IAP == ON
			for(i = 0; i < UIP_UDP_CONNS; i++) {
				uip_udp_periodic(i);
				/* If the above function invocation resulted in data that
				   should be sent out on the network, the global variable
				   uip_len is set to a value > 0. */
				if(uip_len > 0) {
				  uip_arp_out();//UDP

					sendBuffer(netDevice);
				}
			}
//#endif /* UIP_UDP */

				/* Call the ARP timer function every 10 seconds. */
				if( timer_expired( &arp_timer ) )
				{
					timer_reset( &arp_timer );
					uip_arp_timer();
				}
			}
		}
	}
}
/*-----------------------------------------------------------*/

static void prvSetMACAddress( void )
{
#if UIP_FIXEDETHADDR
	/* uip_setethaddr() is defined to nothing when UIP_FIXEDETHADDR is defined */
#else
	struct uip_eth_addr xAddr;

	DEBUG_PRINT(DEBUG_INFO,"%s SET MAC\r\n",__func__);

    /* Configure the MAC address in the uIP stack. */
    xAddr.addr[ 0 ] = configMAC_ADDR0;
    xAddr.addr[ 1 ] = configMAC_ADDR1;
    xAddr.addr[ 2 ] = configMAC_ADDR2;
    xAddr.addr[ 3 ] = configMAC_ADDR3;
    xAddr.addr[ 4 ] = configMAC_ADDR4;
    xAddr.addr[ 5 ] = configMAC_ADDR5;
    uip_setethaddr( xAddr );
#endif
}
/*-----------------------------------------------------------*/
