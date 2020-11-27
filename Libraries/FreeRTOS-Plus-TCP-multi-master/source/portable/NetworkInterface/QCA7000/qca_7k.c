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
 *   qca_7k.c	-
 *
 *   This module implements the Qualcomm Atheros SPI protocol for
 *   kernel-based SPI device.
 *
 *--------------------------------------------------------------------*/

/*====================================================================*
 *   system header files;
 *--------------------------------------------------------------------*/

/*====================================================================*
 *   custom header files;
 *--------------------------------------------------------------------*/

#include "qca_7k.h"

/*====================================================================*
 *
 *   uint16_t qcaspi_read_register(struct qcaspi *qca, uint16_t reg);
 *
 *   Read the specified register from the QCA7K
 *
 *--------------------------------------------------------------------*/

uint16_t
qcaspi_read_register(struct qcaspi *qca, uint16_t reg)
{
	uint16_t tx_data[2] = {0x0};
	uint16_t rx_data[2] = {0x0};

	tx_data[0] = __cpu_to_be16(QCA7K_SPI_READ | QCA7K_SPI_INTERNAL | reg);
	tx_data[1] = 0xAFFE; /* arbitrary value */

	Chip_SSP_DATA_SETUP_T data;
	data.length = 4;
	data.tx_data = &tx_data;
	data.tx_cnt = 0;
	data.rx_data = &rx_data;
	data.rx_cnt = 0;

	int status = Board_SSP_AssertSSEL(qca->SSPx);

	Chip_SSP_RWFrames_Blocking(qca->SSPx, &data);

	if(status) Board_SSP_DeassertSSEL(qca->SSPx);

	return __be16_to_cpu(rx_data[1]);
}

/*====================================================================*
 *
 *   void qcaspi_write_register(struct qcaspi *qca, uint16_t reg, uint16_t value);
 *
 *   Write a 16 bit value to the specified SPI register
 *
 *--------------------------------------------------------------------*/

void
qcaspi_write_register(struct qcaspi *qca, uint16_t reg, uint16_t value)
{
	uint16_t tx_data[2];

	tx_data[0] = __cpu_to_be16(QCA7K_SPI_WRITE | QCA7K_SPI_INTERNAL | reg);
	tx_data[1] = __cpu_to_be16(value);

	Chip_SSP_DATA_SETUP_T data;
	data.length = 4;
	data.tx_data = &tx_data;
	data.tx_cnt = 0;
	data.rx_data = NULL;
	data.rx_cnt = 0;

	int status = Board_SSP_AssertSSEL(qca->SSPx);

	Chip_SSP_RWFrames_Blocking(qca->SSPx, &data);

	if(status) Board_SSP_DeassertSSEL(qca->SSPx);
}

/*====================================================================*
 *
 *   void qcaspi_tx_cmd(struct qcaspi *qca, uint16_t cmd);
 *
 *   Transmits a 16 bit command.
 *
 *--------------------------------------------------------------------*/

int
qcaspi_tx_cmd(struct qcaspi *qca, uint16_t cmd)
{
	cmd = __cpu_to_be16(cmd);

	Chip_SSP_DATA_SETUP_T data;
	data.length = 2;
	data.tx_data = &cmd;
	data.tx_cnt = 0;
	data.rx_data = NULL;
	data.rx_cnt = 0;

	int status = Board_SSP_AssertSSEL(qca->SSPx);

	Chip_SSP_RWFrames_Blocking(qca->SSPx, &data);

	if(status) Board_SSP_DeassertSSEL(qca->SSPx);

	return 0;
}

/*====================================================================*
 *
 *--------------------------------------------------------------------*/
