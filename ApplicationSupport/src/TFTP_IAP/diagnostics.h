/*
 * diagnostics.h
 *
 *  Created on: Feb 4, 2011
 *      Author: James Harwood
 *
 * This module is free software and there is NO WARRANTY.
 * No restrictions on use. You can use, modify and redistribute it for
   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
 */

#ifndef DIAGNOSTICS_H_
#define DIAGNOSTICS_H_

#include "timer.h"
#include "pt.h"

void diag_init();
void diag_set(uint8_t num);
void diag_appcall();

#endif /* DIAGNOSTICS_H_ */
