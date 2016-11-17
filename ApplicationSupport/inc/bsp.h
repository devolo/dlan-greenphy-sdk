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

#ifndef _BSP_H
#define _BSP_H

//#include "app_config.h"
//#include "lpc13xx.h"
#include "LPC17xx.h"
//#include "system_LPC13xx.h"
#include "system_LPC17xx.h"

/* Define the priorities of the supported interrupts */
/* value: 0 .. 3   (0 has highest prio) */

#define SYSTICK_IRQ_PRIORITY 3

#define LED_HEART_BEAT       0
#define LED_D4               1
#define LED_RTX_DALI_BUS     2
#define LED_DALI_BUS_BUSY    3

void bsp_init(void);
void bsp_set_led(uint8_t led, uint8_t state);
void bsp_get_sys_uptime(uint32_t *psec_cnt, uint16_t *pmsec_cnt);

#endif

