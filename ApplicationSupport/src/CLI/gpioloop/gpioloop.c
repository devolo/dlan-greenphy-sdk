#include "FreeRTOS.h"
#include "string.h"
#include "debug.h"
#include "lpc17xx_gpio.h"
#include "gpio.h"
#include "CLI_interface.h"
#include "gpioloop.h"
#include "libCLI.h"

static struct GPIO gatePort = {0,11};

//---------------------------------------------------------------------------------------------------

int gpioloop_command(char * arg)
{

	int rv = cli_arg_unknown;

	arg = removeTrailingCharacter(' ',arg);

	if(strlen(arg))
	{

		char * command = "HIGH";
		size_t length = strlen(command);

		if(!strncmp(arg,command,length))
		{
			GPIO_SetDir((gatePort.port),(GPIO_MAP_PIN(gatePort.pin)), GPIO_OUT);
			GPIO_SetValue((gatePort.port), (GPIO_MAP_PIN(gatePort.pin)));
			rv = cli_ok;
		}

		command = "LOW";
		length = strlen(command);

		if(!strncmp(arg,command,length))
		{
			GPIO_SetDir((gatePort.port),(GPIO_MAP_PIN(gatePort.pin)), GPIO_OUT);
			GPIO_ClearValue((gatePort.port), (GPIO_MAP_PIN(gatePort.pin)));
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


static char name[] = "gpioloop";

char * gpioloop_commandName(void)
{
	return name;
}

void gpioloop_help(void)
{
	 printToUart("To test the QCA7000 GPIOs, you have to utilise a magic gate at port ");
	 printGpio(&gatePort);
	 printToUart("\r\nfor the boot strap options to vanish ...\r\n");
	 printToUart("Use '%s HIGH' to activate the port.\r\n",gpioloop_commandName());
	 printToUart("Use '%s LOW' to deactivate the port.\r\n",gpioloop_commandName());
	 printToUart("Beware, the port is not touched until you use '%s'!\r\n",gpioloop_commandName());
}

static struct command gpioloop = {gpioloop_command,gpioloop_help, gpioloop_commandName};

struct command * gpioloopInit(void)
{
	return &gpioloop;
}
