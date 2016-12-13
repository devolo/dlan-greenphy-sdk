/*
 * emacLPC17xx.h
 *
 *  Created on: 23.04.2012
 */

#ifndef EMACLPC17XX_H_
#define EMACLPC17XX_H_

/* Hardware specific includes. */
#include "EthDev_LPC17xx.h"
#include <lpc17xx_gpio.h>

/* Time to wait between each inspection of the link status. */
#define emacWAIT_FOR_LINK_TO_ESTABLISH ( 50 / portTICK_RATE_MS )

/* Short delay used in several places during the initialisation process. */
#define emacSHORT_DELAY				   ( 2 )

/* Hardware specific bit definitions. */
#define emacLINK_ESTABLISHED		( 0x0001 )
#define emacFULL_DUPLEX_ENABLED		( 0x0004 )
#define emac10BASE_T_MODE			( 0x0002 )
#define emacPINSEL2_VALUE 0x50150105


/* Index to the Tx descriptor that is always used first for every Tx.  The second
descriptor is then used to re-send in order to speed up the uIP Tx process. */
static unsigned long emacTX_DESC_INDEX = 0;

#define PCONP_PCENET    0x40000000
/*-----------------------------------------------------------*/

// for LPC1758, use the MDIO WORKAROUND
#define LPC1758_MDIO_PORT	LPC_GPIO1
#define MDIO    			(1<<(23))
#define MDC     			(1<<(20))

#define ETH_PHY_RESET_PORT 1
#define ETH_PHY_RESET_PIN  24

#define ETH_PHY_INTERRUPT_PORT 2
#define ETH_PHY_INTERRUPT_PIN 8

#define ETH_PHY_INTERRUPT_ID			(EINT3_IRQn)
#define ETH_PHY_INTERRUPT_PRIORITY 	(configEMAC_INTERRUPT_PRIORITY)

/*
 * Configure both the Rx and Tx descriptors during the init process.
 */
static void prvInitDescriptors( void );

/*
 * Setup the IO and peripherals required for Ethernet communication.
 */
static void prvSetupEMACHardware( void );

/*
 * Send lValue to the lPhyReg within the PHY.
 */
static long prvWritePHY( long lPhyReg, long lValue );

/*
 * Read a value from ucPhyReg within the PHY.  *plStatus will be set to
 * pdFALSE if there is an error.
 */
static unsigned short prvReadPHY( unsigned char ucPhyReg, long *plStatus );


#endif /* EMACLPC17XX_H_ */
