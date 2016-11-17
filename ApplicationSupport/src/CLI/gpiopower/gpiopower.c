#include "FreeRTOS.h"
#include "string.h"
#include "debug.h"
#include "CLI_interface.h"
#include "libCLI.h"
#include "gpio.h"
#include "busyWait.h"
#include "greenPhyModuleConfig.h"
#include <relay.h>

//---------------------------------------------------------------------------------------------------

int gpiopower_command(char * arg)
{

	int rv = cli_arg_unknown;

	arg = removeTrailingCharacter(' ',arg);

	if(strlen(arg))
	{
		char * command = "OFF";
		size_t length = strlen(command);

		if(!strncmp(arg,command,length))
		{
			setRelayOff();
			rv = cli_ok;
		}

		command = "ON";
		length = strlen(command);

		if(!strncmp(arg,command,length))
		{
			setRelayOn();
			rv = cli_ok;
		}
	}
	else
	{
		rv = cli_arg_required;
	}

	return rv;
}

//---------------------------------------------------------------------------------------------------


static char name[] = "power";

char * gpiopower_commandName(void)
{
	return name;
}

void gpiopower_help(void)
{
	 printToUart("To test the plug on the metering board, you have to utilise the relay at port ");
	 printGpio(&gpioRelayOn);
	 printToUart(" and port ");
	 printGpio(&gpioRelayOff);
	 printToUart(".\r\n");
	 printToUart("Use '%s ON' to power on the plug.\r\n",gpiopower_commandName());
	 printToUart("Use '%s OFF' to power off the plug.\r\n",gpiopower_commandName());
}

static struct command gpiopower = {gpiopower_command,gpiopower_help, gpiopower_commandName};

struct command * gpiopowerInit(void)
{
	return &gpiopower;
}
