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
 *====================================================================*
 *
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
 *
 * registerGreenPHY.c
 */

#include <registerGreenPHY.h>
#include <spiGreenPHY.h>
#include <string.h>
#include <debug.h>

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
	uint16_t result;

	tx_data[0] = __cpu_to_be16(QCA7K_SPI_READ | QCA7K_SPI_INTERNAL | reg);

	int status = SSP_SSELToggle(qca->SSPx,0);

	qca->sspDataConfig.length = 4;
	qca->sspDataConfig.tx_data = &tx_data;
	tx_data[1]=0xAFFE;
	qca->sspDataConfig.rx_data = &rx_data;
	SSP_ReadWrite(qca->SSPx, &qca->sspDataConfig, SSP_TRANSFER_POLLING);
	result = rx_data[1];

	if(status) SSP_SSELToggle(qca->SSPx,1);

	return __be16_to_cpu(result);
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

	qca->sspDataConfig.rx_data = NULL;
	DEBUG_PRINT(DMA_INTERUPT,"reg:0x%x ",tx_data[0]);

	int status = SSP_SSELToggle(qca->SSPx,0);

	qca->sspDataConfig.length = 4;
	qca->sspDataConfig.tx_data = &tx_data;
	SSP_ReadWrite(qca->SSPx, &qca->sspDataConfig, SSP_TRANSFER_POLLING);

	if (status) SSP_SSELToggle(qca->SSPx,1);
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

	qca->sspDataConfig.length = 2;
	qca->sspDataConfig.tx_data = &cmd;
	qca->sspDataConfig.rx_data = NULL;

	int status = SSP_SSELToggle(qca->SSPx,0);

	SSP_ReadWrite(qca->SSPx, &qca->sspDataConfig, SSP_TRANSFER_POLLING);

	if (status) SSP_SSELToggle(qca->SSPx,1);

	return 0;
}
