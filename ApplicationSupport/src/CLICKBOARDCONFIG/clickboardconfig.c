/*
 * clickboardconfig.c
 *
 *  Created on: 21.09.2015
 *      Author: Sebastian Sura
 */

#include "types.h"

typedef enum clickboard_t {
    NONE,
    HDC1000,
    RELAY,
    DALI,
    COLOR,
    UV,
    THERMO3,
    LCD,
    LAST
} clickboard_t;

int m1old, m2old;
extern config_t *gpconfig;

void setupclickboard(int port, int board) {
    switch (board) {
    case HDC1000:
        init_hdc1000_sensor(port);
        break;
    case RELAY:
        init_relay(port);
        break;
    case DALI:
        init_dali(port);
        break;
    case COLOR:
        init_color_sensor(port);
        break;
    case UV:
        init_uv_sensor(port);
        break;
    case THERMO3:
        init_thermo3_sensor(port);
        break;
    case LCD:
        LCD_Init();
        break;
    default:
        break;
    }
}

void deinit(int board) {
    switch (board) {
    case HDC1000:
        deinit_hdc1000_sensor();
        break;
    case RELAY:
        deinit_relay();
        break;
    case DALI:
        deinit_dali();
        break;
    case COLOR:
        deinit_color_sensor();
        break;
    case UV:
        deinit_uv_sensor();
        break;
    case THERMO3:
        //deinit_thermo3_sensor();
        break;
    case LCD:
        //deinit_lcd();
        break;
    default:
        break;
    }
}

void checkconfig(char *c) {

    /*Save last clickboardconfig for deinit*/
    m1old = gpconfig->M1;
    m2old = gpconfig->M2;

    if (strstr(c, "M1=hdc100")) {
        gpconfig->M1 = HDC1000;
    }
    if (strstr(c, "M1=relay")) {
        gpconfig->M1 = RELAY;
    }
    if (strstr(c, "M1=dali")) {
        gpconfig->M1 = DALI;
    }
    if (strstr(c, "M1=color")) {
        gpconfig->M1 = COLOR;
    }
    if (strstr(c, "M1=uv")) {
        gpconfig->M1 = UV;
    }
    if (strstr(c, "M1=thermo3")) {
        gpconfig->M1 = THERMO3;
    }
    if (strstr(c, "M1=lcd")) {
        gpconfig->M1 = LCD;
    }
    if (strstr(c, "M1=none")) {
        gpconfig->M1 = NONE;
    }


    if (strstr(c, "M2=hdc1000")) {
        gpconfig->M2 = HDC1000;
    }
    if (strstr(c, "M2=relay")) {
        gpconfig->M2 = RELAY;
    }
    if (strstr(c, "M2=dali")) {
        gpconfig->M2 = DALI;
    }
    if (strstr(c, "M2=color")) {
        gpconfig->M2 = COLOR;
    }
    if (strstr(c, "M2=uv")) {
        //m2 = 5;
        /*UVClick not supported on M2*/
        gpconfig->M2 = m2old;
    }
    if (strstr(c, "M2=thermo3")) {
        gpconfig->M2 = THERMO3;
    }
    if (strstr(c, "M2=lcd")) {
        gpconfig->M2 = LCD;
    }
    if (strstr(c, "M2=none")) {
        gpconfig->M2 = NONE;
    }

    if (gpconfig->M1 == 0 && gpconfig->M2 == 0) {
        deinit(m1old);
        deinit(m2old);
        writeflash();
        return;
    }

    /*Initialize Clickboards*/
    if (gpconfig->M1 != gpconfig->M2) {
        if (gpconfig->M1 != m1old) {
            deinit(m1old);
            setupclickboard(1, gpconfig->M1);
        } else {
            /*restore last config*/
            gpconfig->M1 = m1old;
        }
        if (gpconfig->M2 != m2old) {
            deinit(m2old);
            setupclickboard(2, gpconfig->M2);
        } else {
            /*restore last config*/
            gpconfig->M2 = m2old;
        }
    } else {
        /*restore last config*/
        gpconfig->M1 = m1old;
        gpconfig->M2 = m2old;
        return;
    }

    /*Save config in Flash*/
    writeflash();
}

