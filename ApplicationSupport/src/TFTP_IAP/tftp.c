/*
 * tftp.c
 *
 *  Created on: Feb 2, 2011
 *      Author: James Harwood
 *
 * This module is free software and there is NO WARRANTY.
 * No restrictions on use. You can use, modify and redistribute it for
   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
 */


#include "greenPhyModuleApplication.h"

#if TFTP_CLIENT_IAP == ON
#include "config.h"
#include "tftp.h"
#include "uip.h"
#include "flash.h"
#include <string.h>

#define STATE_CLOSED     	0
#define STATE_CONNECTING 	1
#define STATE_SENDING    	2
#define STATE_DATA			3
#define STATE_ACKED			4
#define STATE_ERROR			5

#define OP_RRQ		1
#define OP_WRQ		2
#define OP_DATA		3
#define OP_ACK		4
#define OP_ERROR	5



#define RW_MODE		"octet"

typedef struct _tftp_msg {
	u16_t opcode;
	union {
		struct _req {
			char file_mode[256];
		} request;

		struct _data {
			u16_t blocknum;
			u8_t data[512];
		} data;

		struct _ack {
			u16_t blocknum;
		} ack;

		struct _error {
			u16_t error_code;
			char error_msg[256];
		} error;

	} packet;
} tftp_msg_t;

static struct tftpc_state s;

static u8_t busy_flag;

u8_t tftpc_busy()
{
	return busy_flag;
}

static u8_t result_code;

u8_t tftpc_result()
{
	return result_code;
}

void tftpc_init(void)
{
	s.state = STATE_CLOSED;
	busy_flag = 0;
	PT_INIT(&s.pt);
	s.blocknum = 0;
	s.datalen = 0;
	s.duplicate = 0;
	s.flash_error = 0;
	s.tftp_error = 0;

}

/*---------------------------------------------------------------------------*/
#define BUF_SIZE (512)
static u8_t block_buf[BUF_SIZE] __attribute__ ((aligned (4)));

static
void write_block()
{
	u16_t i;
	u8_t *data_ptr;
	tftp_msg_t *msg;
	msg = (tftp_msg_t *)uip_appdata;
	data_ptr = msg->packet.data.data;

	for(i = 0; i < s.datalen-4; i++) {
		block_buf[i] = data_ptr[i];
	}

	if(flash_write_block((uint32_t *)block_buf) != 0) {
		s.flash_error ++;
	}
}

/*---------------------------------------------------------------------------*/

u8_t tftpc_get(char *fname)
{
	uip_ipaddr_t addr;

	if(s.state != STATE_CLOSED) {
		return -1;  // client is busy
	}
	strncpy(s.fname, fname, MAX_FNAME_LEN-1);
	uip_ipaddr(&addr, TIP1, TIP2, TIP3, TIP4);
	s.conn = uip_udp_new(&addr, HTONS(TFTP_PORT));

	if( s.conn == NULL) {
		return -2;  // no available connections
	} else {
		s.state = STATE_CONNECTING;
		busy_flag = 1;
		return 0;
	}
}
/*---------------------------------------------------------------------------*/

static
void send_ack()
{
	tftp_msg_t *msg;
	msg = (tftp_msg_t *)uip_appdata;
	msg->opcode = HTONS(OP_ACK);
	msg->packet.ack.blocknum = HTONS(s.blocknum);
	uip_send(uip_appdata, 4);
}
/*---------------------------------------------------------------------------*/

static
void send_request()
{
	tftp_msg_t *msg;
	char *pdata;
	u16_t i, datalen;
	static char mode[] = RW_MODE;

	msg = (tftp_msg_t *)uip_appdata;
	pdata = (char *)msg->packet.request.file_mode;

	msg->opcode = HTONS(OP_RRQ);
	datalen = 2;
	for(i = 0; i < strlen(s.fname); i++) {
		*pdata++ = s.fname[i];
		datalen++;
	}
	*pdata++ = 0;
	datalen++;

	for(i = 0; i < strlen(mode); i++) {
		*pdata++ = mode[i];
		datalen++;
	}
	*pdata++ = 0;
	datalen++;

	uip_send(uip_appdata, datalen);
}
/*---------------------------------------------------------------------------*/
static void
parse_msg(void)
{
  tftp_msg_t *m = (tftp_msg_t *)uip_appdata;
  u16_t blocknum;

  switch(HTONS(m->opcode)) {
  case OP_DATA:
	  blocknum = HTONS(m->packet.data.blocknum);
	  s.duplicate = (s.blocknum == blocknum ? 1 : 0);
	  s.blocknum = blocknum;
	  s.datalen = uip_datalen();
	  s.state = STATE_DATA;
	  break;

  case OP_ACK:
	  s.blocknum = HTONS(m->packet.ack.blocknum);
	  s.state = STATE_ACKED;
	  break;

  case OP_ERROR:
	  s.tftp_error = HTONS(m->packet.error.error_code);
	  s.state = STATE_ERROR;
	  break;

  }
}

static
PT_THREAD(handle_tftp(void))
{
  PT_BEGIN(&s.pt);
  s.state = STATE_SENDING;

  s.ticks = CLOCK_SECOND/3;
#ifdef DEBUG
  /*debugging will slow down the code, so increase the timeout  */
  s.ticks = s.ticks * 3;
#endif
  s.retries = NUMBER_OF_RETRIES;

  do {
	  send_request();
	  timer_set(&s.timer, s.ticks);
	  PT_WAIT_UNTIL(&s.pt, uip_newdata() || timer_expired(&s.timer));
	  if(uip_newdata()) {
		  parse_msg();
		  break;
	  }

  } while (--s.retries > 0);

  if(s.state == STATE_DATA) {
	  // do something with the first block of data
	  write_block();
	  while(s.datalen == 516) {
		  s.state = STATE_SENDING;
		  s.retries = 3;
		  do {
			  send_ack();
			  timer_set(&s.timer, s.ticks);
			  PT_YIELD_UNTIL(&s.pt, uip_newdata() || timer_expired(&s.timer));
			  if(uip_newdata()) {
				  parse_msg();
				  break;
			  }
		  } while(--s.retries > 0);

		  if(s.state == STATE_DATA) {
			  if(!s.duplicate) {
				  // do something with next block of data
				  write_block();
			  }
		  } else {
			  // error or timeout
			  uip_close();
			  uip_udp_remove(s.conn);
			  s.state = STATE_CLOSED;
			  busy_flag = 0;
			  result_code = TFTPC_ERROR;
			  PT_RESTART(&s.pt);
		  }

	  }
	  // send final ack
	  send_ack();
	  timer_set(&s.timer, s.ticks);
	  PT_WAIT_UNTIL(&s.pt, timer_expired(&s.timer));
	  uip_close();
	  uip_udp_remove(s.conn);
	  s.state = STATE_CLOSED;
	  if(s.flash_error > 0) {
		  result_code = TFTPC_FLASH_ERROR;
	  } else {
		  result_code = TFTPC_SUCCESS;
	  }
	  busy_flag = 0;
  } else if(s.state == STATE_ACKED) {
	  // only for write operations

  } else {
	  // error, or server down
	  uip_abort();
	  uip_udp_remove(s.conn);
	  s.state = STATE_CLOSED;
	  if(s.tftp_error == 1) {
		  result_code = TFTPC_FILE_NOT_FOUND;
	  } else {
		  result_code = TFTPC_SERVER_DOWN;
	  }
	  busy_flag = 0;
  }

  PT_END(&s.pt);
}

void tftpc_appcall(void)
{
	handle_tftp();
}

#endif
