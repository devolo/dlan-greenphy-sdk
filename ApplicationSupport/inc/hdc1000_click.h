/*
 * hdc1000_click.h
 *
 *  Created on: 17.08.2015
 *      Author: Sebastian Sura
 */

#ifndef INC_HDC1000_CLICK_H_
#define INC_HDC1000_CLICK_H_

#define MAN_ID_REG 0xFE //Manufacturer ID Register
#define TEMP 0
#define TEMP_REG 0x00 //Temperatur Register
#define HUM 1
#define HUM_REG 0x01 //Humidity Register
#define HDC1000_I2C_ADDR 0x40 //I2C Address of HDC1000 Clickboard
#define THERMO3_I2C_ADDR 0x48 //I2C Address of HDC1000 Clickboard

void init_hdc1000_sensor(int i);
void init_thermo3_sensor(int i);

#endif /* INC_HDC1000_CLICK_H_ */
