/*
 * flash.h
 *
 *  Created on: Feb 4, 2011
 *      Author: James Harwood
 *
 * This module is free software and there is NO WARRANTY.
 * No restrictions on use. You can use, modify and redistribute it for
   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
 */

#ifndef FLASH_H_
#define FLASH_H_

//#ifdef __USE_CMSIS
#include "LPC17xx.h"
//#endif

void flash_init();
uint8_t check_valid_code();
uint32_t flash_write_block(uint32_t *src);
void run_app();
void invalidate_code();

#endif /* FLASH_H_ */
