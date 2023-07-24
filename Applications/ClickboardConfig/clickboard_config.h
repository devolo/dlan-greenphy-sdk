/*
 * Copyright (c) 2023, devolo GmbH, Aachen, Germany.
 * All rights reserved.
 *
 * This Software is part of the devolo GreenPHY-SDK.
 *
 * Usage in source form and redistribution in binary form, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Usage in source form is subject to a current end user license agreement
 *    with the devolo GmbH.
 * 2. Neither the name of the devolo GmbH nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 3. Redistribution in binary form is limited to the usage on the GreenPHY
 *    module of the devolo GmbH.
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

#ifndef CLICKBOARD_CONFIG_H
#define CLICKBOARD_CONFIG_H

#define NUM_CLICKBOARD_PORTS   2

/*
 * Clickboard identifier. Numerical value is PID from MicroElektronika.
 */
typedef enum eCLICKBOARD_IDS {
	eClickboardIdNone    = 0,
	eClickboardIdExpand2 = 1838,
	eClickboardIdThermo3 = 1885,
	eClickboardIdColor2  = 1988
} eClickboardId_t;

/*
 * Clickboard initializer function. Called on activation.
 * Returns pdTRUE on success, otherwise pdFALSE.
 */
typedef BaseType_t ( * FClickboardInit ) ( const char *pcName, BaseType_t xPort );

/*
 * Clickboard deinitializer function. Called on deactivation.
 * Returns pdTRUE on success, otherwise pdFALSE.
 */
typedef BaseType_t ( * FClickboardDeinit ) ( void );

/*
 * A struct to hold information about a clickboard.
 */
typedef struct xCLICKBOARD
{
	eClickboardId_t xClickboardId; /* MikroE PID to identify the clickboard. */
	const char *pcName;         /* Lowercase clickboard name, without 'Click'
								suffix, e.g. 'Color2Click' -> 'color2' */
	FClickboardInit fClickboardInit;
	FClickboardDeinit fClickboardDeinit;
	uint8_t xPortsAvailable;    /* Ports on which a clickboard can be activated. */
	uint8_t xPortsActive;       /* Ports on which a clickboard is activated. */
} Clickboard_t;

/*
 * Initializer for the clickboard config interface. It activates any
 * preconfigured clickboard and adds a request handler to the HTTP server.
 */
void xClickboardsInit();

/*
 * Returns a clickboard with given name if found, otherwise NULL.
 */
Clickboard_t *pxFindClickboard( char *pcName );

/*
 * Returns a clickboard on a given port if found, otherwise NULL.
 */
Clickboard_t *pxFindClickboardOnPort( BaseType_t xPort );

/*
 * Activate a clickboard on a given port. Any active clickboard on the port is
 * deactivated first. To get a clickboard use pxFindClickboard first.
 * Return pdTRUE on success, otherwise pdFALSE.
 */
BaseType_t xClickboardActivate( Clickboard_t *pxClickboard, BaseType_t xPort );

/*
 * Deactivate a clickboard. To get a clickboard use pxFindClickboard first.
 * Return pdTRUE on success, otherwise pdFALSE.
 */
BaseType_t xClickboardDeactivate( Clickboard_t *pxClickboard );

#endif /* CLICKBOARD_CONFIG_H */
