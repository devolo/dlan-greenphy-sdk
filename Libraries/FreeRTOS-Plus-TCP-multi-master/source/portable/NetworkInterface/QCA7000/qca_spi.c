/*====================================================================*
 *
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
 *   qca_spi.c
 *
 *   This module implements the Qualcomm Atheros SPI protocol for
 *   kernel-based SPI device; it is essentially an Ethernet-to-SPI
 *   serial converter;
 *
 *--------------------------------------------------------------------*/

/*====================================================================*
 *   system header files;
 *--------------------------------------------------------------------*/
/* Standard includes. */
#include <stdint.h>

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
#include "qca_spi.h"
#include "qca_framing.h"
#include "qca_7k.h"

#if( ipconfigREAD_MAC_FROM_GREENPHY != 0 )
	#include "mme_handler.h"
#endif

/**********************************************************************/
static uint16_t available = 0;

/*====================================================================*
 *
 * Disables all SPI interrupts and returns the old value of the
 * interrupt enable register.
 *
 *--------------------------------------------------------------------*/

uint32_t
disable_spi_interrupts(struct qcaspi *qca)
{
	uint32_t old_intr_enable = qcaspi_read_register(qca, SPI_REG_INTR_ENABLE);
	qcaspi_write_register(qca, SPI_REG_INTR_ENABLE, 0);
	return old_intr_enable;
}

/*====================================================================*
 *
 * Enables only the SPI interrupts passed in the intr argument.
 * All others are disabled.
 * Returns the old value of the interrupt enable register.
 *
 *--------------------------------------------------------------------*/

uint32_t
enable_spi_interrupts(struct qcaspi *qca, uint32_t intr_enable)
{
	uint32_t old_intr_enable = qcaspi_read_register(qca, SPI_REG_INTR_ENABLE);
	qcaspi_write_register(qca, SPI_REG_INTR_ENABLE, intr_enable);
	return old_intr_enable;
}

/*====================================================================*
 *
 * Transmits a write command and len bytes of data
 * from src buffer in a single burst.
 *
 * Returns 0 if not all data could be transmitted,
 * and len if all data was transmitted.
 *
 *--------------------------------------------------------------------*/

extern SemaphoreHandle_t xGreenPHY_DMASemaphore;
extern void GreenPHY_DMA_IRQHandler (portBASE_TYPE * xHigherPriorityTaskWoken);

uint16_t
qcaspi_write_blocking(struct qcaspi *qca, uint8_t *src, uint16_t len)
{
Chip_SSP_DATA_SETUP_T xf_setup = { 0 };

	xf_setup.length = len;
	xf_setup.tx_data = src;
	xf_setup.rx_data = NULL;
	Chip_SSP_RWFrames_Blocking(qca->SSPx, &xf_setup);

	return len;
}

uint16_t
qcaspi_write_burst(struct qcaspi *qca, uint8_t* src, uint16_t len)
{
	uint8_t localBuffer[QCAFRM_HEADER_LEN];

	qcaspi_tx_cmd(qca, QCA7K_SPI_WRITE | QCA7K_SPI_EXTERNAL);

	/* send QCA header */
	QcaFrmCreateHeader(localBuffer, len, 0);
	qcaspi_write_blocking(qca, localBuffer, QCAFRM_HEADER_LEN);

	/* Get a free DMA channel and register the interrupt handler */
	uint8_t channel = Chip_GPDMA_GetFreeChannel(LPC_GPDMA, GPDMA_CONN_SSP0_Tx);
	registerInterruptHandlerDMA(channel, GreenPHY_DMA_IRQHandler);

	/* TxBuf --> SSP */
	Chip_GPDMA_Transfer(LPC_GPDMA,
						channel,
						(uint32_t) src,
						GPDMA_CONN_SSP0_Tx,
						GPDMA_TRANSFERTYPE_M2P_CONTROLLER_DMA,
						len);

	Chip_SSP_DMA_Enable(qca->SSPx);
	/* wait for DMA to complete */
	xSemaphoreTake( xGreenPHY_DMASemaphore, portMAX_DELAY );
	Chip_SSP_DMA_Disable(qca->SSPx);

	/* stop DMA transfer and free the channel */
	Chip_GPDMA_Stop(LPC_GPDMA, channel);

	/* send QCA footer */
	QcaFrmCreateFooter(localBuffer);
	qcaspi_write_blocking(qca, localBuffer, QCAFRM_FOOTER_LEN);

	return len;
}

/*====================================================================*
 *
 * Sends a read command and then receives len
 * bytes of data from the external SPI slave into
 * the buffer at dst.
 *
 * Returns 0 if not all data could be received,
 * and len if all data was received.
 *
 *--------------------------------------------------------------------*/

uint16_t
qcaspi_read_blocking(struct qcaspi *qca, uint8_t *dst, uint16_t len)
{
Chip_SSP_DATA_SETUP_T xf_setup = { 0 };

	qcaspi_write_register(qca, SPI_REG_BFR_SIZE, len);

	int status = Board_SSP_AssertSSEL(qca->SSPx);

	qcaspi_tx_cmd(qca, (QCA7K_SPI_READ | QCA7K_SPI_EXTERNAL));

	xf_setup.length = len;
	xf_setup.tx_data = NULL;
	xf_setup.rx_data = dst;
	Chip_SSP_RWFrames_Blocking(qca->SSPx, &xf_setup);

	if (status) Board_SSP_DeassertSSEL(qca->SSPx);

	available -= len;

	return len;
}

uint16_t
qcaspi_read_burst(struct qcaspi *qca, uint8_t *dst, uint16_t len)
{
	qcaspi_write_register(qca, SPI_REG_BFR_SIZE, len);

	int status = Board_SSP_AssertSSEL(qca->SSPx);

	qcaspi_tx_cmd(qca, (QCA7K_SPI_READ | QCA7K_SPI_EXTERNAL));

	/* The SSP interface needs to write data to drive the clock, thus
	 * we need a dummy TX transfer, where we just send arbitrary data
	 * of the same length as RX */
	uint8_t channelTX = Chip_GPDMA_GetFreeChannel(LPC_GPDMA, GPDMA_CONN_SSP0_Tx);
	uint8_t channelRX = Chip_GPDMA_GetFreeChannel(LPC_GPDMA, GPDMA_CONN_SSP0_Rx);
	registerInterruptHandlerDMA(channelTX, GreenPHY_DMA_IRQHandler);
	registerInterruptHandlerDMA(channelRX, GreenPHY_DMA_IRQHandler);

	/* dummy data --> SSP */
	Chip_GPDMA_Transfer(LPC_GPDMA,
						channelTX,
						(uint32_t) dst,
						GPDMA_CONN_SSP0_Tx,
						GPDMA_TRANSFERTYPE_M2P_CONTROLLER_DMA,
						len);

	/* SSP --> RxBuf */
	Chip_GPDMA_Transfer(LPC_GPDMA,
						channelRX,
						GPDMA_CONN_SSP0_Rx,
						(uint32_t) dst,
						GPDMA_TRANSFERTYPE_P2M_CONTROLLER_DMA,
						len);

	Chip_SSP_DMA_Enable(qca->SSPx);
	/* wait for DMA to complete, take one semaphore for RX and TX each */
	xSemaphoreTake( xGreenPHY_DMASemaphore, portMAX_DELAY );
	xSemaphoreTake( xGreenPHY_DMASemaphore, portMAX_DELAY );
	Chip_SSP_DMA_Disable(qca->SSPx);

	/* stop DMA transfer and free the channels */
	Chip_GPDMA_Stop(LPC_GPDMA, channelTX);
	Chip_GPDMA_Stop(LPC_GPDMA, channelRX);

	if (status) Board_SSP_DeassertSSEL(qca->SSPx);

	available -= len;

	return len;
}

/*====================================================================*
 *
 * Transmits an buffer in burst mode.
 *
 * Returns number of transmitted bytes, 0 on failure.
 *
 *--------------------------------------------------------------------*/

uint16_t
qcaspi_tx_frame(struct qcaspi *qca, NetworkBufferDescriptor_t * txBuffer)
{
	uint16_t writtenBytes = 0;

	uint8_t* pucData = txBuffer->pucEthernetBuffer;
	uint16_t len = txBuffer->xDataLength;
	uint16_t pad_len;
	if (len < QCAFRM_ETHMINLEN) {
		pad_len = QCAFRM_ETHMINLEN - len;
		memset(pucData+len, 0, pad_len);
		len += pad_len;
	}

	int status = Board_SSP_AssertSSEL(qca->SSPx);
	qcaspi_write_register(qca, SPI_REG_BFR_SIZE, len + QCAFRM_FRAME_OVERHEAD);
	if (status) Board_SSP_DeassertSSEL(qca->SSPx);

	status = Board_SSP_AssertSSEL(qca->SSPx);

	/* send ethernet packet via DMA to SPI */
	writtenBytes = qcaspi_write_burst(qca, pucData, len);

	if (status) Board_SSP_DeassertSSEL(qca->SSPx);

	return writtenBytes;
}

/*====================================================================*
 *
 * Transmits as many sk_buff's that will fit in
 * the SPI slave write buffer.
 *
 * Returns -1 on failure, 0 on success.
 *
 *--------------------------------------------------------------------*/
int
qcaspi_transmit(struct qcaspi *qca)
{
	uint16_t available;
	NetworkBufferDescriptor_t *txBuffer;

	/* read the available space in bytes from QCA7k */
	available = qcaspi_read_register(qca, SPI_REG_WRBUF_SPC_AVA);

	while( xQueuePeek(qca->txQueue, &txBuffer, 0) )
	{
		/* check whether there is enough space in the QCA7k buffer to hold
		 * the next packet */
		if ( available < (txBuffer->xDataLength + QCAFRM_FRAME_OVERHEAD) )
		{
			return -1;
		}

		/* receive and process the next packet */
		xQueueReceive(qca->txQueue, &txBuffer, 0);

		uint16_t writtenBytes = qcaspi_tx_frame(qca, txBuffer);

		vReleaseNetworkBufferAndDescriptor(txBuffer);

		available -= (writtenBytes + QCAFRM_FRAME_OVERHEAD);
		qca->stats.tx_packets++;
		qca->stats.tx_bytes += writtenBytes;
	}

	return 0;
}

/*====================================================================*
 *
 * Read the number of bytes available in the SPI slave and
 * then read and process the data from the slave.
 *
 * Returns -1 on error, 0 on success.
 *
 *--------------------------------------------------------------------*/

void
qcaspi_process_rx_buffer(struct qcaspi *qca)
{
int32_t ret;
	qca->rx_buffer_pos = 0;
	while( qca->rx_buffer_pos < qca->rx_buffer_len )
	{
		ret = QcaFrmFsmDecode( &qca->lFrmHdl, qca->rx_buffer[qca->rx_buffer_pos], qca->rx_desc->pucEthernetBuffer );
		switch( ret )
		{
		case QCAFRM_GATHER:
		case QCAFRM_NOHEAD:
			break;
		case QCAFRM_NOTAIL:
			qca->stats.rx_errors++;
			qca->stats.rx_dropped++;
			break;
		case QCAFRM_INVLEN:
			qca->stats.rx_errors++;
			qca->stats.rx_dropped++;
			break;
		default:
			qca->rx_desc->xDataLength = ret;
			break;
		}
		qca->rx_buffer_pos++;
	}
}

int
qcaspi_receive(struct qcaspi *qca)
{
const TickType_t xDescriptorWaitTime = pdMS_TO_TICKS( 250 );
const UBaseType_t uxMinimumBuffersRemaining = 2UL;
IPStackEvent_t xRxEvent = { eNetworkRxEvent, NULL };

	/* Allocate rx buffer if we don't have one available. */
	if (qca->rx_desc == NULL)
	{
		if( uxGetNumberOfFreeNetworkBuffers() > uxMinimumBuffersRemaining )
		{
			qca->rx_desc = pxGetNetworkBufferWithDescriptor(ipTOTAL_ETHERNET_FRAME_SIZE, xDescriptorWaitTime);
		}
	}

	available = qcaspi_read_register(qca, SPI_REG_RDBUF_BYTE_AVA);

	/* At least one header is required. */
	while( qca->rx_desc && ( available >= QcaFrmBytesRequired( &qca->lFrmHdl ) ) )
	{
		switch( QcaFrmGetAction( &qca->lFrmHdl ) )
		{
		case QCAFRM_FIND_HEADER:
			/* Read data of the size of one header. */
			qca->rx_buffer_len = qcaspi_read_blocking( qca, qca->rx_buffer, QcaFrmBytesRequired( &qca->lFrmHdl ) );
			qcaspi_process_rx_buffer( qca );
			break;

		case QCAFRM_COPY_FRAME:
			/* Start DMA read to copy the frame into the ethernet buffer. */
			qca->lFrmHdl.state -= qcaspi_read_burst(qca, qca->rx_desc->pucEthernetBuffer + qca->lFrmHdl.offset, ( qca->lFrmHdl.len - qca->lFrmHdl.offset ) );
			break;

		case QCAFRM_CHECK_FOOTER:
			/* Read footer. */
			qca->rx_buffer_len = qcaspi_read_blocking( qca, qca->rx_buffer, QcaFrmBytesRequired( &qca->lFrmHdl ) );
			qcaspi_process_rx_buffer( qca );
			break;

		case QCAFRM_FRAME_COMPLETE:
			qca->stats.rx_packets++;
			qca->stats.rx_bytes += qca->rx_desc->xDataLength;

			/* Data was received and stored.  Send a message to the IP
			task to let it know. */

		#if( ipconfigREAD_MAC_FROM_GREENPHY != 0 )
			/* Check if frame is an MME which is handled elsewhere. */
			if( !filter_rx_mme( qca->rx_desc ) )
		#endif
			{
				/* Set the receiving interface */
				qca->rx_desc->pxInterface = qca->pxInterface;
			#if( ipconfigUSE_BRIDGE != 0 )
				if( qca->pxInterface->bits.bIsBridged )
				{
					if( xBridge_Process( qca->rx_desc ) == pdPASS )
					{
						/* The bridge passed the descriptor to another NetworkInterface. */
						qca->rx_desc = NULL;
					}
					else
					{
						/* The bridge could not process the buffer, but there is no need
						to free it, as the buffer will be reused for the next packet. */
						qca->stats.rx_dropped++;
						iptraceETHERNET_RX_EVENT_LOST();
					}
				}
				else
			#endif
				{
					/* Pass data up to the IP Task */
					xRxEvent.pvData = ( void * ) qca->rx_desc;
					if( xSendEventStructToIPTask( &xRxEvent, ( TickType_t ) 0 ) == pdPASS )
					{
						/* The descriptor was sent into the TCP/IP stack. */
						qca->rx_desc = NULL;
					}
					else
					{
						/* Could not send the descriptor into the TCP/IP stack,
						but there is no need to free it, as the buffer will be
						reused for the next packet. */
						qca->stats.rx_dropped++;
						iptraceETHERNET_RX_EVENT_LOST();
					}
				}
			}

			iptraceNETWORK_INTERFACE_RECEIVE();

			/* Reset the frame handle, so a new header will be read */
			qca->lFrmHdl.state = QCAFRM_WAIT_AA1;

			if( qca->rx_desc == NULL )
			{
				/* Wait for a new buffer */
				if( uxGetNumberOfFreeNetworkBuffers() > uxMinimumBuffersRemaining )
				{
					qca->rx_desc = pxGetNetworkBufferWithDescriptor(ipTOTAL_ETHERNET_FRAME_SIZE, xDescriptorWaitTime);
				}
			}

			break;
		}
	}

	if( available >= QcaFrmBytesRequired( &qca->lFrmHdl ) )
	{
		/* Could not receive all frames. */
		return -1;
	}
	return 0;
}

/*====================================================================*
 *
 * Flush the tx queue. This function is only safe to
 * call from the qcaspi_spi_thread.
 *
 *--------------------------------------------------------------------*/

void
qcaspi_flush_txq(struct qcaspi *qca)
{
NetworkBufferDescriptor_t * txBuffer = NULL;

	while(xQueueReceive(qca->txQueue,&txBuffer,0))
	{
		vReleaseNetworkBufferAndDescriptor(txBuffer);
	}
}

/*====================================================================*
 *
 * Manage synchronization with the external SPI slave.
 *
 *--------------------------------------------------------------------*/
void
qcaspi_qca7k_sync(struct qcaspi *qca, int event)
{
	uint32_t signature;
	uint32_t spi_config;
	uint32_t wrbuf_space;
	static uint32_t reset_count;

	if( event != QCASPI_SYNC_UPDATE )
		qca->sync = event;

	while( 1 ) {
		switch( qca->sync ) {
		case QCASPI_SYNC_CPUON:
			/* Read signature twice, if not valid go back to unknown state. */
			signature = qcaspi_read_register(qca, SPI_REG_SIGNATURE);
			signature = qcaspi_read_register(qca, SPI_REG_SIGNATURE);
			if (signature != QCASPI_GOOD_SIGNATURE) {
				qca->sync = QCASPI_SYNC_HARD_RESET;
			} else {
				/* ensure that the WRBUF is empty */
				wrbuf_space = qcaspi_read_register(qca, SPI_REG_WRBUF_SPC_AVA);
				if (wrbuf_space != QCASPI_HW_BUF_LEN) {
					qca->sync = QCASPI_SYNC_SOFT_RESET;
				} else {
					qca->sync = QCASPI_SYNC_READY;
					return;
				}
			}
			break;

		case QCASPI_SYNC_UNKNOWN:
		case QCASPI_SYNC_RESET:
			signature = qcaspi_read_register(qca, SPI_REG_SIGNATURE);
			if (signature == QCASPI_GOOD_SIGNATURE)
			{
				/* signature correct, do a soft reset*/
				qca->sync = QCASPI_SYNC_SOFT_RESET;
			}
			else
			{
				/* could not read signature, do a hard reset */
				qca->sync = QCASPI_SYNC_HARD_RESET;
			}
			break;

		case QCASPI_SYNC_SOFT_RESET:
			spi_config = qcaspi_read_register(qca, SPI_REG_SPI_CONFIG);
			qcaspi_write_register(qca, SPI_REG_SPI_CONFIG, spi_config | QCASPI_SLAVE_RESET_BIT);

			qca->sync = QCASPI_SYNC_WAIT_RESET;
			reset_count = 0;
			return;

		case QCASPI_SYNC_HARD_RESET:
			/* reset is normally active low, so reset ... */
			Chip_GPIO_SetPinOutLow(LPC_GPIO, GREENPHY_RESET_GPIO_PORT, GREENPHY_RESET_GPIO_PIN);
			/*  ... for 100 ms ... */
			vTaskDelay( 100 * portTICK_RATE_MS);
			/* ... and release QCA7k from reset */
			Chip_GPIO_SetPinOutHigh(LPC_GPIO, GREENPHY_RESET_GPIO_PORT, GREENPHY_RESET_GPIO_PIN);

			qca->sync = QCASPI_SYNC_WAIT_RESET;
			reset_count = 0;
			return;

		case QCASPI_SYNC_WAIT_RESET:
			/* still awaiting reset, increase counter */
			++reset_count;
			if (reset_count >= QCASPI_RESET_TIMEOUT)
			{
				/* reset did not seem to take place, try again */
				qca->sync = QCASPI_SYNC_RESET;
			}
			break;

		case QCASPI_SYNC_READY:
		default:
			signature = qcaspi_read_register(qca, SPI_REG_SIGNATURE);
			/* if signature is correct, sync is still ready*/
			if (signature == QCASPI_GOOD_SIGNATURE)
			{
				return;
			}
			/* could not read signature, do a hard reset */
			qca->sync = QCASPI_SYNC_HARD_RESET;
			break;
		}
	}

}

/*====================================================================*
 *
 * qcaspi_spi_thread - SPI worker thread.
 *
 * Handle interrupts and transmit requests on the SPI interface.
 *
 * Return:   0 on success, else failure
 *
 *--------------------------------------------------------------------*/

#ifndef	GREENPHY_SYNC_HIGH_CHECK_TIME_MS
	/* Check if the Sync Status of the GREENPHY is still ready after 15 seconds of not
	receiving packets. */
	#define GREENPHY_SYNC_HIGH_CHECK_TIME_MS	15000
#endif

#ifndef	GREENPHY_SYNC_LOW_CHECK_TIME_MS
	/* Check if the Sync Status of the GREENPHY is still not ready every second. */
	#define GREENPHY_SYNC_LOW_CHECK_TIME_MS	1000
#endif

extern void vCheckBuffersAndQueue( void );
extern void GreenPHY_GPIO_IRQHandler (portBASE_TYPE * xHigherPriorityTaskWoken);
void
qcaspi_spi_thread(void *data)
{
struct qcaspi *qca = (struct qcaspi *) data;
uint32_t ulInterruptCause;
uint32_t intr_enable;
uint32_t ulNotificationValue;
TickType_t xSyncRemTime = pdMS_TO_TICKS( GREENPHY_SYNC_LOW_CHECK_TIME_MS );
BaseType_t available = pdFALSE;

	for ( ;; )
	{
		#if ipconfigHAS_PRINTF
			vCheckBuffersAndQueue();
		#endif

		/* Take notification
		 * 0 timeout
		 * 1 interrupt (including receive)
		 * 2 receive (not used, handled by interrupt)
		 * 4 transmit
		 * */
		if( ( qca->sync == QCASPI_SYNC_READY ) && ( available == pdFALSE ) )
		{
			xSyncRemTime = pdMS_TO_TICKS( GREENPHY_SYNC_HIGH_CHECK_TIME_MS );
		}
		else
		{
			xSyncRemTime = pdMS_TO_TICKS( GREENPHY_SYNC_LOW_CHECK_TIME_MS );
		}

		ulNotificationValue = ulTaskNotifyTake( pdTRUE, xSyncRemTime );
		if ( !ulNotificationValue && ( available == pdFALSE ) )
		{
			/* We got a timeout, check if we need to restart sync. */
			qcaspi_qca7k_sync(qca, QCASPI_SYNC_UPDATE);
			/* Not synced. Awaiting reset, or sync unknown. */
			if (qca->sync != QCASPI_SYNC_READY)
			{
				qcaspi_flush_txq(qca);
				continue;
			}
		}

		if ( ulNotificationValue & QCAGP_INT_FLAG )
		{
			/* We got an interrupt. */
			intr_enable = disable_spi_interrupts(qca);
			ulInterruptCause = qcaspi_read_register(qca, SPI_REG_INTR_CAUSE);

			/* Re-enable the GPIO interrupt. */
			registerInterruptHandlerGPIO(GREENPHY_INT_PORT, GREENPHY_INT_PIN, GreenPHY_GPIO_IRQHandler);

			if (ulInterruptCause & SPI_INT_CPU_ON)
			{
				qcaspi_qca7k_sync(qca, QCASPI_SYNC_CPUON);
				/* If not synced, wait reset. */
				if (qca->sync != QCASPI_SYNC_READY)
					continue;

				/* Reset interrupts. */
				intr_enable = (SPI_INT_CPU_ON | SPI_INT_PKT_AVLBL | SPI_INT_RDBUF_ERR | SPI_INT_WRBUF_ERR);
			}

			if (ulInterruptCause & ( SPI_INT_RDBUF_ERR | SPI_INT_WRBUF_ERR ) )
			{
				/* Restart sync. */
				qcaspi_qca7k_sync(qca, QCASPI_SYNC_RESET);
				continue;
			}

			if (ulInterruptCause & SPI_INT_PKT_AVLBL)
			{
				/* Packets available to receive. */
				available = pdTRUE;
			}

			if (ulInterruptCause & SPI_INT_WRBUF_BELOW_WM)
			{
				/* transmit is handled later */
				/* disable write watermark interrupt */
				intr_enable &= ~SPI_INT_WRBUF_BELOW_WM;
			}

			qcaspi_write_register(qca, SPI_REG_INTR_CAUSE, ulInterruptCause);
			enable_spi_interrupts(qca, intr_enable);
		}

		if (qca->sync == QCASPI_SYNC_READY)
		{
			if( available == pdTRUE )
			{
				if( qcaspi_receive(qca) == 0 )
				{
					/* All packets received. */
					available = pdFALSE;
				}
			}

			if( uxQueueMessagesWaiting(qca->txQueue) )
			{
				if( qcaspi_transmit(qca) != 0 )
				{
					/* QCA7k write buffer is full, but we need to send more packets
					 * so set watermark interrupt */
					uint32_t old_intr_enable = qcaspi_read_register(qca, SPI_REG_INTR_ENABLE);
					qcaspi_write_register(qca, SPI_REG_INTR_ENABLE, old_intr_enable | SPI_INT_WRBUF_BELOW_WM);
				}
			}
		}
	}

}

