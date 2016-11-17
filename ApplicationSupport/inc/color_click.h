/*
 * color_click.h
 *
 *  Created on: 18.08.2015
 *      Author: Sebastian Sura
 */

#ifndef INC_COLOR_CLICK_H_
#define INC_COLOR_CLICK_H_

#define TCS3471_ENABLE_REG  0x00
#define TCS3471_ATIME_REG   0x01
#define TCS3471_WTIME_REG   0x03
#define TCS3471_AILTL_REG   0x04
#define TCS3471_AILTH_REG   0x05
#define TCS3471_AIHTL_REG   0x06
#define TCS3471_AIHTH_REG   0x07
#define TCS3471_PERS_REG    0x0C
#define TCS3471_CONFIG_REG  0x0D
#define TCS3471_CONTROL_REG 0x0F
#define TCS3471_ID_REG      0x12
#define TCS3471_STATUS_REG  0x13
#define TCS3471_CDATA_REG   0x14
#define TCS3471_CDATAH_REG  0x15
#define TCS3471_RDATA_REG   0x16
#define TCS3471_RDATAH_REG  0x17
#define TCS3471_GDATA_REG   0x18
#define TCS3471_GDATAH_REG  0x19
#define TCS3471_BDATA_REG   0x1A
#define TCS3471_BDATAH_REG  0x1B

// command register bits
#define TCS3471_COMMAND_BIT  0x80
#define TCS3471_AUTOINCR_BIT 0x20
#define TCS3471_SPECIAL_BIT  0x60
#define TCS3471_INTCLEAR_BIT 0x06

// enable register bits
#define TCS3471_AIEN_BIT     0x10
#define TCS3471_WEN_BIT      0x08
#define TCS3471_AEN_BIT      0x02
#define TCS3471_PON_BIT      0x01

// ID register values
#define TCS3471_1_5_VALUE    0x14
#define TCS3471_3_7_VALUE    0x1D

// status register bits
#define TCS3471_AINT_BIT     0x10
#define TCS3471_AVALID_BIT   0x01

// configuration register bits
#define TCS3471_WLONG_BIT    0x02

#define COLOR_I2C_ADDR 0x29

#define _ENABLE  0x80    // Enable status and interrupts
#define _ATIME   0x81    // RGBC ADC time
#define _WTIME   0x83    // Wait time
#define _AILTL   0x84    // RGBC Interrupt low treshold low byte
#define _AILTH   0x85    // RGBC Interrupt low treshold high byte
#define _AIHTL   0x86    // RGBC Interrupt high treshold low byte
#define _AIHTH   0x87    // RGBC Interrupt high treshold high byte
#define _PERS    0x8C    // Interrupt persistence filters
#define _CONFIG  0x8D    // Configuration
#define _CONTROL 0x8F    // Gain control register
#define _ID      0x92    // Device ID
#define _STATUS  0x93    // Device status
#define _CDATA   0x94    // Clear ADC low data register
#define _CDATAH  0x95    // Clear ADC high data register
#define _RDATA   0x96    // RED ADC low data register
#define _RDATAH  0x97    // RED ADC high data register
#define _GDATA   0x98    // GREEN ADC low data register
#define _GDATAH  0x99    // GREEN ADC high data register
#define _BDATA   0x9A    // BLUE ADC low data register
#define _BDATAH  0x9B    // BLUE ADC high data register
#define _COLOR_W_ADDRESS 0x52
#define _COLOR_R_ADDRESS 0x53

//#if COLOPORT == M1
//#define LED_R_PIN 31
//#define LED_R_PORT 1
//#define LED_G_PIN 2
//#define LED_B_PIN 4
//#define COLOR_INT_PIN 3
//#else
//#define LED_R_PIN 25
//#define LED_R_PORT 0
//#define LED_G_PIN 7
//#define LED_B_PIN 5
//#define COLOR_INT_PIN 6
//#endif


void init_color_sensor();

#endif /* INC_COLOR_CLICK_H_ */
