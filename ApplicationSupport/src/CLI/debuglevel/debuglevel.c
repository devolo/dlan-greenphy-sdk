#include "FreeRTOS.h"
#include "string.h"
#include "debug.h"
#include "CLI_interface.h"
#include "debuglevel.h"
#include "libCLI.h"

#ifdef DEBUG
// extern uint32_t debug_level;

struct debuglevel {
  int level;
  char * name;
};

const struct debuglevel debuglevels[] = {
		{ NOTHING, "NOTHING" },
		{ GREEN_PHY_RX, "GREEN_PHY_RX" },
		{ GREEN_PHY_TX, "GREEN_PHY_TX" },
		{ ETHERNET_RX_BINARY, "ETHERNET_RX_BINARY" },
		{ ETHERNET_TX_BINARY, "ETHERNET_TX_BINARY" },
		{ ETHERNET_RX, "ETHERNET_RX" },
		{ ETHERNET_TX, "ETHERNET_TX" },
		{ GREEN_PHY_RX_BINARY, "GREEN_PHY_RX_BINARY" },
		{ GREEN_PHY_TX_BINARY, "GREEN_PHY_TX_BINARY" },
		{ GREEN_PHY_INTERUPT, "GREEN_PHY_INTERUPT" },
		{ ETHERNET_INTERUPT, "ETHERNET_INTERUPT" },
		{ DEBUG_PANIC, "PANIC" },
		{ DEBUG_EMERG, "EMERG" },
		{ DEBUG_ALERT, "ALERT" },
		{ DEBUG_CRIT, "CRIT" },
		{ DEBUG_ERROR_SEARCH, "ERROR_SEARCH" },
		{ DEBUG_ERR, "ERR" },
		{ DEBUG_WARNING, "WARNING" },
		{ DEBUG_NOTICE, "NOTICE" },
		{ DEBUG_INFO, "INFO" },
		{ DEBUG_TASK, "TASK" },
		{ DMA_INTERUPT, "DMA_INTERUPT" },
		{ CLI, "CLI" },
		{ GPIO_INTERUPT, "GPIO_INTERUPT" },
		{ ETHERNET_FLOW_CONTROL, "ETHERNET_FLOW_CONTROL" },
		{ GREEN_PHY_SYNC_STATUS, "GREEN_PHY_SYNC_STATUS" },
		{ DEBUG_ALL, "ALL" },
		{ DEBUG_INTERRUPTS, "INTERRUPTS" },
		{ DEBUG_BINARY, "BINARY" },
		{ GREEN_PHY_FRAME_SYNC_STATUS, "GREEN_PHY_FRAME_SYNC" },
		{ DEBUG_GREEN_PHY, "GREEN_PHY" },
		{ DEBUG_DATA_FLOW, "DATA_FLOW" },
		{ DEBUG_BUFFER, "BUFFER" },
		{ GREEN_PHY_FW_FEATURES, "GREEN_PHY_FW_FEATURES"}
};
//---------------------------------------------------------------------------------------------------

static char name[] = "debuglevel";

char * debuglevel_commandName(void)
{
	return name;
}

int setdebuglevel(char * arg)
{
	int rv = cli_ok;
	uint32_t debug_level = DEBUG_LEVEL_GET();

	arg = removeTrailingCharacter(' ',arg);
	char * useNewLevel = arg;
	arg = removeTrailingCharacter('|',arg);

	if(useNewLevel == arg)
	{
		debug_level = 0;
	}

	while(arg)
	{
		arg = removeTrailingCharacter(' ',arg);
		arg = removeTrailingCharacter('|',arg);
		char * noNegation = arg;
		arg = removeTrailingCharacter('!',arg);
		int i;
		int found = 0;
		if(strlen(arg))
		{
			DEBUG_PRINT(CLI, "examining: '%s'\r\n",arg);
			for(i=0;i<(sizeof(debuglevels)/sizeof(struct debuglevel));i+=1)
			{
				char * name = debuglevels[i].name;
				size_t length = strlen(name);

				if(!strncmp(arg,name,length))
				{
					DEBUG_PRINT(CLI, "  is    : '%s'",debuglevels[i].name);
					if(noNegation == arg)
					{
						DEBUG_PRINT(CLI, "  -> |= 0x%x\r\n",debuglevels[i].level);
						debug_level |= debuglevels[i].level;
					}
					else
					{
						DEBUG_PRINT(CLI, "  -> &=~ 0x%x\r\n",debuglevels[i].level);
						debug_level &= ~debuglevels[i].level;
					}
					found = 1;
					arg += length;
					break;
				}
				else
				{
					DEBUG_PRINT(CLI, "  is not: '%s'\r\n",debuglevels[i].name);
				}
			}
			if(!found)
			{
				rv = cli_arg_unknown;
				arg = NULL;
			}
		}
		else
		{
			arg = NULL;
		}
	}

	if(rv == cli_ok)
	{
		DEBUG_LEVEL_SET(debug_level);
	}

	return rv;
}

void printDebugLevel(int level)
{
	int printSeperator=0;
	int i;

	for(i=1;i<(sizeof(debuglevels)/sizeof(struct debuglevel));i+=1)
	{
		if((level & debuglevels[i].level) == debuglevels[i].level)
		{
			if(printSeperator)
			{
				printToUart("|");
			}
			printToUart("%s",debuglevels[i].name);
			printSeperator = 1;
		}
	}
}

int getdebuglevel(char * arg)
{
	int rv = cli_ok;

	uint32_t debug_level = DEBUG_LEVEL_GET();

	if(debug_level == NOTHING)
	{
		printDebugLevel(NOTHING);
	}
	else if(debug_level == DEBUG_ALL)
	{
		printDebugLevel(DEBUG_ALL);
	}
	else
	{
		printDebugLevel(debug_level);
	}
	printToUart("\r\n");

	return rv;
}

void debuglevel_help(void)
{
	int i;
	printToUart("Use '%s set LEVEL' to set the debug level, using as LEVEL:\r\n",debuglevel_commandName());
	for(i=0;i<(sizeof(debuglevels)/sizeof(struct debuglevel));i+=1)
	{
		printToUart("  %s\r\n",debuglevels[i].name);
	}
	printToUart("Use '%s get' to print the current debug level.\r\n",debuglevel_commandName());
}

char * getName()
{
	return "get";
}

char * setName()
{
	return "set";
}

static struct command localCommands[] = {
		{setdebuglevel,debuglevel_help,setName},
		{getdebuglevel,debuglevel_help,getName},
		{NULL,NULL,NULL}
};

int debuglevel_command(char * arg)
{
	int rv = cli_arg_required;

	if(strlen(arg))
	{
	  rv = checkCommand(localCommands,arg);
	}
	else
	{
		debuglevel_help();
	}

	return rv;
}

//---------------------------------------------------------------------------------------------------

static struct command debuglevel = {debuglevel_command,debuglevel_help, debuglevel_commandName};

struct command * debuglevelInit(void)
{
	return &debuglevel;
}
#endif
