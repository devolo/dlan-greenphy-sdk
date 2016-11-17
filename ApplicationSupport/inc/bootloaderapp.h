/*
 * bootloaderapp.h
 *
 *  Created on: Feb 5, 2011
 *      Author: James Harwood
 *
 * This module is free software and there is NO WARRANTY.
 * No restrictions on use. You can use, modify and redistribute it for
   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
 */

#ifndef BOOTLOADERAPP_H_
#define BOOTLOADERAPP_H_

void bl_init();
void bl_appcall();
void diag_appcall();

void startApplication();

#endif /* BOOTLOADERAPP_H_ */
