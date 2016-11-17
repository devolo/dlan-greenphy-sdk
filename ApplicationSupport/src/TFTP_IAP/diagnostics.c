/*
 * diagnostics.c
 *
 *  Created on: Feb 4, 2011
 *      Author: James Harwood
 *
 * This module is free software and there is NO WARRANTY.
 * No restrictions on use. You can use, modify and redistribute it for
   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
 */

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include "diagnostics.h"
#include "clock.h"

#include "debug.h"

#define FLASH_TIME (CLOCK_SECOND/4)
#define PAUSE_TIME (CLOCK_SECOND*2)

struct diag_state {
  struct pt pt;
  struct timer timer;
  uint8_t num_flashes;
  uint8_t limit;
  uint8_t count;
};

static struct diag_state s;

void diag_init()
{
	PT_INIT(&s.pt);
	s.count = 0;
	s.limit = 0;
	s.num_flashes = 0;
}

void diag_set(uint8_t num)
{
	s.num_flashes = num;
	switch(s.num_flashes) {
	case 1:
		DEBUG_PRINT(DEBUG_INFO,"SUCCESS\r\n");
		break;
	case 2:
		DEBUG_PRINT(DEBUG_INFO,"SERVER_DOWN\r\n");
		break;
	case 3:
		DEBUG_PRINT(DEBUG_INFO,"FILE_NOT_FOUND\r\n");
		break;
	case 4:
		DEBUG_PRINT(DEBUG_INFO,"ERROR\r\n");
		break;
	case 5:
		DEBUG_PRINT(DEBUG_INFO,"FLASH_ERROR\r\n");
		break;
	case 6:
		DEBUG_PRINT(DEBUG_INFO,"INVALID_FILE\r\n");
		break;
	default:
		DEBUG_PRINT(DEBUG_INFO,"UNKNOWN_ERROR\r\n");
		break;
	}
}

static
PT_THREAD(handle_diag(void))
{
	PT_BEGIN(&s.pt);

	s.limit = s.num_flashes;

	if(s.limit > 0) {
		timer_set(&s.timer, PAUSE_TIME);
		PT_WAIT_UNTIL(&s.pt, timer_expired(&s.timer));
	}
	PT_END(&s.pt);
}


void diag_appcall()
{
	handle_diag();
}
