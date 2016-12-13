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

 spiGreenPHY.c
 */

#include <FreeRTOS.h>
#include <task.h>
#include <GreenPHY.h>
#include <registerGreenPHY.h>
#include <string.h>
#include <dma.h>
#include <buffer.h>
#include <debug.h>
#include <spiGreenPHY.h>
#include <timers.h>
#include <mmeHandling.h>

const signed char * const greenPhySyncName = (const signed char * const) "GreenPhySnc";

void qcaspi_qca7k_sync(struct qcaspi *qca, int event);

/*-----------------------------------------------------------*/

/*
 * Initializes and takes dev as used netdevice interface.
 *
 * Returns -1 on failure, 0 on success.
 */

int greenPhyHandlerInit(struct qcaspi *qca, struct netDeviceInterface * dev)
{
	int rv = 1;

	if( qca != NULL )
	{
		memset(qca, 0, sizeof(struct qcaspi));
		qca->sync = QCASPI_SYNC_UNKNOWN;

		qca->SSPx = GREEN_PHY_SSP_CHANNEL;

		qca->netdevice = dev;

		qca->sspDataConfig.length = 1;
		qca->sspDataConfig.tx_data = NULL;
		qca->sspDataConfig.rx_data = NULL;

		qca->syncGardTimeout = 3;

		qca->reset_count = QCA7K_MAX_RESET_COUNT;

		QcaSetRxStatus(&(qca->rxStatus), init);

		int txQueueSize = ((ETH_NUM_BUFFERS/2) + 1);

		int i;
		for (i=0;i<QCAGP_NO_OF_QUEUES;i+=1)
		{
			qca->tx_priority_q[i] = xQueueCreate( txQueueSize, sizeof( struct netdeviceQueueElement *) );
		}
		qca->rxQueue = xQueueCreate( (ETH_NUM_BUFFERS/2) + 1 , sizeof( struct netdeviceQueueElement *) );

		SSP_ConfigStructInit(&qca->sspChannelConfig);

		SSP_Init(qca->SSPx, &qca->sspChannelConfig);

		SSP_Cmd(qca->SSPx, ENABLE);
		rv = 0;
	}
	return rv;
}

/*-----------------------------------------------------------*/
/*
 * Check for sync with the QCA7k
 */

static void greenPhyNetdeviceSyncCheck( xTimerHandle xTimer )
{
	DEBUG_PRINT(GREEN_PHY_INTERUPT,"GreenPhy syncTimer with 0x%x expired\r\n",xTimer);

	struct qcaspi *qca = (struct qcaspi *) pvTimerGetTimerID(xTimer);
	xTimerDelete(xTimer,0);
	qca->syncGard = 0;

	DEBUG_PRINT(GREEN_PHY_INTERUPT,"GreenPhy sync status 0x%x \r\n",qca->sync);

	if(qca->sync == QCASPI_SYNC_READY)
	{
		DEBUG_PRINT(GREEN_PHY_INTERUPT|GREEN_PHY_SYNC_STATUS,"GreenPhy is ok\r\n");
	}
	else
	{
#if SIMULATE_CPU_ON_EVENT == ON
		qcaspi_qca7k_sync(qca,QCASPI_SYNC_CPUON);
		if(qca->sync == QCASPI_SYNC_READY)
		{
			DEBUG_PRINT(GREEN_PHY_INTERUPT|GREEN_PHY_SYNC_STATUS,"GreenPhy is now sync\r\n");
		}
		else
#endif
		{
			if( qca->syncGardTimeout < 60 )
			{
				DEBUG_PRINT(GREEN_PHY_INTERUPT,"Increasing timeout...\r\n");
				qca->syncGardTimeout += 1;
			}
			DEBUG_PRINT(GREEN_PHY_INTERUPT,"GreenPhy reset!!! \r\n");
			struct netDeviceInterface * netdevice = qca->netdevice;
			netdevice->reset(netdevice);
		}
	}
}

/*-----------------------------------------------------------*/

/*
 * Starts the sync guard.
 */

void startSyncGuard(struct qcaspi *qca)
{
	if(!qca->syncGard)
	{
		xTimerHandle xTimer = xTimerCreate(greenPhySyncName, qca->syncGardTimeout * 1000, FALSE, qca, greenPhyNetdeviceSyncCheck);
		DEBUG_PRINT(GREEN_PHY_SYNC_STATUS,"GreenPhy create syncTimer 0x%x with greenPhy 0x%x\r\n",xTimer,qca);
		xTimerStart(xTimer, 0);
		qca->syncGard = 1;
	}
}

/*-----------------------------------------------------------*/

/*
 * Disables all SPI interrupts and returns the old value of the
 * interrupt enable register.
 */
uint32_t
disable_spi_interrupts(struct qcaspi *qca)
{
	uint32_t old_intr_enable = qcaspi_read_register(qca, SPI_REG_INTR_ENABLE);
	qcaspi_write_register(qca, SPI_REG_INTR_ENABLE, 0);
	return old_intr_enable;
}

/*-----------------------------------------------------------*/

/*
 * Enables only the SPI interrupts passed in the intr argument.
 * All others are disabled.
 * Returns the old value of the interrupt enable register.
 */
uint32_t
enable_spi_interrupts(struct qcaspi *qca, uint32_t intr_enable)
{
	uint32_t old_intr_enable = qcaspi_read_register(qca, SPI_REG_INTR_ENABLE);
	qcaspi_write_register(qca, SPI_REG_INTR_ENABLE, intr_enable);
	return old_intr_enable;
}

/*-----------------------------------------------------------*/
#if GREEN_PHY_SIMPLE_QOS == ON
/*
 * Returns the QID corresponding to the priority of the frame
 * If the frame has VLAN header, the QID maps to the priority of the VLAN tag
 * If the VLAN header is not present, returns default QID of 0
 *
 * Used only with Simple QoS feature in QCA7000 device.
 * With the Simple QoS feature enabled in QCA7000 device, the classification
 * engine in the QCA7000 device is disabled. It is now the responsibility of
 * the host to classify the traffic into one of 4 CAP priorities depending on
 * its needs. In this reference driver implementation, VLAN priorities are
 * used for classifying the traffic into CAP priorities.
 */

uint32_t qcaspi_get_qid_from_eth_frame_data(uint8_t * ptmp)
{
	uint32_t qid=0;
	if(ptmp)
	{
		uint8_t bcast[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

		//Get QID here from the vlan tag
		uint8_t temp_b, vlan_pri;
		uint16_t temp_s, ether_type;

		if(memcmp(ptmp, bcast, 6) == 0)	// All broadcast at CAP-0
		{
			qid = 0;
		}
		else if((*ptmp & 1))  			// All Multicast at CAP-0
		{
			qid = 0;
		}
		else					// Default classification here
		{
			qid = 1; 			// All unicast traffic go at CAP-1 priority

			memcpy(&temp_s, (ptmp+12), 2);
			ether_type = NTOHS(temp_s);
			/***********************************
		 	* VLAN ID to CAP priority mapping
			* V : CAP
		 	* 0 : 1
		 	* 1 : 0
		 	* 2 : 0
		 	* 3 : 1
		 	* 4 : 2
		 	* 5 : 2
		 	* 6 : 3
		 	* 7 : 3
		 	***********************************/
			if(ether_type == ETHER_TYPE_VLAN) // Priority based on VLAN tags
			{
				temp_b = *(ptmp+14);
				vlan_pri = (temp_b>>5) & 0x7;

				if(vlan_pri & 0x4)
				{
					qid = (vlan_pri >> 1);
				}
				else
				{
					switch(vlan_pri & 3)
					{
						case 0:
						case 3:
							qid = 1;
							break;
						default:
							qid = 0;
							break;
					}
				}
			}
		}
	}
	return qid;
}
#endif

/*-----------------------------------------------------------*/

/*
 * Transmits a write command and len bytes of data
 * from src buffer in a single burst.
 *
 * Returns 0 if not all data could be transmitted,
 * and len if all data was transmitted.
 */

uint32_t
qcaspi_dma_write_burst(struct qcaspi *qca, uint8_t * src, uint16_t len)
{
	qcaspi_tx_cmd(qca, QCA7K_SPI_WRITE | QCA7K_SPI_EXTERNAL);

	uint8_t localBuffer[QCAFRM_HEADER_LEN+QCAFRM_QID_LEN];

	uint8_t offset = 0;
	uint32_t protocol_version = 0;
#if GREEN_PHY_SIMPLE_QOS == ON
	if (qca->driver_state == SERIAL_OVER_ETH_VER1_MODE)
	{
		offset = QcaFrmAddQID(localBuffer,qcaspi_get_qid_from_eth_frame_data(src));
		protocol_version = 1;
	}
#endif
	configASSERT((offset == 0)||(offset == QCAFRM_QID_LEN));

	QcaFrmCreateHeader(localBuffer+offset,len,protocol_version);
	qca->sspDataConfig.length = QCAFRM_HEADER_LEN+offset;
	qca->sspDataConfig.tx_data = &localBuffer;
	qca->sspDataConfig.rx_data = NULL;
	SSP_ReadWrite(qca->SSPx, &qca->sspDataConfig, SSP_TRANSFER_POLLING);

	qca->sspDataConfig.length = len;
	qca->sspDataConfig.tx_data = src;
	qca->sspDataConfig.rx_data = NULL;
	SSP_ReadWrite(qca->SSPx, &qca->sspDataConfig, SSP_TRANSFER_DMA);
	QcaFrmCreateFooter(localBuffer);
	qca->sspDataConfig.length = QCAFRM_FOOTER_LEN;
	qca->sspDataConfig.tx_data = &localBuffer;
	qca->sspDataConfig.rx_data = NULL;
	SSP_ReadWrite(qca->SSPx, &qca->sspDataConfig, SSP_TRANSFER_POLLING);

	return len;
}

/*-----------------------------------------------------------*/

/*
 * Sends a read command and then receives len
 * bytes of data from the external SPI slave into
 * the buffer at dst.
 *
 * Returns 0 if not all data could be received,
 * and len if all data was received.
 */
uint32_t
qcaspi_dma_read_burst(struct qcaspi *qca, uint8_t *dst, uint32_t len)
{
	uint16_t cmd = (QCA7K_SPI_READ | QCA7K_SPI_EXTERNAL);

	int status = SSP_SSELToggle(qca->SSPx,0);

	qcaspi_tx_cmd(qca,cmd);

	qca->sspDataConfig.length = len;
	qca->sspDataConfig.tx_data = dst;
	qca->sspDataConfig.rx_data = dst;

	qca->sspDataConfig.tx_data = NULL;
	SSP_ReadWrite(qca->SSPx, &qca->sspDataConfig, SSP_TRANSFER_DMA);

	if (status) SSP_SSELToggle(qca->SSPx,1);

	return len;
}

/*-----------------------------------------------------------*/

/*
 * Transmits an buffer in burst mode.
 *
 * Returns number of transmitted bytes, 0 on failure.
 */

uint16_t qcaspi_tx_frame(struct qcaspi *qca,struct netdeviceQueueElement * txBuffer)
{
	uint16_t lByteWritten = 0;

	if(txBuffer)
	{
		uint16_t len;
		uint8_t pad_len = 0;

		int status;

		status = SSP_SSELToggle(qca->SSPx,0);

		len = getLengthFromQueueElement(txBuffer);

		if (len < QCAFRM_ETHMINLEN) {
			pad_len = QCAFRM_ETHMINLEN - len;
			len += pad_len;
		}

		qcaspi_write_register(qca, SPI_REG_DMA_SIZE, len + QCAFRM_FRAME_OVERHEAD);

		if (status)
			SSP_SSELToggle(qca->SSPx,1);

		status = SSP_SSELToggle(qca->SSPx,0);

		uint8_t * pData = getDataFromQueueElement(txBuffer);

		DEBUG_PRINT(GREEN_PHY_TX,"(GREEN_PHY) tx buffer: 0x%x length %d\r\n",pData,len);
		DEBUG_DUMP(GREEN_PHY_TX_BINARY,pData,len,"GREEN PHY TX");

		DEBUG_DUMP_ICMP_FRAME(GREEN_PHY_TX,pData,len,"GREEN PHY TX");

		lByteWritten = qcaspi_dma_write_burst(qca, pData, len);

		if (status)
			SSP_SSELToggle(qca->SSPx,1);

		DEBUG_PRINT(DEBUG_BUFFER,"[#D#]\r\n");
		returnQueueElement(&txBuffer);
		DEBUG_PRINT(GREEN_PHY_TX,"(GREEN_PHY) return buffer: 0x%x\r\n",pData);
	}
	else
	{
		DEBUG_PRINT(GREEN_PHY_TX|DEBUG_ERR,"(GREEN_PHY) %s no buffer!\r\n",__func__);
	}
	return lByteWritten;
}
#if GREEN_PHY_SIMPLE_QOS == ON
uint32_t qcaspi_get_qid_from_eth_frame(struct netdeviceQueueElement * buffer)
{
	uint32_t qid;

	qid=qcaspi_get_qid_from_eth_frame_data(getDataFromQueueElement(buffer));

	return qid;
}
#endif

/*-----------------------------------------------------------*/

/*
 * Returns the quene element from the qid queue
 *
 * Returns NULL if empty
 */
struct netdeviceQueueElement * getbufferbyqid(struct qcaspi* qca, uint32_t qid)
{
	struct netdeviceQueueElement * rv = NULL;
	configASSERT((qid<QCAGP_NO_OF_QUEUES));
	xQueueReceive(qca->tx_priority_q[qid],&rv,0);
	return rv;
}

/*-----------------------------------------------------------*/

/*
 * Returns the max priority queue id for which there is pending traffic
 *
 * Returns -1 if no pending tx traffic is present on any of the queues
 *
 * Returns -2 if there is pending tx traffic for a high priority queue, but we
 * have run out of resources by means of memory. We need to query the current
 * resource status at the target.
 */
int32_t get_max_prio_queue(struct qcaspi* qca, uint32_t max_size)
{
	int32_t rv=-1;
	struct netdeviceQueueElement * element = NULL;

#if GREEN_PHY_SIMPLE_QOS == ON
	if (qca->driver_state == SERIAL_OVER_ETH_VER1_MODE)
	{
		int32_t i;

		for (i=QCAGP_NO_OF_QUEUES-1; i>=0; i=i-1)
		{
			/*
			 * Is in queue number i an element and would the element fit into the QCA7k buffer?
			 */
			xQueuePeek(qca->tx_priority_q[i],&element,0);
			if ((element)&& (max_size >= getLengthFromQueueElement(element) + QCAFRM_FRAME_OVERHEAD ))
			{
				if ((qca->rxStatus.queue_sizes[i] >= qca->max_queue_size[i]) )
				{
					/* we have used up all credit for this prio ... */
					rv = QCAGP_NO_RESOURCE_FOR_THIS_QUEUE;
					/* ... but is there any lower prio data to send? */
					element = NULL;
					continue;
				}
				else
				{
					rv = i;
					/* pick this queue! */
					break;
				}
			}
		}
	}
	else
#endif
	{
		xQueuePeek(qca->tx_priority_q[QCAGP_DEFAULT_QUEUE],&element,0);
		if ((element) && (max_size >= getLengthFromQueueElement(element) + QCAFRM_FRAME_OVERHEAD))
		{
			rv = QCAGP_DEFAULT_QUEUE;
		}
	}

	return rv;
}

/*-----------------------------------------------------------*/
#if GREEN_PHY_SIMPLE_QOS == ON
/*
 * Returns 1 if it is time to query for current resource status in the target
 * The Host monitors the current queue sizes in the target and if it is less
 * than a threshold value, it is supposed to send a query current status
 * message, before it can send any more tx frames.
 */
int32_t is_time_to_query_queue_sizes(struct qcaspi* qca)
{
	int32_t i;
	int32_t query = 0;
	uint32_t threshold;

	for(i=QCAGP_NO_OF_QUEUES-1; i>=0; i=i-1)
	{
		threshold = (3*qca->max_queue_size[i])>>2;	//threshold of 3/4*max size
		//threshold = qca->max_queue_size[i]>>1;	//threshold of 1/2*max size
		if(qca->rxStatus.queue_sizes[i] >= threshold)
		{
			query = 1;
			break;
		}
	}
	return query;
}
#endif

/*-----------------------------------------------------------*/

/*
 * Tells if there are any packets available for transmit.
 * Returns a pointer to the available buffer if yes, else NULL
 */
struct netdeviceQueueElement * qcaspi_is_packet_available_for_transmit(struct qcaspi *qca)
{
	struct netdeviceQueueElement * rv=NULL;

	int32_t i;
	for (i=QCAGP_NO_OF_QUEUES-1; i>=0; i=i-1)
	{
		xQueuePeek(qca->tx_priority_q[i],&rv,0);
		if (rv != NULL)
		{
			break;
		}
	}

	return rv;
}


/*-----------------------------------------------------------*/

/*
 * Transmits as many buffers that will fit in
 * the SPI slave write buffer.
 *
 * Returns -1 on failure, 0 on out of space, otherwise available data buffer of QCA7000 on success.
 */
int
qcaspi_transmit(struct qcaspi *qca)
{
	int rv = -1;

	uint16_t available;
	int32_t qid;

	/* read out the available space in bytes from QCA7k */
	available = qcaspi_read_register(qca, SPI_REG_WRBUF_SPC_AVA);

	DEBUG_PRINT(GREEN_PHY_TX,"%s TX lAvailable 0x%x\r\n",__func__,available);

	qid = get_max_prio_queue(qca, available);

#if GREEN_PHY_SIMPLE_QOS == ON
	if (qca->driver_state == SERIAL_OVER_ETH_VER1_MODE)
	{
		/*
		 * Check if we have run out of credits. If yes, query credit
		 * information from the target.
		 */
		if (available && ((qid == QCAGP_NO_RESOURCE_FOR_THIS_QUEUE) || (is_time_to_query_queue_sizes(qca) == 1)))
		{
			struct netdeviceQueueElement * mme;
			mme = qcaspi_create_get_property_host_q_info(qca);
			qcaspi_tx_frame(qca, mme);
			available = qcaspi_read_register(qca, SPI_REG_WRBUF_SPC_AVA);
			DEBUG_PRINT(GREEN_PHY_FW_FEATURES,"%s TX lAvailable after mme 0x%x\r\n",__func__,available);
			qid = get_max_prio_queue(qca, available);
		}
	}
#endif

	while (available && qid >=0) {
		uint16_t writtenBytes = qcaspi_tx_frame(qca,getbufferbyqid(qca,qid));
		if ( writtenBytes == 0)
		{
			available = 0;
		}
		else
		{
			available -= (writtenBytes + QCAFRM_FRAME_OVERHEAD);
			qca->stats.tx_packets++;
			qca->stats.tx_bytes += writtenBytes;
		}

		rv = available;

#if GREEN_PHY_SIMPLE_QOS == ON
		if (qca->driver_state == SERIAL_OVER_ETH_VER1_MODE)
		{
			qca->rxStatus.queue_sizes[qid]+=1;
		}
#endif
		qid = get_max_prio_queue(qca, available);
	}

	return rv;
}

/*-----------------------------------------------------------*/

/*
 * Reads out how many bytes are available in the chip's RX buffer.
 */
void qcaspi_read_available_from_chip(struct qcaspi *qca)
{
	/* Read the packet size. */
	qca->rxStatus.lAvailable = qcaspi_read_register(qca, SPI_REG_RDBUF_BYTE_AVA);
}

/*-----------------------------------------------------------*/

/*
 * Get how many bytes are available in the chip's RX buffer.
 */
uint32_t qcaspi_get_available(struct qcaspi *qca)
{
	uint32_t rv = qca->rxStatus.lAvailable;
	return rv;
}

/*-----------------------------------------------------------*/

/*
 * Indicates data availabe and being able to be processed.
 *
 * Returns 0 on nothing to do.
 */
int qcaspi_process_data(struct qcaspi *qca)
{
	int rv = 0;

	if(qca->rxStatus.greenPhyRxBuffer == NULL)
	{
		struct netdeviceQueueElement * tmp = getQueueElement();
		DEBUG_PRINT(DEBUG_BUFFER,"[#I5#]\r\n");
		qca->rxStatus.greenPhyRxBuffer = removeDataFromQueueElement(&tmp);
		qca->rxStatus.current_position = qca->rxStatus.greenPhyRxBuffer;

		DEBUG_PRINT(GREEN_PHY_RX,"(GREEN_PHY) rxStatus.greenPhyRxBuffer: 0x%x\r\n",qca->rxStatus.greenPhyRxBuffer);
	}

	if(qca->rxStatus.greenPhyRxBuffer != NULL)
	{
		rv = qca->rxStatus.lAvailable && (qca->rxStatus.toRead);
	}
	else
	{
		DEBUG_PRINT(DEBUG_ERR,"(GREEN_PHY) %s no buffer...\r\n",__func__);
	}

	return rv;
}

/*-----------------------------------------------------------*/

/*
 * Put the received buffer to the rx queue.
 *
 * Returns -1 on failure, -2 of called without any received buffer, and 0 on success.
 */
int qcaspi_process_received_frame(struct qcaspi *qca)
{
	int rv = 0;

	struct netdeviceQueueElement * rxBuffer = getQueueElementByBuffer(qca->rxStatus.greenPhyRxBuffer,qca->rxStatus.frameLength);

	DEBUG_PRINT(GREEN_PHY_RX,"(GREEN_PHY) received buffer: 0x%x length :%d \r\n",qca->rxStatus.greenPhyRxBuffer,qca->rxStatus.frameLength);
	DEBUG_DUMP(GREEN_PHY_RX_BINARY,qca->rxStatus.greenPhyRxBuffer,qca->rxStatus.frameLength,"GREEN PHY RX");

	DEBUG_DUMP_ICMP_FRAME(GREEN_PHY_RX,qca->rxStatus.greenPhyRxBuffer,qca->rxStatus.frameLength,"GREEN PHY RX");

	if(rxBuffer)
	{
		switch(QcaGetSpiProtocolVersion(&qca->rxStatus))
		{
		case 0:
			// nothing to do
			break;
#if GREEN_PHY_SIMPLE_QOS == ON
		case 1:
			// 4 bytes queue sizes in front of the Ethernet frame
			qca->rxStatus.queue_sizes[0] = qca->rxStatus.greenPhyRxBuffer[0];
			qca->rxStatus.queue_sizes[1] = qca->rxStatus.greenPhyRxBuffer[1];
			qca->rxStatus.queue_sizes[2] = qca->rxStatus.greenPhyRxBuffer[2];
			qca->rxStatus.queue_sizes[3] = qca->rxStatus.greenPhyRxBuffer[3];
			// remove them from the frame
			removeBytesFromFrontOfQueueElement(rxBuffer,4);
			break;
#endif
		default:
			DEBUG_PRINT(DEBUG_ERR,"(GREEN_PHY) Unknown SPI protocol version: 0x%x\r\n",QcaGetSpiProtocolVersion(&qca->rxStatus));
			break;
		}

		qca->stats.rx_packets++;
		qca->stats.rx_bytes += getLengthFromQueueElement(rxBuffer);

#if GREEN_PHY_SIMPLE_QOS == ON
		filter_rx_mme(qca,&rxBuffer);

		if(rxBuffer)
#endif
		{
			if(xQueueSend(qca->rxQueue,&rxBuffer,10)!= pdPASS)
			{
				// Failed ...
				DEBUG_PRINT(DEBUG_BUFFER,"[#E#]\r\n");
				returnQueueElement(&rxBuffer);
				rv = -1;
			}
		}

		qca->rxStatus.greenPhyRxBuffer = NULL;
	}
	else
	{
		rv = -2;
	}
	return rv;
}

/*-----------------------------------------------------------*/

/*
 * Reads data frames from the QCA7k buffer.
 *
 * Returns the number of bytes to read on for a complete frame,
 * and 0 on success.
 */

uint32_t qcaspi_read_from_chip(struct qcaspi *qca)
{
	uint32_t rv = 0;

	if(qcaspi_process_data(qca))
	{
		uint32_t read;
		if(qca->rxStatus.lAvailable > qca->rxStatus.toRead)
		{
			read = qca->rxStatus.toRead;
		}
		else
		{
			read = qca->rxStatus.lAvailable;
		}

		qca->rxStatus.lAvailable -= read;
		qca->rxStatus.toRead -= read;

		rv = qca->rxStatus.toRead;

		qcaspi_dma_read_burst(qca, qca->rxStatus.current_position, read);

		DEBUG_PRINT(GREEN_PHY_RX,"(GREEN_PHY) read to 0x%x %d bytes", qca->rxStatus.current_position, read);
		qca->rxStatus.current_position += read;
		qca->rxStatus.length += read;
		DEBUG_PRINT(GREEN_PHY_RX," current_position now 0x%x\r\n", qca->rxStatus.current_position);
	}

	return rv;
}

/*-----------------------------------------------------------*/

/*
 * Read the number of bytes available in the SPI slave and
 * then read and process the data from the slave.
 *
 * Returns -1 on error, 0 on success.
 */
int
qcaspi_receive(struct qcaspi *qca)
{
	int rv = 0;
	if(qcaspi_get_available(qca) == 0)
	{
		qcaspi_read_available_from_chip(qca);
		if(qcaspi_get_available(qca) == 0)
		{
			DEBUG_PRINT(GREEN_PHY_RX,"(GREEN_PHY) %s called without any data being available!\r\n", __func__);
			return -1;
		}
		qcaspi_write_register(qca, SPI_REG_DMA_SIZE, qcaspi_get_available(qca));
	}

	DEBUG_PRINT(GREEN_PHY_RX,"(GREEN_PHY) available: %d\r\n", qcaspi_get_available(qca));

	while(qcaspi_process_data(qca))
	{
		int more_to_read = qcaspi_read_from_chip(qca);

		DEBUG_PRINT(GREEN_PHY_RX,"(GREEN_PHY) more_to_read: %d\r\n", more_to_read);

		if(!more_to_read)
		{
			rv = qca->rxStatus.process_result(&qca->rxStatus);

			if(rv == 1)
			{
				rv = qcaspi_process_received_frame(qca);
			}
			else if(rv<0)
			{
				qca->stats.rx_errors++;
				qca->stats.rx_dropped++;
				DEBUG_PRINT(GREEN_PHY_RX|DEBUG_ERR,"(GREEN_PHY) %s error: %d %d %d\r\n",__func__,qca->stats.rx_errors,qca->stats.rx_dropped,rv);
			}
		}
	}
	return rv;
}

/*-----------------------------------------------------------*/

/*
 * Flush the tx queue. This function is only safe to
 * call from the greenPhySpiWorker.
 */
static void qcaspi_flush_txq(struct qcaspi *qca)
{
	DEBUG_PRINT(GREEN_PHY_TX,"%s\r\n",__func__);

	struct netdeviceQueueElement * txBuffer = NULL;

	int i;

	for(i=0; i<QCAGP_NO_OF_QUEUES; i+=1)
	{
		while(xQueueReceive(qca->tx_priority_q[i],&txBuffer,0))
		{
			DEBUG_PRINT(DEBUG_BUFFER,"[#F#]\r\n");
			returnQueueElement(&txBuffer);
		}
	}

}

/*-----------------------------------------------------------*/

/*
 * Sets the sync status and starts the sync guard on demand.
 */
void qcaspi_qca7k_set_sync(struct qcaspi *qca, qca7k_sync_t sync)
{
	qca->sync = sync;

	DEBUG_PRINT(GREEN_PHY_SYNC_STATUS,"new qca7k sync status 0x%x\r\n",qca->sync);

	// check against QCASPI_SYNC_UNKNOWN due to reset code in qcaspi_qca7k_sync()
	if (sync == QCASPI_SYNC_UNKNOWN)
	{
		startSyncGuard(qca);
	}
	else if (sync == QCASPI_SYNC_READY)
	{
		qca->reset_count = 0;
	}
}

/*-----------------------------------------------------------*/

/*
 * Manage synchronization with the external SPI slave.
 */
void qcaspi_qca7k_sync(struct qcaspi *qca, int event)
{
	uint32_t signature;
	uint32_t spi_config;
	uint32_t wrbuf_space;
	static uint32_t reset_count;

	/* CPU ON occured, verify signature */
	if (event == QCASPI_SYNC_CPUON) {
#if GREEN_PHY_SIMPLE_QOS == ON
		/* reset the simple QoS state */
		qca->driver_state = INITIALIZED;
		qca->driver_state_count = 0;
#endif

		/* Read signature twice, if not valid go back to unknown state. */
		signature = qcaspi_read_register(qca, SPI_REG_SIGNATURE);
		signature = qcaspi_read_register(qca, SPI_REG_SIGNATURE);
		if (signature != QCASPI_GOOD_SIGNATURE) {
			DEBUG_PRINT(GREEN_PHY_SYNC_STATUS,"Got CPU on, but signature was invalid\r\n");
			qcaspi_qca7k_set_sync(qca, QCASPI_SYNC_UNKNOWN);
		} else {
			/* ensure that the WRBUF is empty */
			wrbuf_space = qcaspi_read_register(qca, SPI_REG_WRBUF_SPC_AVA);
			if (wrbuf_space != QCASPI_HW_BUF_LEN) {
				DEBUG_PRINT(GREEN_PHY_SYNC_STATUS,"Got CPU on, but wrbuf not empty\r\n");
				qcaspi_qca7k_set_sync(qca, QCASPI_SYNC_UNKNOWN);
			} else {
				DEBUG_PRINT(GREEN_PHY_SYNC_STATUS,"Got CPU on, now in sync\r\n");
				qcaspi_qca7k_set_sync(qca, QCASPI_SYNC_READY);
				return;
			}
		}
	}

	/* In sync. */
	if (qca->sync == QCASPI_SYNC_READY) {
		/* Don't check signature after sync in burst mode. */
		return;
	}

	/* Reset the device. */
	if (qca->sync == QCASPI_SYNC_UNKNOWN) {
		DEBUG_PRINT(GREEN_PHY_SYNC_STATUS,"QCASPI_SYNC_UNKNOWN\r\n");
		/* Read signature, if not valid stay in unknown state */
		signature = qcaspi_read_register(qca, SPI_REG_SIGNATURE);
		if (signature != QCASPI_GOOD_SIGNATURE) {
			DEBUG_PRINT(GREEN_PHY_SYNC_STATUS,"could not read signature to reset device, retry.\r\n");
			return;
		}

		DEBUG_PRINT(GREEN_PHY_SYNC_STATUS,"resetting device...\r\n");
		spi_config = qcaspi_read_register(qca, SPI_REG_SPI_CONFIG);
		qcaspi_write_register(qca, SPI_REG_SPI_CONFIG, spi_config | QCASPI_SLAVE_RESET_BIT);
		DEBUG_PRINT(GREEN_PHY_SYNC_STATUS,"QCASPI_SYNC_RESET\r\n");
		qcaspi_qca7k_set_sync(qca, QCASPI_SYNC_RESET);
		reset_count = 0;
		return;
	}

	/* Currently waiting for CPU on to take us out of reset. */
	if (qca->sync == QCASPI_SYNC_RESET) {
		++reset_count;
		DEBUG_PRINT(GREEN_PHY_SYNC_STATUS,"waiting for CPU on, count %d.\r\n", reset_count);
		if (reset_count >= QCASPI_RESET_TIMEOUT) {
			/* reset did not seem to take place, try again */
			DEBUG_PRINT(GREEN_PHY_SYNC_STATUS,"reset timeout, restarting process\r\n");
			qcaspi_qca7k_set_sync(qca, QCASPI_SYNC_UNKNOWN);
		}
	}
}

/*====================================================================*
 *
 * greenPhySpiWorker - SPI worker thread.
 *
 * Handle interrupts and transmit requests on the SPI interface.
 *
 * Return:   0 on success, else failure
 *
 *--------------------------------------------------------------------*/
int greenPhySpiWorker(void *data)
{
	struct qcaspi *qca = (struct qcaspi *) data;
	uint32_t vInterruptCause;
	uint32_t intr_enable;

	int available = 1;
	DEBUG_EXECUTE(int workingCounter=0);

	while (1) {
		DEBUG_EXECUTE(workingCounter += 1);
		if (!greenPhyInterruptAvailable()  && (!qcaspi_is_packet_available_for_transmit(qca) || (available < 1))&& qca->sync == QCASPI_SYNC_READY) {
			//DEBUG_PRINT(GREEN_PHY_TX|GREEN_PHY_RX|DEBUG_ERROR_SEARCH,"%s BREAK\r\n",__func__);
			break;
		}

		DEBUG_PRINT(GREEN_PHY_TX|GREEN_PHY_RX,"%s working ... %d \r\n",__func__,workingCounter);

		/* not synced, awaiting reset, or unknown */
		if (qca->sync != QCASPI_SYNC_READY) {
			//printk(KERN_DEBUG "qcaspi: sync: not ready, turn off carrier and flush\n");
			//netif_carrier_off(qca->dev);

			qcaspi_flush_txq(qca);

			//netif_wake_queue(qca->dev);

			/*  ... for 1000 ms ... */
			vTaskDelay( 1000 / portTICK_RATE_MS);
		}

		if (greenPhyInterruptAvailable()) {
			greenPhyClearInterrupt();
			intr_enable = disable_spi_interrupts(qca);
			vInterruptCause = qcaspi_read_register(qca, SPI_REG_INTR_CAUSE);

			//DEBUG_PRINT(GREEN_PHY_RX|GREEN_PHY_TX|DEBUG_ERROR_SEARCH,"[GreenPHY] %s got cause 0x%x\r\n",__func__,vInterruptCause);

			if (vInterruptCause & SPI_INT_CPU_ON) {
				DEBUG_PRINT(GREEN_PHY_RX|GREEN_PHY_FW_FEATURES|DEBUG_ERR," QCASPI_SYNC_CPUON\r\n");
				qcaspi_qca7k_sync(qca, QCASPI_SYNC_CPUON);
				/* not synced. */
				if (qca->sync != QCASPI_SYNC_READY)
					continue;

				intr_enable = (SPI_INT_CPU_ON | SPI_INT_PKT_AVLBL | SPI_INT_RDBUF_ERR | SPI_INT_WRBUF_ERR);
				//netif_carrier_on(qca->dev);
			}

			if (vInterruptCause & SPI_INT_RDBUF_ERR) {
				/* restart sync */
				DEBUG_PRINT(GREEN_PHY_RX|DEBUG_ERR," SPI_INT_RDBUF_ERR\r\n");
				// reset is needed !
				qcaspi_qca7k_set_sync(qca, QCASPI_SYNC_UNKNOWN);
				continue;
			}

			if (vInterruptCause & SPI_INT_WRBUF_ERR) {
				/* restart sync */
				DEBUG_PRINT(GREEN_PHY_TX|DEBUG_ERR," SPI_INT_WRBUF_ERR\r\n");
				// reset is needed !
				qcaspi_qca7k_set_sync(qca, QCASPI_SYNC_UNKNOWN);
				continue;
			}

			DEBUG_PRINT(GREEN_PHY_RX|GREEN_PHY_TX,"[GreenPHY] %s early clearing cause 0x%x\r\n",__func__,vInterruptCause);

			qcaspi_write_register(qca, SPI_REG_INTR_CAUSE, vInterruptCause);
			enable_spi_interrupts(qca, intr_enable);

			/* can only handle other interrupts if sync has occured */
			if (qca->sync == QCASPI_SYNC_READY) {
				if (vInterruptCause & SPI_INT_PKT_AVLBL) {
					DEBUG_PRINT(GREEN_PHY_RX,"%s RX\r\n",__func__);
					qcaspi_receive(qca);
				}
			}
		}

		if (qca->sync == QCASPI_SYNC_READY) {
#if GREEN_PHY_SIMPLE_QOS == ON
			if((qca->driver_state < BOOTLOADER_MODE) && (qca->driver_state_count < 10)) {
				qca->driver_state_count++;
				if(qca->driver_state == FIRMWARE_MODE_QUERY_SERIAL_OVER_ETH_VERSION) {
					DEBUG_PRINT(GREEN_PHY_FW_FEATURES, "qcaspi: %s FIRMWARE_MODE_QUERY_SERIAL_OVER_ETH_VERSION\r\n", __func__);
					struct netdeviceQueueElement * mme;
					mme = qcaspi_create_get_property_host_q_info(qca);
					qcaspi_tx_frame(qca, mme);
				} else if(qca->driver_state == INITIALIZED){
					DEBUG_PRINT(GREEN_PHY_FW_FEATURES, "qcaspi: %s INITIALIZED\r\n", __func__);
					qca->driver_state = SW_VERSION_QUERY;
					qca->driver_state_count = 0;
					struct netdeviceQueueElement * mme;
					mme = qcaspi_create_get_sw_version_mme(qca);
					qcaspi_tx_frame(qca, mme);
				} else if(qca->driver_state == SW_VERSION_QUERY){
					DEBUG_PRINT(GREEN_PHY_FW_FEATURES, "qcaspi: %s SW_VERSION_QUERY\r\n", __func__);
					if (qca->driver_state_count != 1)
					{
						struct netdeviceQueueElement * mme;
						mme = qcaspi_create_get_sw_version_mme(qca);
						qcaspi_tx_frame(qca, mme);
					}
					else
					{
						DEBUG_PRINT(GREEN_PHY_FW_FEATURES, "qcaspi: %s version queried\r\n", __func__);
					}
				}
			} else if((qca->driver_state < BOOTLOADER_MODE) && (qca->driver_state_count >= 10)){
				qca->driver_state = SERIAL_OVER_ETH_VER0_MODE;
				qca->driver_state_count = 0;
				//netif_tx_start_all_queues(qca->dev);
				DEBUG_PRINT(GREEN_PHY_FW_FEATURES, "qcaspi: %s SERIAL_OVER_ETH_VER0_MODE\r\n", __func__);
			}
#endif
			if (qcaspi_is_packet_available_for_transmit(qca))
			{
				DEBUG_PRINT(GREEN_PHY_TX,"%s TX\r\n",__func__);
				available = qcaspi_transmit(qca);
			}
		}
	}

	return 0;
}

/*-----------------------------------------------------------*/
