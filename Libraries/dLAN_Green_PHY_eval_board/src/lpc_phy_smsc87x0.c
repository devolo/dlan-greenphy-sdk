/*
 * @brief SMSC 87x0 simple PHY driver
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2012
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#include "chip.h"
#include "lpc_phy.h"

/** @defgroup SMSC87X0_PHY BOARD: PHY status and control driver for the SMSC 87x0
 * @ingroup BOARD_PHY
 * Various functions for controlling and monitoring the status of the
 * SMSC 87x0 PHY.
 * @{
 */

/* LAN8720 PHY register offsets */
#define LAN8_BCR_REG        0x0	/*!< Basic Control Register */
#define LAN8_BSR_REG        0x1	/*!< Basic Status Reg */
#define LAN8_PHYID1_REG     0x2	/*!< PHY ID 1 Reg  */
#define LAN8_PHYID2_REG     0x3	/*!< PHY ID 2 Reg */
#define LAN8_PHYSPLCTL_REG  0x1F/*!< PHY special control/status Reg */

/* LAN8720 BCR register definitions */
#define LAN8_RESET          (1 << 15)	/*!< 1= S/W Reset */
#define LAN8_LOOPBACK       (1 << 14)	/*!< 1=loopback Enabled */
#define LAN8_SPEED_SELECT   (1 << 13)	/*!< 1=Select 100MBps */
#define LAN8_AUTONEG        (1 << 12)	/*!< 1=Enable auto-negotiation */
#define LAN8_POWER_DOWN     (1 << 11)	/*!< 1=Power down PHY */
#define LAN8_ISOLATE        (1 << 10)	/*!< 1=Isolate PHY */
#define LAN8_RESTART_AUTONEG (1 << 9)	/*!< 1=Restart auto-negoatiation */
#define LAN8_DUPLEX_MODE    (1 << 8)	/*!< 1=Full duplex mode */

/* LAN8720 BSR register definitions */
#define LAN8_100BASE_T4     (1 << 15)	/*!< T4 mode */
#define LAN8_100BASE_TX_FD  (1 << 14)	/*!< 100MBps full duplex */
#define LAN8_100BASE_TX_HD  (1 << 13)	/*!< 100MBps half duplex */
#define LAN8_10BASE_T_FD    (1 << 12)	/*!< 100Bps full duplex */
#define LAN8_10BASE_T_HD    (1 << 11)	/*!< 10MBps half duplex */
#define LAN8_AUTONEG_COMP   (1 << 5)	/*!< Auto-negotation complete */
#define LAN8_RMT_FAULT      (1 << 4)	/*!< Fault */
#define LAN8_AUTONEG_ABILITY (1 << 3)	/*!< Auto-negotation supported */
#define LAN8_LINK_STATUS    (1 << 2)	/*!< 1=Link active */
#define LAN8_JABBER_DETECT  (1 << 1)	/*!< Jabber detect */
#define LAN8_EXTEND_CAPAB   (1 << 0)	/*!< Supports extended capabilities */

/* LAN8720 PHYSPLCTL status definitions */
#define LAN8_SPEEDMASK      (7 << 2)	/*!< Speed and duplex mask */
#define LAN8_SPEED100F      (6 << 2)	/*!< 100BT full duplex */
#define LAN8_SPEED10F       (5 << 2)	/*!< 10BT full duplex */
#define LAN8_SPEED100H      (2 << 2)	/*!< 100BT half duplex */
#define LAN8_SPEED10H       (1 << 2)	/*!< 10BT half duplex */

/* LAN8720 PHY ID 1/2 register definitions */
#define LAN8_PHYID1_OUI     0x0007		/*!< Expected PHY ID1 */
#define LAN8_PHYID2_OUI     0xC0F0		/*!< Expected PHY ID2, except last 4 bits */

/* DP83848 PHY update flags */
static uint32_t physts, olddphysts;

/* Pointer to delay function used for this driver */
static p_msDelay_func_t pDelayMs;

/*
 * Apply NXP AN10859
 * LPC1700 Ethernet MII Management (MDIO) via software
 */

/*--------------------------- output_MDIO ------------------------------------*/

static void output_MDIO (unsigned int val, unsigned int n) {
	/* Output a value to the MII PHY management interface. */
	for (val <<= (32 - n); n; val <<= 1, n--) {
		if (val & 0x80000000) {
			Chip_GPIO_SetPinOutHigh(LPC_GPIO, MDIO_GPIO_PORT_NUM, MDIO_GPIO_BIT_NUM);
		}
		else {
			Chip_GPIO_SetPinOutLow(LPC_GPIO, MDIO_GPIO_PORT_NUM, MDIO_GPIO_BIT_NUM);
		}
		Chip_GPIO_SetPinOutHigh(LPC_GPIO, MDC_GPIO_PORT_NUM, MDC_GPIO_BIT_NUM);
		Chip_GPIO_SetPinOutLow(LPC_GPIO, MDC_GPIO_PORT_NUM, MDC_GPIO_BIT_NUM);
	}
}

/*--------------------------- input_MDIO ------------------------------------*/

static unsigned int input_MDIO (void) {
	/* Input a value from the MII PHY management interface. */
	unsigned int i,val = 0;

	for (i = 0; i < 16; i++) {
		val <<= 1;

		Chip_GPIO_SetPinOutHigh(LPC_GPIO, MDC_GPIO_PORT_NUM, MDC_GPIO_BIT_NUM);
		Chip_GPIO_SetPinOutLow(LPC_GPIO, MDC_GPIO_PORT_NUM, MDC_GPIO_BIT_NUM);
		if (Chip_GPIO_GetPinState(LPC_GPIO, MDIO_GPIO_PORT_NUM, MDIO_GPIO_BIT_NUM)){
			val |= 1;
		}
	}
	return (val);
}

/*--------------------------- turnaround_MDIO -------------------------------*/

static void turnaround_MDIO (void) {
	/* Turnaround MDO is tristated. */
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, MDIO_GPIO_PORT_NUM, MDIO_GPIO_BIT_NUM);
	Chip_GPIO_SetPinOutHigh(LPC_GPIO, MDC_GPIO_PORT_NUM, MDC_GPIO_BIT_NUM);
	Chip_GPIO_SetPinOutLow(LPC_GPIO, MDC_GPIO_PORT_NUM, MDC_GPIO_BIT_NUM);
}

/*--------------------------- prvWritePHY ------------------------------------*/

static long prvWritePHY( long lPhyReg, long lValue )
{
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, MDIO_GPIO_PORT_NUM, MDIO_GPIO_BIT_NUM);

   /* 32 consecutive ones on MDO to establish sync */
   output_MDIO (0xFFFFFFFF, 32);

   /* start code (01), write command (01) */
   output_MDIO (0x05, 4);

   /* write PHY address */
   output_MDIO ( 0x0100/*Default PHY device address*/ >> 8, 5);

   /* write the PHY register to write */
   output_MDIO (lPhyReg, 5);

   /* turnaround MDIO (1,0)*/
   output_MDIO (0x02, 2);

   /* write the data value */
   output_MDIO (lValue, 16);

   /* turnaround MDO is tristated */
   turnaround_MDIO ();

   return SUCCESS;
}

/*--------------------------- prvReadPHY ------------------------------------*/

static unsigned short prvReadPHY( unsigned char ucPhyReg)
{
	unsigned int val;

	/* Software MII Management for LPC175x. */
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, MDIO_GPIO_PORT_NUM, MDIO_GPIO_BIT_NUM);

	/* 32 consecutive ones on MDO to establish sync */
	output_MDIO (0xFFFFFFFF, 32);

	/* start code (01), read command (10) */
	output_MDIO (0x06, 4);

	/* write PHY address */
	output_MDIO (0x0100/*Default PHY device address*/ >> 8, 5);

	/* write the PHY register to write */
	output_MDIO (ucPhyReg, 5);

	/* turnaround MDO is tristated */
	turnaround_MDIO ();

	/* read the data value */
	val = input_MDIO ();

	/* turnaround MDIO is tristated */
	turnaround_MDIO ();

	return (val);
}
/*-----------------------------------------------------------*/

/* Update PHY status from passed value */
static void smsc_update_phy_sts(uint16_t linksts, uint16_t sdsts)
{
	/* Update link active status */
	if (linksts & LAN8_LINK_STATUS) {
		physts |= PHY_LINK_CONNECTED;
	}
	else {
		physts &= ~PHY_LINK_CONNECTED;
	}

	switch (sdsts & LAN8_SPEEDMASK) {
	case LAN8_SPEED100F:
	default:
		physts |= PHY_LINK_SPEED100;
		physts |= PHY_LINK_FULLDUPLX;
		break;

	case LAN8_SPEED10F:
		physts &= ~PHY_LINK_SPEED100;
		physts |= PHY_LINK_FULLDUPLX;
		break;

	case LAN8_SPEED100H:
		physts |= PHY_LINK_SPEED100;
		physts &= ~PHY_LINK_FULLDUPLX;
		break;

	case LAN8_SPEED10H:
		physts &= ~PHY_LINK_SPEED100;
		physts &= ~PHY_LINK_FULLDUPLX;
		break;
	}

	/* If the status has changed, indicate via change flag */
	if ((physts & (PHY_LINK_SPEED100 | PHY_LINK_FULLDUPLX | PHY_LINK_CONNECTED)) !=
		(olddphysts & (PHY_LINK_SPEED100 | PHY_LINK_FULLDUPLX | PHY_LINK_CONNECTED))) {
		olddphysts = physts;
		physts |= PHY_LINK_CHANGED;
	}
}

/* Initialize the SMSC 87x0 PHY */
uint32_t lpc_phy_init(bool rmii, p_msDelay_func_t pDelayMsFunc)
{
	uint16_t tmp;
	int32_t i;

	pDelayMs = pDelayMsFunc;

	/* Initial states for PHY status */
	olddphysts = physts = 0;

	Chip_GPIO_SetPinDIROutput(LPC_GPIO, MDC_GPIO_PORT_NUM, MDC_GPIO_BIT_NUM);

	/* Only first read and write are checked for failure */
	/* Put the DP83848C in reset mode and wait for completion */
	prvWritePHY(LAN8_BCR_REG, LAN8_RESET);
	i = 400;
	while (i > 0) {
		pDelayMs(1);
		tmp = prvReadPHY(LAN8_BCR_REG);

		if (!(tmp & (LAN8_RESET | LAN8_POWER_DOWN))) {
			i = -1;
		}
		else {
			i--;
		}
	}
	/* Timeout? */
	if (i == 0) {
		return ERROR;
	}

	/* Setup link */
	prvWritePHY(LAN8_BCR_REG, LAN8_AUTONEG);

	/* The link is not set active at this point, but will be detected
	   later */

	return SUCCESS;
}

/* Phy status update; state machine not needed, as registers are read
 * with software MDIO in blocking mode */
uint32_t lpcPHYStsPoll(void)
{
uint16_t linksts, sdsts;

    /* clear link changed flag */
	physts &= ~PHY_LINK_CHANGED;

	/* read link and speed status */
	linksts = prvReadPHY(LAN8_BSR_REG);
	sdsts = prvReadPHY(LAN8_PHYSPLCTL_REG);

	/* update physts */
	smsc_update_phy_sts(linksts, sdsts);
	return physts;
}
