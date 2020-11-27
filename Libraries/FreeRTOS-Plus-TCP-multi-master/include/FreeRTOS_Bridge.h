/*
 * FreeRTOS+TCP Labs Build 160919 (C) 2016 Real Time Engineers ltd.
 * Authors include Hein Tibosch and Richard Barry
 *
 *******************************************************************************
 ***** NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ***
 ***                                                                         ***
 ***                                                                         ***
 ***   FREERTOS+TCP IS STILL IN THE LAB (mainly because the FTP and HTTP     ***
 ***   demos have a dependency on FreeRTOS+FAT, which is only in the Labs    ***
 ***   download):                                                            ***
 ***                                                                         ***
 ***   FreeRTOS+TCP is functional and has been used in commercial products   ***
 ***   for some time.  Be aware however that we are still refining its       ***
 ***   design, the source code does not yet quite conform to the strict      ***
 ***   coding and style standards mandated by Real Time Engineers ltd., and  ***
 ***   the documentation and testing is not necessarily complete.            ***
 ***                                                                         ***
 ***   PLEASE REPORT EXPERIENCES USING THE SUPPORT RESOURCES FOUND ON THE    ***
 ***   URL: http://www.FreeRTOS.org/contact  Active early adopters may, at   ***
 ***   the sole discretion of Real Time Engineers Ltd., be offered versions  ***
 ***   under a license other than that described below.                      ***
 ***                                                                         ***
 ***                                                                         ***
 ***** NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ***
 *******************************************************************************
 *
 * FreeRTOS+TCP can be used under two different free open source licenses.  The
 * license that applies is dependent on the processor on which FreeRTOS+TCP is
 * executed, as follows:
 *
 * If FreeRTOS+TCP is executed on one of the processors listed under the Special
 * License Arrangements heading of the FreeRTOS+TCP license information web
 * page, then it can be used under the terms of the FreeRTOS Open Source
 * License.  If FreeRTOS+TCP is used on any other processor, then it can be used
 * under the terms of the GNU General Public License V2.  Links to the relevant
 * licenses follow:
 *
 * The FreeRTOS+TCP License Information Page: http://www.FreeRTOS.org/tcp_license
 * The FreeRTOS Open Source License: http://www.FreeRTOS.org/license
 * The GNU General Public License Version 2: http://www.FreeRTOS.org/gpl-2.0.txt
 *
 * FreeRTOS+TCP is distributed in the hope that it will be useful.  You cannot
 * use FreeRTOS+TCP unless you agree that you use the software 'as is'.
 * FreeRTOS+TCP is provided WITHOUT ANY WARRANTY; without even the implied
 * warranties of NON-INFRINGEMENT, MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. Real Time Engineers Ltd. disclaims all conditions and terms, be they
 * implied, expressed, or statutory.
 *
 * 1 tab == 4 spaces!
 *
 * http://www.FreeRTOS.org
 * http://www.FreeRTOS.org/plus
 * http://www.FreeRTOS.org/labs
 *
 */

#ifndef FREERTOS_BRIDGE_H
#define FREERTOS_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS_Routing.h"

/* Age value for forwarding table entries that do not expire. */
#define INFINITE_FORWARDING_TABLE_AGE ( 255 )

/*
 *
 */
void vRefreshForwardingTableEntry( const MACAddress_t *pxMACAddress, NetworkInterface_t *pxInterface, uint8_t ucAge );

/*
 * Reduce the age count in each entry within the forwarding table.  An entry is
 * no longer considered valid and is deleted if its age reaches zero.
 */
void vAgeForwardingTable( void );

/*
 * Get the first bridged interface.
 */
NetworkInterface_t *FreeRTOS_FirstNetworkInterfaceInBridge( void );

/*
 * Get the next bridged interface.
 */
NetworkInterface_t *FreeRTOS_NextNetworkInterfaceInBridge( NetworkInterface_t *pxInterface );

/*
 * Forward a frame to the bridged interfaces. Returns pdTRUE if the frame was
 * successfully forwarded to one or multiple interfaces and pdFALSE if not. If
 * the frame could not be forwarded it is the caller's responsibility to return
 * the network buffer.
 */
BaseType_t xBridge_Process( NetworkBufferDescriptor_t * const pxNetworkBuffer );

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* FREERTOS_BRIDGE_H */

