/*
 * netdevice_CLI.h
 *
 *  Created on: 28.03.2013
 */

#ifndef NETDEVICE_CLI_H_
#define NETDEVICE_CLI_H_

#include <netdevice.h>

struct command * netdeviceInit_CLI(void);
void netdeviceAdd(struct netDeviceInterface * device, char * name);

#endif /* DEBUGLEVEL_H_ */
