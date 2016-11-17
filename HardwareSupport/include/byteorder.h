/*
 * byteorder.h
 *
 *  Created on: Mar 22, 2012
 */

#ifndef BYTEORDER_H_
#define BYTEORDER_H_

#define BYTE_ORDER_LITTLE_ENDIAN

#ifdef BYTE_ORDER_LITTLE_ENDIAN
	#ifndef HTONS
		#define HTONS(n) (((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00) >> 8))
	#endif
	#define NTOHS(n) (((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00) >> 8))
	#define __cpu_to_le16(n) (n)
	#define __le16_to_cpu(n) (n)
	#define __cpu_to_be16(n) (((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00) >> 8))
	#define __be16_to_cpu(n) (((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00) >> 8))
	#define __cpu_to_be32(n) (((((unsigned long)((unsigned long)(n)>>24u) & 0xFF) | ( (unsigned long)((unsigned long)(n)>>8u)  & 0XFF00 )) | ( (unsigned long)((unsigned long)(n)<<8u)  & 0XFF0000 )) | ( (unsigned long)((unsigned long)(n)<<24u) & 0XFF000000 ))
	#define __be32_to_cpu(n) (((((unsigned long)((unsigned long)(n)>>24u) & 0xFF) | ( (unsigned long)((unsigned long)(n)>>8u)  & 0XFF00 )) | ( (unsigned long)((unsigned long)(n)<<8u)  & 0XFF0000 )) | ( (unsigned long)((unsigned long)(n)<<24u) & 0XFF000000 ))
	#define __cpu_to_le32(n) (n)
	#define __le32_to_cpu(n) (n)
#else
#error running on LPC1758 LE
#endif

#endif /* BYTEORDER_H_ */
