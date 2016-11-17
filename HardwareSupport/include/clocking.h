/*
 * clocking.h: Oscillator and PLL related functions for lpc17xx
 *
 * Copyright (C) 2011, MyVoice CAD/CAM Services
 * All Rights reserved.
 *
 * LPCXpresso users are granted unlimited use of this (and only this) piece of code.
 * Feel free to use it in any commercial or non-commercial application.
 * If you make any enhancements or bug fixes, feel free to inform me of this
 * (updates are highly appreciated) via email: rob@myvoice.nl
 *
 * History
 * 2011-07-03	v 1.00	Preliminary version, first release
 * 2011-07-08   v 1.10  Changed PLL values to save power on INTRC_OSC
 *                      Verified PLL values to result in correct frequencies
 *
 *
 * Clocking functions
 *
 * Software License Agreement
 *
 * The software is owned by MyVoice CAD/CAM Services and/or its suppliers, and is
 * protected under applicable copyright laws.  All rights are reserved.  Any
 * use in violation of the foregoing restrictions may subject the user to criminal
 * sanctions under applicable laws, as well as to civil liability for the breach
 * of the terms and conditions of this license.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 * USE OF THIS SOFTWARE FOR COMMERCIAL DEVELOPMENT AND/OR EDUCATION IS SUBJECT
 * TO A CURRENT END USER LICENSE AGREEMENT (COMMERCIAL OR EDUCATIONAL) WITH
 * MYVOICE CAD/CAM SERVICES.
 *
 */

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "portmacro.h"

/*
 * Can't get clocking from the RTC to work with debugger.
 * make sure that this is not compiled ...
 */
#undef LPC_CLOCK_FROM_RTC

#define LPC_INTRC_OSC 0
#define LPC_MAIN_OSC  1
#ifdef LPC_CLOCK_FROM_RTC
	#define LPC_RTC_OSC   2
#endif

/*
 * Set which oscillator to use
 */
int LPC_UseOscillator(uint8_t osc);
int LPC_SetPLL0(uint32_t cclk);
int LPC_EnablePLL1(void);
int LPC_DisablePLL1(void);
