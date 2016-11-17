/*
 * saveconfig.c
 *
 *  Created on: 29.09.2015
 *      Author: Sebastian Sura
 */

#include "LPC17xx.h"
#include "lpc_types.h"
#include "lpc17xx_iap.h"
#include "string.h"
#include "types.h"
#include "uip.h"
//#include "lpc17xx_libcfg.h"


#define FLASH_PROG_AREA_START       0x78000
#define FLASH_PROG_AREA_SIZE		0x8000
#define FLASH_BLOCKS 8
#define MAGIC_START 0x12101988

/** The origin buffer on RAM */
#define BUFF_SIZE           512

extern config_t *gpconfig;
extern uip_ipaddr_t mqtt_addr;

/********************************************************************//**
 * @brief		Searches for the last valid Data-block in Sector 29 of Flash
 * @param[in]	None
 * @return 		Pointer to 'MagicWord' of last valid Data-block
 * 				0 if no valid Data was found
 *********************************************************************/
uint8_t* findmagic(int lastfree) {
    config_t *magic = (config_t*) FLASH_PROG_AREA_START;
    config_t *temp;
    void *ptr = magic;
    int i;

    /*Look for last saved config*/
    if (magic->magicword != MAGIC_START)
        return 0;

    for (i = 0; i < FLASH_BLOCKS - 1; i++) {
        ptr += 4096;
        temp = ptr;
        if (temp->magicword != MAGIC_START)
            break;
        magic = ptr;
    }

    if (lastfree) {
        if (magic == (void *)0x7f000)
            temp = (void *)FLASH_PROG_AREA_START;
        return (uint8_t*) temp;
    }
    return (uint8_t*) magic;
}

/********************************************************************//**
 * @brief       Checks if pointer is a hostname or IP address
 * @param[in]   Pointer to Hostname
 * @return      1 if Hostname, 0 of IP Address
 *********************************************************************/
int ishostname(char *host) {

    char string[] = "abcdefghijklmnopqrstuvwxyz";

    if ( strchr(host, '.') && strpbrk(host, string))
        return 1;
    return 0;
}

/********************************************************************//**
 * @brief       Converts a String to uip_ip4addr_t
 * @param[in]   Pointer to String, Pointer to uip_ip4_addr_t to fill in
 * @return      None
 *********************************************************************/
void stringtoip(char *string, uip_ip4addr_t *ip) {

    char *ptr;
    unsigned char byte[4] = { 0 };
    uip_ipaddr_t tempip;
    size_t index = 0;

    ptr = string;
            while (*string) {
                if (isdigit((unsigned char) *string)) {
                    byte[index] *= 10;
                    byte[index] += *string - '0';
                } else {
                    index++;
                }
                string++;
            }
            uip_ipaddr(tempip, byte[0], byte[1], byte[2], byte[3]);
            uip_ipaddr_copy(ip, tempip);
}

/********************************************************************//**
 * @brief       Reads the last valid values in flash and initializes Task
 * @param[in]   None
 * @return      None
 *********************************************************************/
void readflash() {

    config_t *ptr = (config_t*) findmagic(0);
    gpconfig = malloc(sizeof(config_t));
    memset(gpconfig, '\0', sizeof(config_t));

    /*Do nothing if no valid config was found*/
    if (!ptr)
        return;

    //Copy config from flash to heap
    memcpy(gpconfig, ptr, sizeof(config_t));

#if HTTP_SERVER == ON
    //Initialize Clickboard
    if (gpconfig->M1 >= 0 && gpconfig->M1 < 8 && gpconfig->M2 >= 0
            && gpconfig->M2 < 8) {


        setupclickboard(1, gpconfig->M1);
        setupclickboard(2, gpconfig->M2);

        if (gpconfig->active == 1) {
            if (ishostname(gpconfig->hostname)) {
            	//init_mqtt();
                resolv_query(gpconfig->hostname);
            } else {
                stringtoip(gpconfig->hostname, mqtt_addr);
                init_mqtt();
            }
        }
    }
#endif

}

/********************************************************************//**
 * @brief       Erases Sector 29 of Flash
 * @param[in]   None
 * @return      None
 *********************************************************************/
void eraseflash() {

    uint32_t result[4];
    EraseSector(29, 29);
    BlankCheckSector(29, 29, &result[0], &result[1]);
}

/********************************************************************//**
 * @brief       Looks for free space in flash to save current config
 *              if no free space is available it erases Sector 29
 * @param[in]   None
 * @return      None
 *********************************************************************/
int writeflash() {

    IAP_STATUS_CODE status;
    char buffer[BUFF_SIZE];
    uint8_t *ptr;

    /*Get pointer to last valid Block*/
    ptr = findmagic(1);
    if (!ptr)
        ptr = (uint8_t*) FLASH_PROG_AREA_START;

    /*If no Data was found erase sector or 256-block does not fit to sector*/
    if (ptr == (uint8_t*) FLASH_PROG_AREA_START
            || ptr
                    > (uint8_t*) (FLASH_PROG_AREA_START + FLASH_PROG_AREA_SIZE
                            - 4095)) {
        eraseflash();
        ptr = (uint8_t*) FLASH_PROG_AREA_START;
    }

    /*Save config to Buffer first*/
    memset(buffer, '\0', sizeof(buffer));
    gpconfig->magicword = MAGIC_START;
    gpconfig->length = 1552;
    //gpconfig->M1 = (uint8_t) getm1();
    //gpconfig->M2 = (uint8_t) getm2();
    memcpy(&buffer, gpconfig, sizeof(config_t));

    /*write Buffer to Flash*/
    status = CopyRAM2Flash(ptr, buffer, IAP_WRITE_4096);

    return 1;
}

/********************************************************************//**
 * @brief       Looks in String C for String A and replaces A with B
 * @param[in]   String A, String B, String C
 * @return      Pointer to replaced String
 *********************************************************************/
char* replace_string(char *search, char *replace, char *string) {
    char *tempString, *searchStart;
    int len = 0;

    // preuefe ob Such-String vorhanden ist
    searchStart = strstr(string, search);
    if (searchStart == NULL) {
        return string;
    }

    while (searchStart != NULL) {

        tempString = (char*) malloc(strlen(string) * sizeof(char));
        if (tempString == NULL) {
            return NULL;
        }
        strcpy(tempString, string);

        len = searchStart - string;
        string[len] = '\0';

        strcat(string, replace);

        len += strlen(search);
        strcat(string, (char*) tempString + len);

        free(tempString);

        searchStart = strstr(string, search);
    }

    return string;

}

/********************************************************************//**
 * @brief       Differentiates if hostname or ip Adress is provided and
 *              fills global uip_ip4addr_t structure
 * @param[in]   Pointer to Hostname/IP
 * @return      None
 *********************************************************************/
void parse_host(char* c) {

    if ((strlen(c) < 6) || strlen(c) > 45)
        return;

    if (strcmp(c, gpconfig->hostname) == 0)
            return;

    //check for host or ip
    if (ishostname(c)) {
        //close_conn();
        resolv_query(c);
        strncpy(gpconfig->hostname, c, 64);
    } else {
        //String to IP
        stringtoip(c, mqtt_addr);
        strncpy(gpconfig->hostname, c, 64);
    }

    writeflash();
}

/********************************************************************//**
 * @brief       Checks if valid username is submitted and saves to flash
 * @param[in]   Pointer to username
 * @return      None
 *********************************************************************/
void parse_user(char* c) {

    if (strlen(c) < 36)
        return;
    strncpy(gpconfig->user, c, 36);
    gpconfig->user[36] = '\0';
    //cat topic
    memset(gpconfig->topic, '\0', sizeof(gpconfig->topic));
    strcpy(gpconfig->topic, "/v1/");
    strncat(gpconfig->topic, gpconfig->user, 36);
    strcat(gpconfig->topic, "/data");

    writeflash();
}

/********************************************************************//**
 * @brief       Checks if valid password is submitted and saves to flash
 * @param[in]   Pointer to password
 * @return      None
 *********************************************************************/
void parse_password(char * c) {

    if (strlen(c) < 8)
        return;
    strncpy(gpconfig->password, c, 12);
    gpconfig->password[12] = '\0';
    writeflash();
}

/********************************************************************//**
 * @brief       Checks if valid Client ID is submitted and saves to flash
 * @param[in]   Pointer to Client ID
 * @return      None
 *********************************************************************/
void parse_clientid(char *c) {

    if (strlen(c) < 23)
        return;
    c = replace_string("%2F", "/", c);
    c = replace_string("%2B", "+", c);
    strncpy(gpconfig->clientid, c, 23);
    gpconfig->clientid[23] = '\0';
    writeflash();
}

/********************************************************************//**
 * @brief       Checks if valid topic is submitted and saves to flash
 *              concat "/data" to it to fit relayrs configuration
 * @param[in]   Pointer to topic
 * @return      None
 *********************************************************************/
void parse_topic(char *c) {

    if (strlen(c) < 41)
        return;
    c = replace_string("%2F", "/", c);
    c = replace_string("%2B", "+", c);
    strncpy(gpconfig->topic, c, 41);
    strcat(gpconfig->topic, "data");
    gpconfig->topic[45] = '\0';
    writeflash();
}
