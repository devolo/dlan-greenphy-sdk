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
 *
 * mmeHandling.h
 *
 */

#include <string.h>
#include <byteorder.h>
#include <qca_vs_mme.h>
#include <mmeHandling.h>
#include <debug.h>

#if (GREEN_PHY_SIMPLE_QOS == ON)

/* Simple QoS - The resource allocation ratio between different CAP
 * priorities.
 */
static const uint8_t target_priority_queue_size_ratio[QCAGP_NO_OF_QUEUES] = { 4, 1, 1, 1}; // increasing priority order

struct netdeviceQueueElement * qcaspi_create_get_sw_version_mme(struct qcaspi *qca)
{
	struct netdeviceQueueElement * rv = getQueueElement();

	if(rv)
	{
		uint8_t *  data = getDataFromQueueElement(rv);
		struct SwVerReq* sw_ver_req_mme;
		uint16_t temp_16;
		struct CCMMEFrame* mme_frame;
		unsigned char target_mac_address[] = { 0x00, 0xB0, 0x52, 0x00, 0x00, 0x01 };
		unsigned char my_mac_address[] = { 0x00, 0xB0, 0x52, 0xA0, 0xB0, 0xC0 };

		memset(data, 0, 60);
		setLengthOfQueueElement(rv, 60);

		mme_frame = (struct CCMMEFrame*)data;
		memcpy(mme_frame->mODA, target_mac_address, sizeof(target_mac_address));
		memcpy(mme_frame->mOSA, my_mac_address, sizeof(my_mac_address));

		temp_16 = HTONS(0x88e1);
		memcpy(&mme_frame->mRegular_V0.mEtherType, &temp_16, 2);

		mme_frame->mRegular_V0.mMMV = 0x0;

		temp_16 = __cpu_to_le16(MME_TYPE_VS_SW_VER_REQ);
		qca->expected_mme_to_drop = MME_TYPE_VS_SW_VER_CNF;
		memcpy(&mme_frame->mRegular_V0.mMMTYPE, &temp_16, 2);

		sw_ver_req_mme = (struct SwVerReq*)(&mme_frame->mRegular_V0.mMMEntry);

		sw_ver_req_mme->mOUI[0] = 0x00;
		sw_ver_req_mme->mOUI[1] = 0xB0;
		sw_ver_req_mme->mOUI[2] = 0x52;

	}

	return rv;
}

#if (GREEN_PHY_SIMPLE_QOS == ON)

struct netdeviceQueueElement * qcaspi_create_get_property_host_q_info(struct qcaspi *qca)
{
	struct netdeviceQueueElement * rv = getQueueElement();

	if(rv)
	{
		uint8_t * data = getDataFromQueueElement(rv);
		struct PropertyReq* property_req_mme;
		uint16_t temp_16;
		uint32_t temp;
		struct CCMMEFrame* mme_frame;
		unsigned char my_mac_address[] = { 0x00, 0xB0, 0x52, 0xA0, 0xB0, 0xC0 };

		memset(data, 0, 60);
		setLengthOfQueueElement(rv, 60);

		mme_frame = (struct CCMMEFrame*)data;

		memcpy(mme_frame->mODA, qca->target_mac_addr, sizeof(qca->target_mac_addr));
		memcpy(mme_frame->mOSA, my_mac_address, sizeof(my_mac_address));

		temp_16 = HTONS(0x88e1);
		memcpy(&mme_frame->mRegular_V0.mEtherType, &temp_16, 2);

		mme_frame->mRegular_V0.mMMV = 0x0;

		temp_16 = __cpu_to_le16(MME_TYPE_VS_GET_PROPERTY_REQ);
		qca->expected_mme_to_drop = MME_TYPE_VS_GET_PROPERTY_CNF;
		memcpy(&mme_frame->mRegular_V0.mMMTYPE, &temp_16, 2);

		property_req_mme = (struct PropertyReq*)(&mme_frame->mRegular_V0.mMMEntry);

		property_req_mme->mOUI[0] = 0x00;
		property_req_mme->mOUI[1] = 0xB0;
		property_req_mme->mOUI[2] = 0x52;

		temp = __cpu_to_le32(0x1);
		memcpy(&property_req_mme->mCookie, &temp, 4);

		property_req_mme->mPropertyFormat = 0x1;

		temp = __cpu_to_le32(4);
		memcpy(&property_req_mme->mPropertyStringLength, &temp, 4);

		property_req_mme->mFirstCharacterOfPropertyString = 105;
	}

	return rv;
}

#endif

/*
 * This function is used at the initial stage to determine if the QCA7000
 * device is operating in Bootloader mode/Serial Over Ethernet Ver0
 * mode/Serial Over Ethernet Ver1 mode
 *
 * Boot loader mode is used for flashing a new firmware/pib
 *
 * Serial Over Ethernet Ver0 mode - Simple QoS in QCA7000 is disabled.
 *
 * Serial Over Ethernet Ver1 mode - Simple QoS in QCA7000 is enabled. In this
 * mode, the classification engine in the QCA7000 is disabled. Instead the
 * Host does the classification of the traffic based on its needs.
 *
 * Returns -1 on error, 0 on success, 1 on expected MME.
 */

int process_rx_mme_frame(struct qcaspi* qca, uint8_t * data)
{
	int rv = 0;

	if(data)
	{
		struct SwVerCnf* sw_ver_cnf_mme;
		struct PropertyCnf* get_property_cnf_mme;
		struct HostQueueInfo* host_queue_info;
		struct CCMMEFrame* mme_frame;
		uint16_t temp_16;
		uint32_t temp_a, temp_b, temp, i, temp_total;
		uint16_t mme_type;

		mme_frame = (struct CCMMEFrame*)data;

		memcpy(&temp_16, &mme_frame->mRegular_V0.mMMTYPE, 2);
		mme_type = __le16_to_cpu(temp_16);


		if(qca->expected_mme_to_drop == mme_type)
		{
			rv = 1;
			qca->expected_mme_to_drop = 0;
		}

		switch(mme_type)
		{
			case MME_TYPE_VS_SW_VER_CNF:
				if(qca->driver_state < BOOTLOADER_MODE)
				{
					sw_ver_cnf_mme = (struct SwVerCnf*)(&mme_frame->mRegular_V0.mMMEntry);

					DEBUG_PRINT(GREEN_PHY_FW_FEATURES|DEBUG_INFO, "GreenPHY FW '%s'\r\n",sw_ver_cnf_mme->mVersion);

					if(memcmp(sw_ver_cnf_mme->mVersion, "BootLoader", sizeof("BootLoader")) == 0) {
						DEBUG_PRINT(GREEN_PHY_FW_FEATURES|DEBUG_INFO, "BOOTLOADER MODE\r\n");
						qca->driver_state = BOOTLOADER_MODE;
						//netif_tx_start_all_queues(qca->dev);
					} else {
						DEBUG_PRINT(GREEN_PHY_FW_FEATURES, "FIRMWARE MODE\r\n");
						if(memcmp(sw_ver_cnf_mme->mVersion, FIRMWARE_1_1_0_11, strlen(FIRMWARE_1_1_0_11)) == 0) {
					   		qca->driver_state = SERIAL_OVER_ETH_VER0_MODE;
					   		DEBUG_PRINT(GREEN_PHY_FW_FEATURES|DEBUG_ERR, "For "FIRMWARE_1_1_0_11" workaround to SERIAL_OVER_ETH_VER0_MODE\r\n");
						}
						else if(memcmp(sw_ver_cnf_mme->mVersion, FIRMWARE_1_1_0_01, strlen(FIRMWARE_1_1_0_01)) == 0) {
					   		qca->driver_state = SERIAL_OVER_ETH_VER0_MODE;
					   		DEBUG_PRINT(GREEN_PHY_FW_FEATURES|DEBUG_ERR, "For "FIRMWARE_1_1_0_01" workaround to SERIAL_OVER_ETH_VER0_MODE\r\n");
						}
						else
						{
				   			qca->driver_state = FIRMWARE_MODE_QUERY_SERIAL_OVER_ETH_VERSION;
							qca->driver_state_count = 0;
						}
						if(memcmp(qca->target_mac_addr, mme_frame->mOSA, 6) != 0)
							memcpy(qca->target_mac_addr, mme_frame->mOSA, 6);

					}
				}
			break;
			case MME_TYPE_VS_GET_PROPERTY_CNF:

				get_property_cnf_mme = (struct PropertyCnf*)(&mme_frame->mRegular_V0.mMMEntry);
				memcpy(&temp_a, &get_property_cnf_mme->mStatus, 4);
				temp_b = __le32_to_cpu(temp_a);
				if(temp_b == 0) { 	//Get property SUCCESS
					DEBUG_PRINT(GREEN_PHY_FW_FEATURES, "SERIAL_OVER_ETH_VER1_MODE\r\n");
					host_queue_info =
					    (struct HostQueueInfo*)(&get_property_cnf_mme->mFirstByteOfPropertyData);

					DEBUG_PRINT(GREEN_PHY_FW_FEATURES, "%s Max Size:%d\r\n %d %d %d %d\r\n",
						__func__,
						host_queue_info->mMaxQueueSize[0],
						host_queue_info->mCurrentQueueStatus[0],
						host_queue_info->mCurrentQueueStatus[1],
						host_queue_info->mCurrentQueueStatus[2],
						host_queue_info->mCurrentQueueStatus[3]);

					qca->total_credits = host_queue_info->mMaxQueueSize[0];
					qca->rxStatus.queue_sizes[0] = host_queue_info->mCurrentQueueStatus[0];
					qca->rxStatus.queue_sizes[1] = host_queue_info->mCurrentQueueStatus[1];
					qca->rxStatus.queue_sizes[2] = host_queue_info->mCurrentQueueStatus[2];
					qca->rxStatus.queue_sizes[3] = host_queue_info->mCurrentQueueStatus[3];

					if(qca->driver_state < BOOTLOADER_MODE)
					{
						/* Divide the total credits to the 4 queues in
						 * the ratio queue_size_ratio */
					 	temp_total = 0;
						temp = 0;
						for(i=0; i<QCAGP_NO_OF_QUEUES; i++)
							temp_total += target_priority_queue_size_ratio[i];

						for(i=0; i<(QCAGP_NO_OF_QUEUES-1); i++)
						{
							qca->max_queue_size[i] = (qca->total_credits * target_priority_queue_size_ratio[i])/temp_total;
							temp+= qca->max_queue_size[i];
						}
						/* the last one will have all the remaining */
						qca->max_queue_size[QCAGP_NO_OF_QUEUES-1] = qca->total_credits - temp;

						DEBUG_PRINT(GREEN_PHY_FW_FEATURES, "%s Queue size distribution: %d %d %d %d\r\n",
							__func__,
							qca->max_queue_size[0],
							qca->max_queue_size[1],
							qca->max_queue_size[2],
							qca->max_queue_size[3]);

			   			qca->driver_state = SERIAL_OVER_ETH_VER1_MODE;
						//netif_tx_start_all_queues(qca->dev);
					}

				} else {		//Get Property FAILURE
					if(qca->driver_state < BOOTLOADER_MODE)
					{
						//netif_tx_start_all_queues(qca->dev);
					}
			   		qca->driver_state = SERIAL_OVER_ETH_VER0_MODE;
			   		DEBUG_PRINT(GREEN_PHY_FW_FEATURES, "SERIAL_OVER_ETH_VER0_MODE\r\n");
				}
				break;

			case MME_TYPE_VS_HST_ACTION_IND:
				DEBUG_PRINT(GREEN_PHY_FW_FEATURES, "%s Host Action MME 0xA062\r\n", __func__);
				qca->driver_state = BOOTLOADER_MODE;
				//netif_tx_start_all_queues(qca->dev);
			break;
		    default:
		    	DEBUG_PRINT(GREEN_PHY_FW_FEATURES, "UNKNOWN MME TYPE!\r\n");
			break;
		}
	}
	else
	{
		rv = -1;
	}

	return rv;
}

int is_frame_mme(uint8_t * pFrame)
{
	int rv = 0;
	/* check the received frame ... if it is an MME, process it */
	uint16_t temp_s, eth_type;
	memcpy(&temp_s, ((void*)pFrame + 12), 2);
	eth_type = NTOHS(temp_s);
	if(eth_type == 0x88E1)
	{
		rv = 1;
	}

	return rv;
}

int expecting_mme(struct qcaspi* qca)
{
   int rv = qca->expected_mme_to_drop;
   return rv;
}

void filter_rx_mme(struct qcaspi* qca, struct netdeviceQueueElement **rxBuffer)
{
	if(expecting_mme(qca))
	{
		uint8_t * pFrame = getDataFromQueueElement(*rxBuffer);
		if(is_frame_mme(pFrame))
		{
			if(process_rx_mme_frame(qca, pFrame))
			{
				DEBUG_PRINT(GREEN_PHY_FW_FEATURES,"droped expected MME\r\n");
				returnQueueElement(rxBuffer);
			}
		}
	}
}

#endif
