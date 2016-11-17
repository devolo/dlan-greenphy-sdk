/*
 * apphandler.c
 *
 *  Created on: 10.09.2015
 *      Author: Sebastian Sura
 */

#include "uip.h"

/********************************************************************//**
 * @brief		looks for dest. port and decides which function to call
 * @param[in]	None
 * @return 		None
 *********************************************************************/
void apphandler_tcp(void) {

    if (uip_conn->rport == HTONS(1883)) {
        mqtt_appcall();
    } else {
        switch (uip_conn->lport) {
        case HTONS(80):
            httpd_appcall();
            break;
        case HTONS(1883):
            //mqtt_appcall();
            break;
        }
    }
}

void apphandler_udp(void) {

    switch (uip_udp_conn->rport) {
    case HTONS(67):
        dhcpc_appcall();
        return;
    case HTONS(53):
        resolv_appcall();
        return;
        break;
    }

    switch (uip_udp_conn->lport) {
    case HTONS(68):
        dhcpc_appcall();
        return;
    case HTONS(53):
        resolv_appcall();
        return;
    }

    //dhcpc_appcall();
}
