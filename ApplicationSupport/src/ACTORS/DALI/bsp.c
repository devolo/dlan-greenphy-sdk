/******************************************************************************/
/*  Copyright (c) 2013 NXP B.V.  All rights are reserved.                     */
/*  Reproduction in whole or in part is prohibited without the prior          */
/*  written consent of the copyright owner.                                   */
/*                                                                            */
/*  This software and any compilation or derivative thereof is, and           */
/*  shall remain the proprietary information of NXP and is                    */
/*  highly confidential in nature. Any and all use hereof is restricted       */
/*  and is subject to the terms and conditions set forth in the               */
/*  software license agreement concluded with NXP B.V.                        */
/*                                                                            */
/*  Under no circumstances is this software or any derivative thereof         */
/*  to be combined with any Open Source Software, exposed to, or in any       */
/*  way licensed under any Open License Terms without the express prior       */
/*  written permission of the copyright owner.                                */
/*                                                                            */
/*  For the purpose of the above, the term Open Source Software means         */
/*  any software that is licensed under Open License Terms. Open              */
/*  License Terms means terms in any license that require as a                */
/*  condition of use, modification and/or distribution of a work              */
/*                                                                            */
/*  1. the making available of source code or other materials                 */
/*     preferred for modification, or                                         */
/*                                                                            */
/*  2. the granting of permission for creating derivative                     */
/*     works, or                                                              */
/*                                                                            */
/*  3. the reproduction of certain notices or license terms                   */
/*     in derivative works or accompanying documentation, or                  */
/*                                                                            */
/*  4. the granting of a royalty-free license to any party                    */
/*     under Intellectual Property Rights                                     */
/*                                                                            */
/*  regarding the work and/or any work that contains, is combined with,       */
/*  requires or otherwise is based on the work.                               */
/*                                                                            */
/*  This software is provided for ease of recompilation only.                 */
/*  Modification and reverse engineering of this software are strictly        */
/*  prohibited.                                                               */
/*                                                                            */
/******************************************************************************/

/*******************************************************************************
 * standard include files
 ******************************************************************************/

#include <stdbool.h>

/*******************************************************************************
 * project specific include files
 ******************************************************************************/

#include "bsp.h"

/*******************************************************************************
 * type definitions
 ******************************************************************************/

#define LPC_CORE_CLOCKSPEED_HZ   ( 48000000 )
#define SYS_TICK_RATE_HZ       100

#define EN_GPIO       (1 <<  6)
#define EN_TIMER32_0  (1 <<  9)
#define EN_TIMER32_1  (1 << 10)
#define EN_USBREG     (1 << 14)
#define EN_IOCON      (1 << 16)

static uint8_t hb_led_state;
static volatile uint32_t sys_seconds_cnt;
static volatile uint16_t sys_millisec_cnt;

/*******************************************************************************
 * global functions
 ******************************************************************************/

//void TickHandler(void)
//TIMER1_IRQHandler
void TIMER1_IRQHandler(void) {
	uint32_t millisec_increment = 1000 / SYS_TICK_RATE_HZ;

	sys_millisec_cnt += millisec_increment;
	if (sys_millisec_cnt >= 1000) {
		sys_millisec_cnt -= 1000;
		sys_seconds_cnt++;
		//hb_led_state ^= 1;
		//bsp_set_led(LED_HEART_BEAT, hb_led_state);
	}
}

void bsp_init(void) {
	uint32_t timerFreq;
	/* Init global variables */
	hb_led_state = 0;
	sys_seconds_cnt = 0;
	sys_millisec_cnt = 0;

	/* Setup the system tick timer */
	//NVIC_SetPriority(SysTick_IRQn, SYSTICK_IRQ_PRIORITY);
	//SysTick_Config(LPC_CORE_CLOCKSPEED_HZ / SYS_TICK_RATE_HZ);
	/*Timer3*/

	LPC_SC->PCONP |= 1 << 1; //Power up Timer0
	//LPC_SC->PCONP |= 1 << 2; //Power up Timer1
	//LPC_SC->PCLKSEL0 |= 1 << 5; // Clock for timer = CCLK/2
	//LPC_TIM1->PR = 480000; //((SystemCoreClock/1000000) - 1); // timer runs at 1 MHz - 1usec per tick
	//NVIC_ClearPendingIRQ(TIMER3_IRQn);
	//NVIC_EnableIRQ(TIMER3_IRQn);
	//LPC_TIM1->TC = 0;
	//LPC_TIM1->MR0 = 24;
	//LPC_TIM1->CCR = 0;
	//LPC_TIM1->MCR = 3;
	//NVIC_EnableIRQ(TIMER1_IRQn); // Enable timer interrupt
	//LPC_TIM1->TCR = 1; // Reset Timer0

	//LPC_TIM3->TCR |= 1 << 0; // Start timer

	/* evtl USB?
	 LPC_SYSCON->SYSAHBCLKCTRL |= (EN_GPIO |
	 EN_TIMER32_0 |
	 EN_TIMER32_1 |
	 EN_USBREG |
	 EN_IOCON);


	 // default function on port 3_0 is GPIO (D1 LED)
	 LPC_GPIO3->DIR |= (1 << 0);     // set as output
	 // default function on port 3_1 is GPIO (D4 LED)
	 LPC_GPIO3->DIR |= (1 << 1);     // set as output
	 // default function on port 3_2 is GPIO (D5 LED)
	 LPC_GPIO3->DIR |= (1 << 2);	// set as output
	 // default function on port 3_3 is GPIO (D6 LED)
	 LPC_GPIO3->DIR |= (1 << 3);     // set as output
	 */

	// ALL LEDs OFF
	//bsp_set_led(LED_HEART_BEAT, 0);
	//bsp_set_led(LED_D4, 0);
	//bsp_set_led(LED_RTX_DALI_BUS, 0);
	//bsp_set_led(LED_DALI_BUS_BUSY, 0);
}

/*void bsp_set_led(uint8_t led, uint8_t state)
 {
 if (state)
 {   // LED ON
 switch (led)
 {
 case LED_HEART_BEAT:    LPC_GPIO3->DATA |= (1 << 0); break;
 case LED_D4:            LPC_GPIO3->DATA |= (1 << 1); break;
 case LED_RTX_DALI_BUS:  LPC_GPIO3->DATA |= (1 << 2); break;
 case LED_DALI_BUS_BUSY: LPC_GPIO3->DATA |= (1 << 3); break;
 }
 }
 else
 {   // LED OFF
 switch (led)
 {
 case LED_HEART_BEAT:    LPC_GPIO3->DATA &= ~(1 << 0); break;
 case LED_D4:            LPC_GPIO3->DATA &= ~(1 << 1); break;
 case LED_RTX_DALI_BUS:  LPC_GPIO3->DATA &= ~(1 << 2); break;
 case LED_DALI_BUS_BUSY: LPC_GPIO3->DATA &= ~(1 << 3); break;
 }
 }
 }*/

void bsp_get_sys_uptime(uint32_t *psec_cnt, uint16_t *pmsec_cnt) {
	// we do not want to disable the systick interrupt, so we have to deal
	// with a systick during the execution of this function
	uint32_t sec_cnt_1 = sys_seconds_cnt;
	uint16_t msec_cnt_1 = sys_millisec_cnt;
	uint32_t sec_cnt_2 = sys_seconds_cnt;
	uint16_t msec_cnt_2 = sys_millisec_cnt;
	if ((sec_cnt_1 == sec_cnt_2) && (msec_cnt_2 < msec_cnt_1)) {
		sec_cnt_2 += 1;
	}
	*psec_cnt = sec_cnt_2;
	*pmsec_cnt = msec_cnt_2;
}

/* End of file */
