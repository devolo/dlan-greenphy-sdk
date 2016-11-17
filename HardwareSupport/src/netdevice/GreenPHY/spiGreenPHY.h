/*
 * Copyright (c) 2012, devolo AG, Aachen, Germany.
 * All rights reserved.
 *
 * This Software is part of the devolo GreenPHY-SDK.
 *
 * Usage in source form and redistribution in binary form, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Usage in source form is subject to a current end user license agreement
 *    with the devolo AG.
 * 2. Neither the name of the devolo AG nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 3. Redistribution in binary form is limited to the usage on the GreenPHY
 *    module of the devolo AG.
 * 4. Redistribution in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.

 spiGreenPHY.h
 */

#ifndef SPIGREENPHY_H_
#define SPIGREENPHY_H_

#include <lpc17xx_ssp.h>
#include <framingGreenPHY.h>
#include <lpc17xx_gpio.h>

#include <FreeRTOS.h>
#include <queue.h>

#include <buffer.h>

#include <greenPhyInterrupt.h>
#include <netdevice.h>

/*====================================================================*
 *   constants;
 *--------------------------------------------------------------------*/

/* sync related constants */
#define QCASPI_SYNC_UNKNOWN 0
#define QCASPI_SYNC_RESET   1
#define QCASPI_SYNC_READY   2
#define QCASPI_RESET_TIMEOUT 10

/* sync events */
#define QCASPI_SYNC_UPDATE 0
#define QCASPI_SYNC_CPUON  1

/* QCA7k */
#define QCA7K_SPI_READ (1 << 15)
#define QCA7K_SPI_WRITE (0 << 15)
#define QCA7K_SPI_INTERNAL (1 << 14)
#define QCA7K_SPI_EXTERNAL (0 << 14)
#define QCA7K_MAX_RESET_COUNT 2

/* QCA7k internals */
#define QCASPI_CMD_LEN 2
#define QCASPI_HW_PKT_LEN 4
#define QCASPI_HW_BUF_LEN 0xC5B

/* The used SSP */
#define GREEN_PHY_SSP_CHANNEL LPC_SSP0

/*====================================================================*
 *   SPI registers;
 *--------------------------------------------------------------------*/

#define	SPI_REG_DMA_SIZE        0x0100
#define SPI_REG_WRBUF_SPC_AVA   0x0200
#define SPI_REG_RDBUF_BYTE_AVA  0x0300
#define SPI_REG_SPI_CONFIG      0x0400
#define SPI_REG_SPI_STATUS      0x0500
#define SPI_REG_INTR_CAUSE      0x0C00
#define SPI_REG_INTR_ENABLE     0x0D00
#define SPI_REG_RDBUF_WATERMARK 0x1200
#define SPI_REG_WRBUF_WATERMARK 0x1300
#define SPI_REG_SIGNATURE       0x1A00
#define SPI_REG_ACTION_CTRL     0x1B00

/*====================================================================*
 *   SPI_CONFIG register definition;
 *--------------------------------------------------------------------*/

#define QCASPI_SLAVE_RESET_BIT (1 << 6)

/*====================================================================*
 *   INTR_CAUSE/ENABLE register definition.
 *--------------------------------------------------------------------*/

#define SPI_INT_WRBUF_BELOW_WM (1 << 10)
#define SPI_INT_CPU_ON         (1 << 6)
#define SPI_INT_ADDR_ERR       (1 << 3)
#define SPI_INT_WRBUF_ERR      (1 << 2)
#define SPI_INT_RDBUF_ERR      (1 << 1)
#define SPI_INT_PKT_AVLBL      (1 << 0)

/*====================================================================*
 *   ACTION_CTRL register definition.
 *--------------------------------------------------------------------*/

#define SPI_ACTRL_PKT_AVA_SPLIT_MODE (1 << 8)
#define SPI_ACTRL_PKT_AVA_INTR_MODE (1 << 0)

/*====================================================================*
 *   constants;
 *--------------------------------------------------------------------*/

/* defined in the data sheet of the QCA7k */
#define QCASPI_GOOD_SIGNATURE 0xAA55

/* sync related constants */
#define QCASPI_SYNC_UNKNOWN 0
#define QCASPI_SYNC_RESET   1
#define QCASPI_SYNC_READY   2
#define QCASPI_RESET_TIMEOUT 10

/* sync events */
#define QCASPI_SYNC_UPDATE 0
#define QCASPI_SYNC_CPUON  1

#define QCAGP_NO_RESOURCE_FOR_THIS_QUEUE  -2

/*====================================================================*
 *   driver variables;
 *--------------------------------------------------------------------*/

/* RX and TX stats */
struct stats {
	uint32_t rx_errors;
	uint32_t rx_dropped;
	uint32_t rx_packets;
	uint32_t rx_bytes;
	uint32_t tx_packets;
	uint32_t tx_bytes;
};

typedef enum driver_state_t {
    INITIALIZED,
    SW_VERSION_QUERY,
    SW_VER_QUERY_DONE,
    FIRMWARE_MODE_QUERY_SERIAL_OVER_ETH_VERSION,
    BOOTLOADER_MODE,
    SERIAL_OVER_ETH_VER0_MODE,
    SERIAL_OVER_ETH_VER1_MODE
} driver_state_t;

/* everything which is needed for LPC1758/QCA7k SPI communication */
struct qcaspi {
	/* the device interface, so the reset can be called by SPI routines*/
	struct netDeviceInterface * netdevice;

	/* everything which is needed to maintain SPI sync */
	struct read_from_chip_s rxStatus;

	/* some stats populated by the driver */
	struct stats stats;

	/* the TX queues, simply a FreeRTOS queue */
	//xQueueHandle txQueue;
	xQueueHandle tx_priority_q[QCAGP_NO_OF_QUEUES];

	/* the RX queue, simply a FreeRTOS queue */
	xQueueHandle rxQueue;

	/* QCA7k */
	qca7k_sync_t sync; /* Status of the driver, is it in sync with the QCA7k? */
	uint8_t syncGard;  /* boolean to indicate that syncGard is watching the Status of the QCA7k*/
	uint8_t syncGardTimeout;  /* number of seconds the guard waits to see the reset of the QCA7k*/
	uint8_t reset_count; /* number of reset frames send to the QCA7k */

	/* LPC17xx */
	SSP_CFG_Type sspChannelConfig;     /* SSP_ConfigStruct */
	SSP_DATA_SETUP_Type sspDataConfig; /* SPI Data configuration structure */
	LPC_SSP_TypeDef* SSPx;             /* SPI register structure*/

#if GREEN_PHY_SIMPLE_QOS == ON
	/* Simple QoS */
	driver_state_t driver_state;
	uint8_t driver_state_count;
	uint8_t target_mac_addr[6];
	//SIMPLE QOS related data structures
	uint8_t total_credits;
	uint8_t max_queue_size[QCAGP_NO_OF_QUEUES];
	uint16_t expected_mme_to_drop;
#endif
};

/*====================================================================*
 *   functions;
 *--------------------------------------------------------------------*/

/*********************************************************************//**
 * @brief 		Initialisation routine for the SPI communication
 * @param[in] 	qca		structure must be praeallocated
 * @param[in]	dev		pointer to the device interface
 * @return 		0 on success
 ***********************************************************************/
int greenPhyHandlerInit(struct qcaspi *qca, struct netDeviceInterface * dev);

/*********************************************************************//**
 * @brief 		Worker function handling QCA7k interrupt events
 * @param[in] 	data	void pointer to struct qcaspi
 * @return 		0 on success
 ***********************************************************************/
int greenPhySpiWorker(void *data);

/*********************************************************************//**
 * @brief 		To start the sync guard watching the QCA7K sync status
 * @param[in] 	data	pointer to struct qcaspi
 * @return 		NONE
 ***********************************************************************/
void startSyncGuard(struct qcaspi *qca);

#if GREEN_PHY_SIMPLE_QOS == ON
/*********************************************************************//**
 * @brief 		To select the correct tx queue
 * @param[in] 	buffer	pointer to netdevice element to send
 * @return 		qid
 ***********************************************************************/
uint32_t qcaspi_get_qid_from_eth_frame(struct netdeviceQueueElement * element);
#endif

#endif /* SPIGREENPHY_H_ */
