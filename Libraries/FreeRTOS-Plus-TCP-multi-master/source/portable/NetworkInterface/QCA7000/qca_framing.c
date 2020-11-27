/*====================================================================*
 *
 *   Copyright (c) 2011, 2012, Atheros Communications Inc.
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
 *   qca_framing.c
 *
 *   Atheros ethernet framing. Every Ethernet frame is surrounded
 *   by an atheros frame while transmitted over a serial channel;
 *
 *--------------------------------------------------------------------*/

#include <string.h>

#include "qca_framing.h"

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

	buf[0] = 0xAA;
	buf[1] = 0xAA;
	buf[2] = 0xAA;
	buf[3] = 0xAA;
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
	buf[0] = 0x55;
	buf[1] = 0x55;
	return QCAFRM_FOOTER_LEN;
}

/*====================================================================*
 *
 *   QcaFrmAddQID
 *
 *   Fills a provided buffer with the Host Queue ID
 *
 * Return:   The number of bytes written.
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
 *   QcaFrmFsmInit
 *
 *   Initialize the framing handle. To be called at initialization for new QcaFrmHdl allocated.
 *
 *--------------------------------------------------------------------*/

void
QcaFrmFsmInit(QcaFrmHdl *frmHdl)
{
	frmHdl->state = QCAFRM_WAIT_AA1;
	frmHdl->offset = 0;
	frmHdl->len = 0;
}

uint16_t
QcaFrmBytesRequired( QcaFrmHdl *frmHdl )
{
	switch( frmHdl->state )
	{
	case QCAFRM_WAIT_AA1:        return QCAFRM_TOTAL_HEADER_LEN;
	case QCAFRM_WAIT_AA2:        return QCAFRM_HEADER_LEN - 1;
	case QCAFRM_WAIT_AA3:        return QCAFRM_HEADER_LEN - 2;
	case QCAFRM_WAIT_AA4:        return QCAFRM_HEADER_LEN - 3;
	case QCAFRM_WAIT_LEN_BYTE0:  return QCAFRM_HEADER_LEN - 4;
	case QCAFRM_WAIT_LEN_BYTE1:  return QCAFRM_HEADER_LEN - 5;
	case QCAFRM_WAIT_RSVD_BYTE1: return QCAFRM_HEADER_LEN - 6;
	case QCAFRM_WAIT_RSVD_BYTE2: return QCAFRM_HEADER_LEN - 7;
	default:                     return frmHdl->state - QCAFRM_FOOTER_LEN;
	case QCAFRM_WAIT_551:        return QCAFRM_FOOTER_LEN;
	case QCAFRM_WAIT_552:        return QCAFRM_FOOTER_LEN - 1;
	case QCAFRM_COMPLETE:        return 0;
	}
}

QcaFrmAction
QcaFrmGetAction( QcaFrmHdl *frmHdl )
{
	switch( frmHdl->state )
	{
	case QCAFRM_WAIT_AA1:
	case QCAFRM_WAIT_AA2:
	case QCAFRM_WAIT_AA3:
	case QCAFRM_WAIT_AA4:
	case QCAFRM_WAIT_LEN_BYTE0:
	case QCAFRM_WAIT_LEN_BYTE1:
	case QCAFRM_WAIT_RSVD_BYTE1:
	case QCAFRM_WAIT_RSVD_BYTE2:
		return QCAFRM_FIND_HEADER;
	default:
		return QCAFRM_COPY_FRAME;
	case QCAFRM_WAIT_551:
	case QCAFRM_WAIT_552:
		return QCAFRM_CHECK_FOOTER;
	case QCAFRM_COMPLETE:
		return QCAFRM_FRAME_COMPLETE;
	}
}

/*====================================================================*
 *
 *   QcaFrmFsmDecode
 *
 *   Gather received bytes and try to extract a full ethernet frame by following a simple state machine.
 *
 * Return:   QCAFRM_GATHER       No ethernet frame fully received yet.
 *           QCAFRM_NOHEAD       Header expected but not found.
 *           QCAFRM_INVLEN       Atheros frame length is invalid
 *           QCAFRM_NOTAIL       Footer expected but not found.
 *           > 0                 Number of byte in the fully received Ethernet frame
 *
 *--------------------------------------------------------------------*/

int32_t
QcaFrmFsmDecode( QcaFrmHdl *frmHdl, uint8_t recvByte, uint8_t *buffer )
{
int32_t ret = QCAFRM_GATHER;

	switch(frmHdl->state)
	{
	/* 4 bytes header pattern */
	case QCAFRM_COMPLETE:
	case QCAFRM_WAIT_AA1:
		if( recvByte == 0xAA )
		{
			frmHdl->state = QCAFRM_WAIT_AA2;
		}
		break;
	case QCAFRM_WAIT_AA2:
	case QCAFRM_WAIT_AA3:
	case QCAFRM_WAIT_AA4:
		if (recvByte != 0xAA)
		{
			ret = QCAFRM_NOHEAD;
			frmHdl->state = QCAFRM_WAIT_AA1;
		}
		else
		{
			frmHdl->state--;
		}
		break;
		/* 2 bytes length. */
 	case QCAFRM_WAIT_LEN_BYTE0:
 		frmHdl->len = recvByte;
 		frmHdl->state = QCAFRM_WAIT_LEN_BYTE1;
		break;
	case QCAFRM_WAIT_LEN_BYTE1:
		frmHdl->len = frmHdl->len | (recvByte << 8);
		frmHdl->state = QCAFRM_WAIT_RSVD_BYTE1;
		break;
	case QCAFRM_WAIT_RSVD_BYTE1:
		frmHdl->state = QCAFRM_WAIT_RSVD_BYTE2;
		break;
	case QCAFRM_WAIT_RSVD_BYTE2:
		if (frmHdl->len > QCAFRM_ETHMAXLEN || frmHdl->len < QCAFRM_ETHMINLEN)
		{
			ret = QCAFRM_INVLEN;
			frmHdl->state = QCAFRM_WAIT_AA1;
		}
		else
		{
			frmHdl->state = (QcaFrmState) (frmHdl->len + QCAFRM_FOOTER_LEN);
			frmHdl->offset = 0;
		}
		break;
	default:
		/* Receiving Ethernet frame itself. */
		buffer[frmHdl->offset] = recvByte;
		frmHdl->offset++;
		frmHdl->state--;
		break;
	case QCAFRM_WAIT_551:
		if (recvByte != 0x55)
		{
			ret = QCAFRM_NOTAIL;
			frmHdl->state = QCAFRM_WAIT_AA1;
		}
		else
		{
			frmHdl->state--;
		}
		break;
	case QCAFRM_WAIT_552:
		if (recvByte != 0x55)
		{
			ret = QCAFRM_NOTAIL;
			frmHdl->state = QCAFRM_WAIT_AA1;
		}
		else
		{
			/* Frame is fully received. */
			ret = frmHdl->len;
			frmHdl->state = QCAFRM_COMPLETE;
		}
		break;
	}

	return ret;
}

/*====================================================================*
 *
 *--------------------------------------------------------------------*/
