/*
 * config.h
 *
 *  Created on: Feb 4, 2011
 *      Author: James Harwood
 *
 * This module is free software and there is NO WARRANTY.
 * No restrictions on use. You can use, modify and redistribute it for
   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include "netConfig.h"

#define FIRMWARE_FILENAME "greenphy.dvl"

#define NUMBER_OF_RETRIES 3

// controller IP address
#define MYIP1 configIP_ADDR0
#define MYIP2 configIP_ADDR1
#define MYIP3 configIP_ADDR2
#define MYIP4 configIP_ADDR3

// Subnet Mask
#define SMSK1 configNET_MASK0
#define SMSK2 configNET_MASK1
#define SMSK3 configNET_MASK2
#define SMSK4 configNET_MASK3

// default router IP address
#define DRTR1 configDRTR_IP_ADDR0
#define DRTR2 configDRTR_IP_ADDR1
#define DRTR3 configDRTR_IP_ADDR2
#define DRTR4 configDRTR_IP_ADDR3

// TFTP server address
#define TIP1 configTFTP_IP_ADDR0
#define TIP2 configTFTP_IP_ADDR1
#define TIP3 configTFTP_IP_ADDR2
#define TIP4 configTFTP_IP_ADDR3

#endif /* CONFIG_H_ */
