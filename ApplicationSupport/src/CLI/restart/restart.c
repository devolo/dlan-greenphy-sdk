#include "restart.h"
#include "FreeRTOS.h"
#include "string.h"
#include "timers.h"
#include "CLI_interface.h"
#include "libCLI.h"
#include "debug.h"

#include "portmacro.h"

signed char buffer[1024];

void  restart_callback( xTimerHandle xTimer )
{

	char * arg = (char *) pvTimerGetTimerID(xTimer);
	xTimerDelete(xTimer,0);

	arg = removeTrailingCharacter(' ',arg);

	NVIC_SystemReset();
}

//---------------------------------------------------------------------------------------------------

int restart_command(char * arg)
{
	xTimerHandle restart_timer;
	restart_timer = xTimerCreate((const signed char*)"Restart_timer",(unsigned long)10, pdTRUE,(void*)arg, restart_callback);
	xTimerStart(restart_timer,0);

	return 0;
}

//---------------------------------------------------------------------------------------------------


void restart_help(void)
{
	 printToUart("This will restart the microprocessor.\r\n");
}

static char name[] = "restart";

char * restart_commandName(void)
{
	return name;
}

static struct command restart = {restart_command,restart_help, restart_commandName};

struct command * restartInit(void)
{
	return &restart;
}
