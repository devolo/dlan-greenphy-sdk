/*
 * types.h
 *
 *  Created on: 03.11.2015
 *      Author: Sebastian Sura
 */

#ifndef INC_TYPES_H_
#define INC_TYPES_H_

#include <stdint.h>
#include "uip.h"
#include "timer.h"
#include "psock.h"
#include "task.h"
#include "jansson.h"

struct MQTT_Queue_struct {
    char meaning[16];
    json_type type;
    union {
        int   i_val;
        float f_val;
        char  s_val[32];
    };
};

typedef struct __attribute__ ((packed)) config {
    uint32_t magicword;
    uint16_t length;
    uint8_t M1;
    uint8_t M2;
    uint8_t active;
    //uint8_t IP[4];
    char user[37];
    char password[13];
    char clientid[24];
    char hostname[65];
    char topic[46];
} config_t;

struct mqtt_state {
    struct pt pt;
    struct psock s;
    char state;
    struct uip_conn *conn;
    struct timer timer;
    u16_t ticks;
};

#endif /* INC_TYPES_H_ */
