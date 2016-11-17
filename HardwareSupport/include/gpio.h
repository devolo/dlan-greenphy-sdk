/*
 * gpio.h
 *
 *  Created on: 16.08.2012
 */

#ifndef GPIO_H_
#define GPIO_H_

struct GPIO{
	uint8_t port;
	uint8_t pin;
};

int printGpio(struct GPIO * gpio);

#endif /* GPIO_H_ */
