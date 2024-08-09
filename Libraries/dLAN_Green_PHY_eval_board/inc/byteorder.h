/*
 * Copyright (c) 2016-2024, devolo solutions GmbH, Aachen, Germany.
 * All rights reserved.
 *
 * This Software is part of the devolo GreenPHY-SDK.
 *
 * Usage in source form and redistribution in binary form, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Usage in source form is subject to a current end user license agreement
 *    with the devolo solutions GmbH.
 * 2. Neither the name of the devolo solutions GmbH nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 3. Redistribution in binary form is limited to the usage on the GreenPHY
 *    module of the devolo solutions GmbH.
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
