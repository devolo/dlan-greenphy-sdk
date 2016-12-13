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
 *   Atheros ethernet framing. Every Ethernet frame is surrounded
 *   by an atheros frame while transmitted over a serial channel;
 *
 *--------------------------------------------------------------------*/

#include "framingGreenPHY.h"
#include "debug.h"
#include "stddef.h"

/* QCA7k ethernet over SPI  header */
struct rx_header_s{
	uint32_t length;
	uint32_t startOfFrame;
	uint16_t packetLength;
	uint16_t reserved;
};

/*====================================================================*
 *
 *   QcaFrmCreateHeader
 *
 *   Fills a provided buffer with the Atheros frame header.
 *
 *   Return: The number of bytes written in header.
 *
 *--------------------------------------------------------------------*/

int32_t
QcaFrmCreateHeader(uint8_t *buf, uint16_t len, uint32_t protocol_version)
{
	len = __cpu_to_le16(len);

	buf[0] = QCAFRM_HEADER_SIGNATURE;
	buf[1] = QCAFRM_HEADER_SIGNATURE;
	buf[2] = QCAFRM_HEADER_SIGNATURE;
	buf[3] = QCAFRM_HEADER_SIGNATURE;
	buf[4] = (uint8_t)((len >> 0) & 0xFF);
	buf[5] = (uint8_t)((len >> 8) & 0xFF);
	buf[6] = 0;
	buf[7] = 0;

	if(protocol_version)
	{
		uint32_t temp = buf[5];
		buf[5] = (uint8_t)( (((protocol_version  << QCAFRM_VERSION_SHIFT) & QCAFRM_VERSION_MASK) | temp) & 0xFF );
		buf[6] = (uint8_t)( (protocol_version >> (8-QCAFRM_VERSION_SHIFT)) & 0xFF );
		buf[7] = (uint8_t)( (protocol_version >> (16-QCAFRM_VERSION_SHIFT)) & 0xFF );
	}

	return QCAFRM_HEADER_LEN;
}

/*====================================================================*
 *
 *   QcaFrmCreateFooter
 *
 *   Fills a provided buffer with the Atheros frame footer.
 *
 * Return:   The number of bytes written in footer.
 *
 *--------------------------------------------------------------------*/

int32_t
QcaFrmCreateFooter(uint8_t *buf)
{
	buf[0] = QCAFRM_TAIL_SIGNATURE;
	buf[1] = QCAFRM_TAIL_SIGNATURE;
	return QCAFRM_FOOTER_LEN;
}

/*====================================================================*
 *
 *   QcaFrmAddQID
 *
 *   Fills a provided buffer with the Host Queue ID
 *
 * Return:   The number of bytes written in footer.
 *
 *--------------------------------------------------------------------*/

int32_t
QcaFrmAddQID(uint8_t *buf, uint8_t qid)
{
	buf[0] = 0x00;
	buf[1] = qid;
	return QCAFRM_QID_LEN;
}

/*====================================================================*
 *
 *   QcaCheckLength
 *
 *   Checks the length of the received QCA7k header.
 *
 * Return:   1 on length ok, 0 otherwise.
 *
 *--------------------------------------------------------------------*/
static
int
QcaCheckLength(struct read_from_chip_s *rxStatus,struct rx_header_s * header)
{
	int rv = 0;

	uint32_t length = __be32_to_cpu(header->length);
	uint16_t packetLength = __le16_to_cpu(header->packetLength);

	if(packetLength>0x4000)
	{
		// there seems to be a bug in 1.1.0.11, SPI protocol version 1 is reported as 0x4000 in field packetLength
		packetLength -= 0x4000;

		if(length == packetLength+14)
		{
			// set version to 1
			rxStatus->protocol_version = 1;
			rv = 1;
		}
	}
	else
	{
		int overhead = 0;
		uint32_t protocol_version = (__be16_to_cpu(header->reserved) & QCAFRM_VERSION_MASK) >> QCAFRM_VERSION_SHIFT;

		switch(protocol_version)
		{
		case 0:
			overhead = 10;
			break;
		case 1:
			overhead = 14;
			break;
		default:
			DEBUG_PRINT(DEBUG_ERR,"Unknown protocol_version 0x%x\r\n",protocol_version);
			break;
		}
		if(overhead && length == packetLength+overhead)
		{
			rxStatus->protocol_version = protocol_version;
			rv = 1;
		}
	}

	return rv;
}

/*====================================================================*
 *
 *   QcaCheckHeader
 *
 *   Checks the received QCA7k header.
 *
 * Return:   0 on frame being received, -1 on sync neccessary.
 *
 *--------------------------------------------------------------------*/

static
int
QcaCheckHeader(struct read_from_chip_s *rxStatus)
{
	int rv = 0;
	struct rx_header_s * header = (struct rx_header_s *)(rxStatus->current_position - sizeof(struct rx_header_s));

	DEBUG_EXECUTE( \
			if(rxStatus->consecutivelyReceivedValidFrames == 0) \
			{ \
				DEBUG_DUMP(GREEN_PHY_FRAME_SYNC_STATUS|DEBUG_ERR,rxStatus->greenPhyRxBuffer,rxStatus->length,"RAW RX check async header"); \
			} \
			);

	if(header->startOfFrame == 0xaaaaaaaa && QcaCheckLength(rxStatus,header))
	{
		DEBUG_EXECUTE( \
				if(header->reserved != 0) \
				{ \
					DEBUG_PRINT(DEBUG_ERR,"Header version 0x%x\r\n",header->reserved); \
					DEBUG_PRINT(DEBUG_ERR,"Found protocol_version 0x%x\r\n",rxStatus->protocol_version); \
				} \
				);

		QcaSetRxStatus(rxStatus, receiveFrame);
	}
	else
	{
		DEBUG_PRINT(GREEN_PHY_FRAME_SYNC_STATUS|DEBUG_ERR," FRAME start sync, header error, consecutivelyReceivedValidFrames 0x%x\r\n",rxStatus->consecutivelyReceivedValidFrames);
		DEBUG_DUMP(GREEN_PHY_FRAME_SYNC_STATUS|DEBUG_ERR,rxStatus->greenPhyRxBuffer,rxStatus->length,"RAW RX header error");
		rv = -1;
		QcaSetRxStatus(rxStatus, sync);
	}
	return rv;
}

/*====================================================================*
 *
 *   QcaGetSpiProtocolVersion
 *
 *   Gets the SPI protocol version of the received frame.
 *
 * Return:   0 on success.
 *
 *--------------------------------------------------------------------*/
uint32_t  QcaGetSpiProtocolVersion(struct read_from_chip_s *rxStatus)
{
	uint32_t  rv = rxStatus->protocol_version;
	return rv;
}
/*====================================================================*
 *
 *   setProcessResultFunc
 *
 *   Sets the process_result fp. If there is nothing to read, it will
 *   be executed at once!
 *
 * Return:   0 on success.
 *
 *--------------------------------------------------------------------*/
int setProcessResultFunc(struct read_from_chip_s *rxStatus, qcaspi_process_result process_result)
{
	int rv = 0;
	rxStatus->process_result = process_result;
	if(!rxStatus->toRead)
	{
		DEBUG_PRINT(DEBUG_ALL," nothing to read, 0x%x() executed at once!\r\n",process_result);
		rv = process_result(rxStatus);
	}
	return rv;
}

static int QcaCheckSyncResult(struct read_from_chip_s *rxStatus);

/*====================================================================*
 *
 *   QcaSetRxStatusReadNumberOfBytes
 *
 *   Set the number of bytes to read to rxStatus to check the header.
 *
 * Return:   0 on success.
 *
 *--------------------------------------------------------------------*/
int QcaSetRxStatusReadNumberOfBytes(struct read_from_chip_s *rxStatus, uint32_t toRead)
{
	int rv = 0;
	rxStatus->toRead = toRead;
	DEBUG_PRINT(GREEN_PHY_FRAME_SYNC_STATUS|DEBUG_ERR," new toRead:0x%x\r\n",rxStatus->toRead);
	rv = setProcessResultFunc(rxStatus,QcaCheckSyncResult);
	return rv;
}

/*====================================================================*
 *
 *   QcaCheckSyncResult
 *
 *   Checks the QCA7k sync status result and set the next state to receive
 *   an ethernet frame.
 *
 * Return:   0 on success.
 *
 *--------------------------------------------------------------------*/

static
int
QcaCheckSyncResult(struct read_from_chip_s *rxStatus)
{
	DEBUG_DUMP(GREEN_PHY_FRAME_SYNC_STATUS,rxStatus->greenPhyRxBuffer,rxStatus->length,"GREEN PHY RAW RX check sync result");

	int rv = 0;

	uint32_t i;
	uint8_t * data;
	int syncFound;

	/* search the sync pattern, can be made faster by searching not 0xaa at data but 0xaa at (data+3) as first step */
	for(i=0,data = rxStatus->greenPhyRxBuffer,syncFound=0;i<rxStatus->length;i=i+1)
	{
		// we are searching 0xaaaaaaaa at byte borders, so, is it 0xaa?
		if(*data == 0xaa)
		{
			syncFound += 1;
		}
		else
		{
			syncFound = 0;
		}
		data += 1;
		if(syncFound>3)
		{
			// this was the pattern!
			break;
		}
	}

	switch(syncFound)
	{
	// nothing found, carry on ...
		case 0:
			rv = QcaSetRxStatus(rxStatus, sync);
			break;
	// pattern partly found, read some more bytes ...
		case 1:
			DEBUG_DUMP(DEBUG_ERR,rxStatus->greenPhyRxBuffer,rxStatus->length,"AA");
			//QcaSetRxStatusReadNumberOfBytes(rxStatus,sizeof(struct rx_header_s)-(offsetof(struct rx_header_s,startOfFrame)+sizeof(uint32_t))+3);
			QcaSetRxStatusReadNumberOfBytes(rxStatus,2*sizeof(struct rx_header_s));
			break;
		case 2:
			DEBUG_DUMP(DEBUG_ERR,rxStatus->greenPhyRxBuffer,rxStatus->length,"AAAA");
			//QcaSetRxStatusReadNumberOfBytes(rxStatus,sizeof(struct rx_header_s)-(offsetof(struct rx_header_s,startOfFrame)+sizeof(uint32_t))+2);
			QcaSetRxStatusReadNumberOfBytes(rxStatus,2*sizeof(struct rx_header_s));
			break;
		case 3:
			DEBUG_DUMP(DEBUG_ERR,rxStatus->greenPhyRxBuffer,rxStatus->length,"AAAAAA");
			//QcaSetRxStatusReadNumberOfBytes(rxStatus,sizeof(struct rx_header_s)-(offsetof(struct rx_header_s,startOfFrame)+sizeof(uint32_t))+1);
			QcaSetRxStatusReadNumberOfBytes(rxStatus,2*sizeof(struct rx_header_s));
			break;
	// pattern found, can we check it?
		default:
		{
			int offset = (rxStatus->greenPhyRxBuffer+rxStatus->length-data);
			DEBUG_PRINT(GREEN_PHY_FRAME_SYNC_STATUS|DEBUG_ERR," FRAME foundPatternAAAAAAAA\r\n  i:0x%x data:0x%x offset:0x%x(0x%x)\r\n",i,data,offset,(rxStatus->greenPhyRxBuffer+rxStatus->length-data));
			DEBUG_PRINT(GREEN_PHY_FRAME_SYNC_STATUS|DEBUG_ERR," current_position:0x%x\r\n",rxStatus->current_position);
			DEBUG_DUMP(DEBUG_ERR,rxStatus->greenPhyRxBuffer,rxStatus->length,"AAAAAAAA");
			if(offset == 0)
			{
				// pattern is found at the end of the data stream, so read on the rest of the header
				//QcaSetRxStatusReadNumberOfBytes(rxStatus,sizeof(struct rx_header_s)-(offsetof(struct rx_header_s,startOfFrame)+sizeof(uint32_t)));
				QcaSetRxStatusReadNumberOfBytes(rxStatus,sizeof(struct rx_header_s));
			}
			else if(offset >= 4)
			{
				// set current position to the end of the header. This will be checked in state 'receiveFrame'.
				rxStatus->current_position = data + sizeof(struct rx_header_s) - offsetof(struct rx_header_s,startOfFrame) - sizeof(uint32_t) ;
				DEBUG_PRINT(GREEN_PHY_FRAME_SYNC_STATUS|DEBUG_ERR," new current_position:0x%x\r\n",rxStatus->current_position);
				// the header is complete, just check it
				QcaCheckHeader(rxStatus);
			}
			else
			{
				// pattern is found at the end of the data stream, some data is behind and the header is not complete, so read on the rest of the header
				//QcaSetRxStatusReadNumberOfBytes(rxStatus,sizeof(uint32_t)-offset);
				QcaSetRxStatusReadNumberOfBytes(rxStatus,2*sizeof(struct rx_header_s));
			}
		}
		break;
	}

	return rv;
}

/*====================================================================*
 *
 *   QcaCheckReceivedFrame
 *
 *   Checks the received ethernet frame.
 *
 * Return:   1 on success, otherwise sync is lost.
 *
 *--------------------------------------------------------------------*/

static int QcaCheckReceivedFrame(struct read_from_chip_s *rxStatus)
{
	int rv = 0;
	uint32_t length = rxStatus->length;
	uint8_t EOF_2 =  rxStatus->greenPhyRxBuffer[length-1];
	uint8_t EOF_1 =  rxStatus->greenPhyRxBuffer[length-2];

	if( EOF_1 == 0x55 && EOF_2 == 0x55)
	{
		DEBUG_EXECUTE(rxStatus->consecutivelyReceivedValidFrames+=1);
		rv = 1;
		rxStatus->frameLength = length-2;
		QcaSetRxStatus(rxStatus, receiveHeader);
	}
	else
	{
		DEBUG_PRINT(GREEN_PHY_FRAME_SYNC_STATUS|DEBUG_ERR," FRAME start sync no EOF consecutivelyReceivedValidFrames 0x%x\r\n",rxStatus->consecutivelyReceivedValidFrames);
		DEBUG_EXECUTE(rxStatus->consecutivelyReceivedValidFrames = 0);
		DEBUG_DUMP(DEBUG_ERR,rxStatus->greenPhyRxBuffer,rxStatus->length,"RAW RX no EOF");
		// recheck the received data to find another start pattern ...
		rv = QcaCheckSyncResult(rxStatus);
	}

	return rv;
}

/*====================================================================*
 *
 *   QcaSetRxStatus
 *
 *   Sets rxStatus on base of the provided state.
 *
 * Return:   0 on success.
 *
 *--------------------------------------------------------------------*/

int QcaSetRxStatus(struct read_from_chip_s *rxStatus, receiveState_t state)
{
	int rv = 0;
	rxStatus->state = state;

	switch(rxStatus->state)
	{
	case sync:
		DEBUG_PRINT(GREEN_PHY_FRAME_SYNC_STATUS," FRAME sync\r\n");
		rxStatus->toRead = sizeof(struct rx_header_s);
		if(rxStatus->length > sizeof(struct rx_header_s))
		{
			rxStatus->length = 0;
			rxStatus->current_position = rxStatus->greenPhyRxBuffer;
		}
		DEBUG_EXECUTE(rxStatus->consecutivelyReceivedValidFrames=0);
		rv = setProcessResultFunc(rxStatus,QcaCheckSyncResult);
		break;
	case init:
		DEBUG_PRINT(GREEN_PHY_FRAME_SYNC_STATUS|DEBUG_ERR," FRAME init\r\n");
		/*no break*/
	case receiveHeader:
		DEBUG_PRINT(GREEN_PHY_FRAME_SYNC_STATUS," FRAME receiveHeader\r\n");
		rxStatus->length = 0;
		rxStatus->toRead = sizeof(struct rx_header_s);
		rxStatus->current_position = rxStatus->greenPhyRxBuffer;
		rv = setProcessResultFunc(rxStatus,QcaCheckHeader);
		DEBUG_EXECUTE(if(rxStatus->consecutivelyReceivedValidFrames == 1) DEBUG_PRINT(GREEN_PHY_FRAME_SYNC_STATUS|DEBUG_ERR,"\r\n\r\nFRAME sync!\r\n\r\n"));
		break;
	case receiveFrame:
		DEBUG_PRINT(GREEN_PHY_FRAME_SYNC_STATUS," FRAME receiveFrame\r\n");
		struct rx_header_s * header = (struct rx_header_s *)(rxStatus->current_position - sizeof(struct rx_header_s));
		rxStatus->toRead = __be32_to_cpu(header->length) - 8; // the length and the sync pattern is already read
		// is it a length we could handle?
		if(rxStatus->toRead && rxStatus->toRead<=ETH_MAX_FLEN)
		{
			// read in the complete frame
			uint32_t delta = (rxStatus->current_position-rxStatus->greenPhyRxBuffer);
			rxStatus->length = rxStatus->length-delta;
			if(rxStatus->length)
			{
				if(rxStatus->length>rxStatus->toRead)
				{
					DEBUG_PRINT(GREEN_PHY_FRAME_SYNC_STATUS|DEBUG_ERR," length reduced\r\n");
					// unfortunately, this can happen on sync loss, so drop the rest of the data stream
					rxStatus->length = rxStatus->toRead;
				}
				int i;
				for(i=0;i<rxStatus->length;i+=1)
				{
					// just a copy
					rxStatus->greenPhyRxBuffer[i] = rxStatus->current_position[i];
				}
				rxStatus->toRead -= rxStatus->length;
				DEBUG_DUMP(GREEN_PHY_FRAME_SYNC_STATUS|DEBUG_ERR,rxStatus->greenPhyRxBuffer,rxStatus->length,"RAW RX after copy");
			}
			rxStatus->current_position = rxStatus->greenPhyRxBuffer+rxStatus->length;
			rv = setProcessResultFunc(rxStatus,QcaCheckReceivedFrame);
		}
		else
		{
			// packetLength is not valid -> sync again
			DEBUG_PRINT(GREEN_PHY_FRAME_SYNC_STATUS|DEBUG_ERR," FRAME start sync invalid packetLength consecutivelyReceivedValidFrames 0x%x\r\n",rxStatus->consecutivelyReceivedValidFrames);
			DEBUG_DUMP(GREEN_PHY_FRAME_SYNC_STATUS|DEBUG_ERR,rxStatus->greenPhyRxBuffer,rxStatus->length,"RAW RX invalid packetLength");
			DEBUG_EXECUTE(rxStatus->consecutivelyReceivedValidFrames = 0);
			rv = QcaSetRxStatus(rxStatus,sync);
		}
	break;
	default:
		DEBUG_PRINT(DEBUG_ERR|DEBUG_ALL," %s default\r\n",__func__);
		break;

	}

	return rv;
}

/*====================================================================*
 *
 *--------------------------------------------------------------------*/
