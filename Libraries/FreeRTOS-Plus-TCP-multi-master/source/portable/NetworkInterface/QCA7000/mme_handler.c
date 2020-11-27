/* Standard includes. */
#include <stdint.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "FreeRTOS_Routing.h"
#include "NetworkBufferManagement.h"

/* Board includes */
#include "byteorder.h"

#include "qca_vs_mme.h"
#include "mme_handler.h"

static uint16_t usExpectedMME = 0;
static QueueHandle_t xMMEQueue = NULL;

NetworkBufferDescriptor_t *qcaspi_create_get_sw_version_mme( NetworkInterface_t *pxInterface, uint16_t *usResponseMMType )
{
	const TickType_t xDescriptorWaitTime = pdMS_TO_TICKS( 250 );
	NetworkBufferDescriptor_t *pxDescriptor;

	struct CCMMEFrame *pxMMEFrame;
	struct SwVerReq *sw_ver_req_mme;
	uint16_t usTemp;
	size_t usSize = 60;

	// _ML_ TODO: Get these from the pxEndpoint
	unsigned char target_mac_address[] = { 0x00, 0xB0, 0x52, 0x00, 0x00, 0x01 };
	unsigned char my_mac_address[] = { 0x00, 0xB0, 0x52, 0xA0, 0xB0, 0xC0 };


	pxDescriptor = pxGetNetworkBufferWithDescriptor( ipTOTAL_ETHERNET_FRAME_SIZE, xDescriptorWaitTime );
	if( pxDescriptor != NULL )
	{
		pxDescriptor->xDataLength = usSize;
		memset(pxDescriptor->pucEthernetBuffer, 0, usSize);

		pxMMEFrame = (struct CCMMEFrame *) pxDescriptor->pucEthernetBuffer;
		memcpy(pxMMEFrame->mODA, target_mac_address, sizeof(target_mac_address));
		memcpy(pxMMEFrame->mOSA, my_mac_address, sizeof(my_mac_address));

		usTemp = FreeRTOS_htons(eEtherTypeMME);
		memcpy(&pxMMEFrame->mRegular_V0.mEtherType, &usTemp, sizeof(usTemp));

		pxMMEFrame->mRegular_V0.mMMV = eMMVersion0;

		usTemp = __cpu_to_le16(eSwVerMMTypeReq);
		memcpy(&pxMMEFrame->mRegular_V0.mMMTYPE, &usTemp, sizeof(usTemp));

		sw_ver_req_mme = (struct SwVerReq*)(&pxMMEFrame->mRegular_V0.mMMEntry);

		sw_ver_req_mme->mOUI[0] = 0x00;
		sw_ver_req_mme->mOUI[1] = 0xB0;
		sw_ver_req_mme->mOUI[2] = 0x52;

		*usResponseMMType = eSwVerMMTypeCnf;
	}

	return pxDescriptor;
}

BaseType_t expecting_mme()
{
	BaseType_t rv = pdFAIL;

	if( usExpectedMME != 0 )
	{
		rv = pdPASS;
	}

	return rv;
}

BaseType_t is_frame_mme( struct CCMMEFrame *pxMMEFrame )
{
	BaseType_t rv = pdFAIL;
	uint16_t usEthType;

	memcpy(&usEthType, &pxMMEFrame->mRegular_V0.mEtherType, sizeof(pxMMEFrame->mRegular_V0.mEtherType));
	usEthType = FreeRTOS_ntohs(usEthType);
	if(usEthType == eEtherTypeMME)
	{
		rv = pdPASS;
	}

	return rv;
}

BaseType_t filter_rx_mme( NetworkBufferDescriptor_t *pxDescriptor )
{
	BaseType_t rv = pdFAIL;
	struct CCMMEFrame *pxMMEFrame;
	uint16_t usMMType;

	/* Check whether we're awaiting a MME and the MME Queue was initialised. */
	if( expecting_mme() && ( xMMEQueue != NULL ) )
	{
		pxMMEFrame = (struct CCMMEFrame *) pxDescriptor->pucEthernetBuffer;
		if( is_frame_mme( pxMMEFrame ) )
		{
			memcpy(&usMMType, &pxMMEFrame->mRegular_V0.mMMTYPE, sizeof(pxMMEFrame->mRegular_V0.mMMTYPE));
			usMMType = __le16_to_cpu(usMMType);
			if( usMMType == usExpectedMME )
			{
				xQueueSend( xMMEQueue, ( void * ) &pxDescriptor, ( TickType_t ) 0 );
				rv = pdPASS;
			}
		}
	}

	return rv;
}

NetworkBufferDescriptor_t *send_receive_mme_blocking( NetworkInterface_t *pxInterface, NetworkBufferDescriptor_t *pxDescriptor, uint16_t usResponseMMType )
{
	NetworkBufferDescriptor_t* pxRxDescriptor = NULL;
	const TickType_t xBlockTime = pdMS_TO_TICKS( 1000 );

	/* The caller of this method should ensure the frame is an MME. */
	configASSERT( is_frame_mme( (struct CCMMEFrame *) pxDescriptor->pucEthernetBuffer ) );

	/* If the MME Queue does not already exist, create it. */
	if( xMMEQueue == NULL )
	{
		xMMEQueue = xQueueCreate( 1, sizeof( struct AMessage * ) );
	}

	/* Store the expected MMType for the filter. */
	usExpectedMME = usResponseMMType;

	/* Output the MME on the Interface and wait for a response. */
	pxInterface->pfOutput( pxInterface, pxDescriptor, pdTRUE );
	xQueueReceive( xMMEQueue, &( pxRxDescriptor ), xBlockTime );

	/* Clean up */
	usExpectedMME = 0;

	return pxRxDescriptor;
}
