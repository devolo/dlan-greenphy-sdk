#include "CLI.h"
#include "stdint.h"
#include "string.h"
#include "uart.h"
#include "debug.h"
#include "FreeRTOS.h"
#include "portmacro.h"
#include "task.h"
#include "libCLI.h"
#include "CLI_interface.h"
#include "gpiotest.h"
#include "restart.h"
#include "gpioloop.h"
#include "gpiopower.h"
#include "debuglevel.h"
#include "netdevice_CLI.h"

#define mainCLI_TASK_PRIORITY			( tskIDLE_PRIORITY + 1 )

struct command befehlsliste[10];
static char name[] = "help";

//---------------------------------------------------------------------------------------------------

void printResult(int result)
{
	switch(result)
	{
	case cli_ok:
		 printToUart("OK\r\n");
		break;
	case cli_command_too_long:
		printToUart("STRING TOO LONG\r\n");
		break;
	case cli_command_unknown:
		 printToUart("Use 'help' to get a list of commands.\r\n");
		 printToUart("UNKNOWN COMMAND\r\n");
		break;
	case cli_arg_unknown:
		 printToUart("UNKNOWN ARGUMENT\r\n");
		break;
	case cli_arg_required:
		 printToUart("ARGUMENT REQUIRED\r\n");
		break;
	}

	if(result != cli_ok)
	{
		 printToUart("ERROR\r\n");
	}
}

//---------------------------------------------------------------------------------------------------
void removeData(uint8_t * input, uint16_t size, uint16_t pos, uint16_t toSkip)
{
	uint16_t d,s;

	if(pos)
	{
		d=pos-1;
	}
	else
	{
		d=pos;
	}

	s=pos+toSkip;

	DEBUG_PRINT(CLI, "%s source %d destination %d\r\n",__func__,s,d);
	DEBUG_DUMP(CLI, input, size, "before");

	for(;s<size;d+=1,s+=1)
	{
		input[d] = input[s];
	}
	for(;d<size;d+=1)
	{
		input[d] = 0x0;
	}

	DEBUG_DUMP(CLI, input, size, "after");
}

void removeControlCodes(uint8_t * input, uint16_t size)
{
	int16_t i,cursor;

	DEBUG_PRINT(CLI, "%s\r\n",__func__);

	for(i=0,cursor=0;i<size-1;i+=1,cursor+=1)
	{

		DEBUG_PRINT(CLI, "pos 0x%x char 0x%x '%c' cursor 0x%x size 0x%x ",i,input[i],input[i],cursor,size);

		if(input[i] && (input[i] < 0x20))
		{
			switch(input[i])
			{
			case 0x1b: // escape
			{
				DEBUG_PRINT(CLI, "ESC ");
				if(input[i+2] == 'C') // ->
				{
					if(cursor==0)
					{
						cursor = -1;
					}
				}
				else if(input[i+2] == 'D') // <-
				{
					if(cursor>1)
					{
						cursor -= 2;
					}
					else
					{
						cursor = -1;
					}
				}
				removeData(input, size, i+1, 2);
				i-=1;
				size-=3;
			}
			break;
			case 0x08: // backspace
			{
				DEBUG_PRINT(CLI, "BS ");
				removeData(input, size, i, 1);
				i-=2;
				size-=2;
				cursor -= 2;
			}
			break;
			default:  // just remove the control code
			{
				DEBUG_PRINT(CLI, "CTRL ");
				removeData(input, size, i, 0);
				size-=1;
			}
			break;
			}

			if(i<-1)
			{
				i = -1;
			}
		}
		else
		{
			switch(input[i])
			{
				case 0x7f: // delete
				{
					DEBUG_PRINT(CLI, "DEL ");
					removeData(input, size, i, 1);
					i-=2;
					size-=2;
					cursor -= 2;
				}
				break;
			}
			if(cursor!=i)
			{
				DEBUG_PRINT(CLI, "X ");
				input[cursor]=input[i];
				removeData(input, size, i+1, 0);
				i-=1;
				size-=1;
			}
		}
		DEBUG_PRINT(CLI, "\r\n");
	}
}

char * removeTrailingCharacter(char toRemove, char *input)
{
	char * rv = input;

	while(*rv == toRemove) {
		rv+=1;
	}

	return rv;
}

int getNumberOfCommands(struct command* liste)
{
	int rv = 0;
	while (liste->command_func && liste->command_help && liste->command_name ) {
		rv+=1; liste+=1;
	};

	return rv;
}

struct command * findCommandFromString(struct command* liste, char *input)
{
	struct command * rv = NULL;
	unsigned int i, numberOfCommands;

	numberOfCommands = getNumberOfCommands(liste);

	for (i =0; i<numberOfCommands; i++,liste++)
	{
		char * command = "";
		command = liste->command_name();
		size_t length = strlen(command);

		if(!strncmp(input,command,length))
		{
			rv = liste;
			break;
		}
	}

	return rv;
}

//---------------------------------------------------------------------------------------------------

int help(char * args)	// Zeigt an was alles geht an Befehlen - mit kurzer Erläuterung
{
	int rv = cli_ok;
	unsigned int i = 0;
	unsigned int numberOfCommands = getNumberOfCommands(befehlsliste);

	args = removeTrailingCharacter(' ',args);

	if(!strlen(args))
	{
		printToUart("Commands:\r\n");
		for (i =0; i<numberOfCommands; i++)
		{
			printToUart("  %s\r\n", befehlsliste[i].command_name());
		}
		printToUart("To get more help for a COMMAND, type 'help COMMAND'\r\n");
	}
	else
	{
		struct command * foundCommand = findCommandFromString(befehlsliste, args);

		if(foundCommand)
		{
			foundCommand->command_help();
		}
		else
		{
			rv = cli_command_unknown;
		}
	}

	return rv;
}

//---------------------------------------------------------------------------------------------------

void help_help(void)
{
	 printToUart("Help explains more in detail what the given command does.\r\n");
}

char * help_commandName(void)
{
	return name;
}

static struct command help_command = {help, help_help, help_commandName};

struct command * helpInit(void)
{
	return &help_command;
}

//---------------------------------------------------------------------------------------------------

int checkCommand(struct command* liste,char *input)
{
	int rv = 0;

	input = removeTrailingCharacter(' ',input);

	struct command * foundCommand = findCommandFromString(liste, input);

	if(foundCommand)
	{
		 rv = foundCommand->command_func(input+strlen(foundCommand->command_name()));
	}
	else
	{
		 rv = cli_command_unknown;
	}

	return rv;
}

//---------------------------------------------------------------------------------------------------

void mainCLI ( void * parameters)
{
	( void ) parameters;

#define COMMAND_BUFFER_SIZE 255
	static uint8_t commandBuffer[COMMAND_BUFFER_SIZE+1];

	// say hello to receive commands
	printToUart("\r\nCLI\r\n");
	printResult(cli_ok);

	uint8_t * readPosition = commandBuffer;
	uint16_t size = COMMAND_BUFFER_SIZE;

	for (;;)
	{
		int numberOfReadBytes = UARTReceive(0, readPosition, size, portMAX_DELAY);

		if(numberOfReadBytes)
		{
			// check for errors and report
			if(numberOfReadBytes<0)
			{
				numberOfReadBytes = -numberOfReadBytes;
				printToUart(":OVERFLOW:");
			}

			// echo the read characters
	        UARTSend(0, readPosition , numberOfReadBytes );

	        if(readPosition[numberOfReadBytes-1] == '\r')
	        {
		        // echo with crlf if cr is received
				printToUart("\r\n");

	        	readPosition[numberOfReadBytes-1]=0x0;

				removeControlCodes(commandBuffer,(COMMAND_BUFFER_SIZE-size+numberOfReadBytes));

	        	// execute the checker
	        	printResult(checkCommand(befehlsliste, (char*)commandBuffer));
				readPosition = commandBuffer;
				size = COMMAND_BUFFER_SIZE;
				memset(commandBuffer, 0, sizeof(COMMAND_BUFFER_SIZE));

	        }
	        else
	        {
	        	size -= numberOfReadBytes;
	        	readPosition += numberOfReadBytes;

	        	if(size <= 1)
	        	{
					printToUart("\r\n");
	        		printResult(cli_command_too_long);
	        		readPosition = commandBuffer;
	        		size = COMMAND_BUFFER_SIZE;
	        	}
	        }
		}
	}
}

//---------------------------------------------------------------------------------------------------

void cliInit()
{
	printInit(UART0);
	memset(befehlsliste, 0, sizeof(befehlsliste));
	befehlsliste[0]=*helpInit();
	befehlsliste[1]=*gpiotestInit();
	befehlsliste[2]=*restartInit();
	befehlsliste[3]=*gpioloopInit();
	befehlsliste[4]=*gpiopowerInit();
	befehlsliste[5]=*netdeviceInit_CLI();
#ifdef DEBUG
	// must be the last to avoid gaps ...
	befehlsliste[6]=*debuglevelInit();
#endif
	static const signed char * const CliLoop = (const signed char * const) "mainCLI";
	xTaskCreate( mainCLI, CliLoop, 240, NULL, mainCLI_TASK_PRIORITY, NULL );

}

//---------------------------------------------------------------------------------------------------
