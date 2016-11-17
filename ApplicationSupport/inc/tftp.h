/*
 * tftp.h
 *
 *  Created on: Feb 1, 2011
 *      Author: James Harwood
 *
 * This module is free software and there is NO WARRANTY.
 * No restrictions on use. You can use, modify and redistribute it for
   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
 */

#ifndef TFTP_H_
#define TFTP_H_

#undef UIP_UDP_APPCALL

#include "greenPhyModuleApplication.h"

#if TFTP_CLIENT_IAP == ON

#undef UIP_APPCALL
#define UIP_APPCALL()

#include "timer.h"
#include "pt.h"
#include "uip-conf.h"

#define TFTP_PORT 69
#define MAX_FNAME_LEN 128

#define TFTPC_SUCCESS			0
#define TFTPC_SERVER_DOWN		1
#define TFTPC_FILE_NOT_FOUND	2
#define TFTPC_ERROR				3
#define TFTPC_FLASH_ERROR		4

struct tftpc_state {
  struct pt pt;
  char state;
  struct uip_udp_conn *conn;
  struct timer timer;
  u16_t ticks;
  u16_t retries;
  u16_t blocknum;
  u16_t tftp_error;
  u16_t flash_error;
  u16_t datalen;
  u8_t duplicate;
  char fname[MAX_FNAME_LEN];
};

void tftpc_init(void);
u8_t tftpc_get(char *fname);
u8_t tftpc_busy();
u8_t tftpc_result();

void tftpc_appcall(void);

typedef struct tftpc_state uip_udp_appstate_t;

#define UIP_UDP_TFTP
#define UIP_UDP_APPCALL tftpc_appcall

#endif

#endif /* TFTP_H_ */
