#include "FreeRTOS.h"
#include "string.h"
#include "debug.h"
#include "CLI_interface.h"
#include "netdevice_CLI.h"
#include "libCLI.h"

//---------------------------------------------------------------------------------------------------


struct netdevice_table {
	struct netDeviceInterface * netdevice;
	char * name;
};

#define MAX_NUMBER_OF_NETDEVICES 4

struct netdevice_table netdevices[MAX_NUMBER_OF_NETDEVICES] = {{0}};

void netdeviceAdd(struct netDeviceInterface * device, char * name)
{
	int i;
	for(i=0; i<MAX_NUMBER_OF_NETDEVICES; i+=1)
	{
		if(netdevices[i].netdevice == NULL)
		{
			netdevices[i].netdevice = device;
			netdevices[i].name = name;
			break;
		}
	}
}

static char name[] = "netdevice";

char * netdevice_CLI_commandName(void)
{
	return name;
}

int resetnetdevice_CLI(char * arg)
{
	int rv = cli_arg_unknown;

	arg = removeTrailingCharacter(' ',arg);

	int i;

	for(i=0;i<(sizeof(netdevices)/sizeof(struct netdevice_table));i+=1)
	{
		char * name = netdevices[i].name;
		size_t length = strlen(name);

		if(length)
		{
			if(!strncmp(arg,name,length))
			{
				DEBUG_PRINT(CLI, "resetting '%s' ...\r\n",name);
				netdevices[i].netdevice->reset(netdevices[i].netdevice);
				DEBUG_PRINT(CLI, "reset '%s' done!\r\n",name);
				rv = cli_ok;
				break;
			}
		}
	}

	return rv;
}

int printnetdevice_CLI(char * arg)
{
	int rv = cli_ok;

	int i;

	printToUart("Available devices:\r\n");
	for(i=0;i<(sizeof(netdevices)/sizeof(struct netdevice_table));i+=1)
	{
		if(netdevices[i].name == NULL)
			break;
		printToUart(" %s\r\n",netdevices[i].name);
	}
	printToUart("\r\n");

	return rv;
}

void netdevice_CLI_help(void)
{
	printToUart("Use '%s reset <name>' to reset a net device.\r\n",netdevice_CLI_commandName());
	printToUart("Use '%s print' to print the available devices.\r\n",netdevice_CLI_commandName());
	printnetdevice_CLI(netdevice_CLI_commandName());
}

char * resetName()
{
	return "reset";
}

char * printName()
{
	return "print";
}

static struct command localCommands[] = {
		{resetnetdevice_CLI,netdevice_CLI_help,resetName},
		{printnetdevice_CLI,netdevice_CLI_help,printName},
		{NULL,NULL,NULL}
};

int netdevice_CLI_command(char * arg)
{
	int rv = cli_arg_required;

	if(strlen(arg))
	{
	  rv = checkCommand(localCommands,arg);
	}
	else
	{
		netdevice_CLI_help();
	}

	return rv;
}

//---------------------------------------------------------------------------------------------------

static struct command netdevice_CLI = {netdevice_CLI_command,netdevice_CLI_help, netdevice_CLI_commandName};

struct command * netdeviceInit_CLI(void)
{
	return &netdevice_CLI;
}
