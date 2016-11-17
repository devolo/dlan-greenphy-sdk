/*
 * libCLI.h
 *
 *  Created on: 16.08.2012
 */

#ifndef LIBCLI_H_
#define LIBCLI_H_

#include "CLI_interface.h"

typedef enum {
	cli_ok = 0,
	cli_command_too_long,
	cli_command_unknown,
	cli_arg_unknown,
	cli_arg_required
} cliResults_t;

char * removeTrailingCharacter(char toRemove, char *input);

int checkCommand(struct command* liste,char *input);

#endif /* LIBCLI_H_ */
