/*====================================================================*
 *
 *   Copyright (c) 2011, 2012, Qualcomm Atheros Communications Inc.
 *
 *   Permission to use, copy, modify, and/or distribute this software
 *   for any purpose with or without fee is hereby granted, provided
 *   that the above copyright notice and this permission notice appear
 *   in all copies.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *   WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 *   THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 *   CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 *   LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 *   NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 *   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *--------------------------------------------------------------------*/

/*====================================================================*
 *
 *   qca_ar7k.h
 *
 *   Qualcomm Atheros SPI register definition.
 *
 *   This module is designed to define the Qualcomm Atheros SPI register
 *   placeholders;
 *
 *--------------------------------------------------------------------*/

#ifndef QCA_SPI_HEADER
#define QCA_SPI_HEADER

/*====================================================================*
 *   system header files;
 *--------------------------------------------------------------------*/
/* LPCOpen includes */
#include "board.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "NetworkBufferManagement.h"
#include "FreeRTOS_Routing.h"
#include "FreeRTOS_Bridge.h"


/*====================================================================*
 *   custom header files;
 *--------------------------------------------------------------------*/

/* QCA7k includes */
#include "qca_framing.h"

/*====================================================================*
 *   constants;
 *--------------------------------------------------------------------*/

#define QCASPI_GOOD_SIGNATURE 0xAA55

/* sync related constants */
#define QCASPI_SYNC_UNKNOWN    0
#define QCASPI_SYNC_CPUON      1
#define QCASPI_SYNC_READY      2
#define QCASPI_SYNC_RESET      3
#define QCASPI_SYNC_SOFT_RESET 4
#define QCASPI_SYNC_HARD_RESET 5
#define QCASPI_SYNC_WAIT_RESET 6
#define QCASPI_SYNC_UPDATE     7

#define QCASPI_RESET_TIMEOUT   10

/* Simple QoS related constants*/
/* Number of TX queues, equals number of CAPs */
#define QCAGP_NO_OF_QUEUES  4

/* default queue to use, e.g. if SimpleQoS is disabled */
#define QCAGP_DEFAULT_QUEUE  0

/* Task notification constants */
#define QCAGP_INT_FLAG (1<<0)
#define QCAGP_RX_FLAG  (1<<1) /* RX is passed as interrupt, too */
#define QCAGP_TX_FLAG  (1<<2)

/* Max amount of bytes read in one run */
#define QCASPI_BURST_LEN ( QCASPI_HW_BUF_LEN + 4 )

/*====================================================================*
 *   driver variables;
 *--------------------------------------------------------------------*/

/* RX and TX stats */
struct stats {
	uint32_t rx_errors;
	uint32_t rx_dropped;
	uint32_t rx_packets;
	uint32_t rx_bytes;
	uint32_t tx_errors;
	uint32_t tx_dropped;
	uint32_t tx_packets;
	uint32_t tx_bytes;
};

struct qcaspi {
	LPC_SSP_T* SSPx;
	uint8_t sync;

	/* the TX queue, simply a FreeRTOS queue */
	QueueHandle_t txQueue;
	NetworkBufferDescriptor_t *rx_desc;

	uint8_t rx_buffer[QCAFRM_TOTAL_HEADER_LEN];
	uint16_t rx_buffer_size;
	uint16_t rx_buffer_pos;
	uint16_t rx_buffer_len;
	QcaFrmHdl lFrmHdl;

	struct stats stats;
	NetworkInterface_t *pxInterface;
};

void qcaspi_spi_thread(void *data);

/*====================================================================*
 *
 *--------------------------------------------------------------------*/

#endif
