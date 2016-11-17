/*====================================================================*
 *
 *   Copyright (c) 2011, Atheros Communications Inc.
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
 */

/*====================================================================*
 *
 *   QcaFraming.h
 *
 *   Atheros Ethernet framing. Every Ethernet frame is surrounded by an atheros
 *   frame while transmitted over a serial channel.
 *
 *
 *--------------------------------------------------------------------*/

#ifndef _QCAFRAMING_H
#define _QCAFRAMING_H

/*====================================================================*
 *   system header files;
 *--------------------------------------------------------------------*/

#include <lpc_types.h>
#include <byteorder.h>
#include <buffer.h>

/*====================================================================*
 *   constants
 *--------------------------------------------------------------------*/
/* Number of total octets of an ethernet header. */
#define ETH_HLEN        14

/* Number of TX queues, equals number of CAPs */
#define QCAGP_NO_OF_QUEUES  4

/* default queue to use, e.g. if SimpleQoS is disabled */
#define QCAGP_DEFAULT_QUEUE  0

/* Frame is currently being received */
#define QCAFRM_GATHER 0

/*  No header byte while expecting it */
#define QCAFRM_NOHEAD (QCAFRM_ERR_BASE - 1)

/* No tailer byte while expecting it */
#define QCAFRM_NOTAIL (QCAFRM_ERR_BASE - 2)

/* Frame length is invalid */
#define QCAFRM_INVLEN (QCAFRM_ERR_BASE - 3)

/* Frame length is invalid */
#define QCAFRM_INVFRAME (QCAFRM_ERR_BASE - 4)

/* Min/Max Ethernet MTU */
#define QCAFRM_ETHMINMTU 46
#define QCAFRM_ETHMAXMTU 1500

/* Min/Max frame lengths */
#define QCAFRM_ETHMINLEN (QCAFRM_ETHMINMTU + ETH_HLEN)
#define QCAFRM_ETHMAXLEN (QCAFRM_ETHMAXMTU + ETH_HLEN)

/* QCA7K header len */
#define QCAFRM_HEADER_LEN 8

/* QCA7K buffer overhead for sending a frame */
#define QCAFRM_FRAME_OVERHEAD  ((QCAFRM_HEADER_LEN) + (QCAFRM_FOOTER_LEN))

/* QCA7K header signature */
#define QCAFRM_HEADER_SIGNATURE 	0xAA

/* QCA7K footer len */
#define QCAFRM_FOOTER_LEN 2

/* QCA7K tail signature */
#define QCAFRM_TAIL_SIGNATURE 		0x55

/* QCA7K QID */
#define QCAFRM_QID_LEN     2

#define QCAFRM_VERSION_MASK 	0xC0
#define QCAFRM_VERSION_SHIFT	6

/* QCA7K Framing. */
#define QCAFRM_ERR_BASE -1000

#define ETHER_TYPE_VLAN  0x8100

typedef  uint8_t qca7k_sync_t;
typedef enum {init, receiveHeader , receiveFrame, sync} receiveState_t;

struct read_from_chip_s;
typedef int (*qcaspi_process_result)(struct read_from_chip_s *rxStatus);

/* used to find and check SPI sync */
struct read_from_chip_s {
	/* number of bytes available in QCA7k */
	uint32_t lAvailable;
	/* length of currently read data */
	uint32_t length;
	/* frameLength holds the length of the received frame */
	uint32_t frameLength;
	/* number of bytes to read from QCA7k memory */
	uint32_t toRead;
	/* buffer to store the read data */
	data_t   greenPhyRxBuffer;
	/* to read new data to greenPhyRxBuffer*/
	data_t   current_position;
	/* state of sync */
	receiveState_t state;
	/* function used to process the current state */
	qcaspi_process_result process_result;
#ifdef DEBUG
	/* number of consecutively received valid frames */
	uint32_t consecutivelyReceivedValidFrames;
#endif

	/* Protocol version of this frame */
	uint32_t protocol_version;

	/* Queue sizes in the PLC devices as reported through this frame */
	uint8_t  queue_sizes[QCAGP_NO_OF_QUEUES];
};

/*====================================================================*
 *
 *   QcaSetRxStatus
 *
 *   Sets the state to rxStatus.
 *
 *   Return: 0 on success.
 *
 *--------------------------------------------------------------------*/

int QcaSetRxStatus(struct read_from_chip_s *rxStatus, receiveState_t state);


/*====================================================================*
 *
 *   QcaFrmCreateHeader
 *
 *   Fills a provided buffer with the QCA7K frame header.
 *
 *   Return: The number of bytes written in header.
 *
 *--------------------------------------------------------------------*/

int32_t QcaFrmCreateHeader(uint8_t *buf, uint16_t len, uint32_t protocol_version);

/*====================================================================*
 *
 *   QcaFrmCreateFooter
 *
 *   Fills a provided buffer with the QCA7K frame footer.
 *
 *   Return: The number of bytes written in footer.
 *
 *--------------------------------------------------------------------*/

int32_t QcaFrmCreateFooter(uint8_t *buf);

/*====================================================================*
 *
 *   QcaFrmAddQID
 *
 *   Fills a provided buffer with the Host Queue ID
 *
 * Return:   The number of bytes written in footer.
 *
 *--------------------------------------------------------------------*/

int32_t QcaFrmAddQID(uint8_t *buf, uint8_t qid);

/*====================================================================*
 *
 *   QcaGetSpiProtocolVersion
 *
 *   Gets the SPI protocol version of the received frame.
 *
 * Return:   0 on success.
 *
 *--------------------------------------------------------------------*/
uint32_t  QcaGetSpiProtocolVersion(struct read_from_chip_s *rxStatus);

#endif
