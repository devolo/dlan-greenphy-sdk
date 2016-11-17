/*
 * CLI_interface.h
 *
 *  Created on: 16.08.2012
 */

#ifndef CLI_INTERFACE_H_
#define CLI_INTERFACE_H_

struct command{
	int (*command_func)(char * argument);
	void (*command_help)(void);
	char * (*command_name)();
};

#endif /* CLI_INTERFACE_H_ */
