/*
 * relay_click.h
 *
 *  Created on: 17.08.2015
 *      Author: Sebastian Sura
 */

#ifndef INC_RELAY_CLICK_H_
#define INC_RELAY_CLICK_H_


//#if RELAYPORT == M1
//#define RELAY1 4
//#define RELAY2 2
//#else
//#define RELAY1 5
//#define RELAY2 7
//#endif

void init_relay();
void relay_on(int relay);
void relay_off(int relay);

#endif /* INC_RELAY_CLICK_H_ */
