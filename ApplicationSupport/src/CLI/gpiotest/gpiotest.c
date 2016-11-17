#include "FreeRTOS.h"
#include "gpiotest.h"
#include "debug.h"
#include "lpc17xx_gpio.h"
#include "gpio.h"
#include "CLI_interface.h"

struct GPIO_test_state{
	struct GPIO line;
	uint8_t default_state;
	uint8_t value_at_zero;
	uint8_t value_at_one;
};

struct GPIO_test{
	struct GPIO_test_state first;
	struct GPIO_test_state second;
	struct GPIO_test_state third;
};

struct GPIO_test test_tuple[]={
		{{{1,30},0,0,0},{{0,9},0,0,0},{{255,255},0,0,0}},
		{{{1,19},0,0,0},{{2,2},0,0,0},{{255,255},0,0,0}},
		{{{2,7},0,0,0},{{0,30},0,0,0},{{255,255},0,0,0}},
		{{{1,18},0,0,0},{{2,4},0,0,0},{{255,255},0,0,0}},
		{{{2,5},0,0,0},{{0,29},0,0,0},{{255,255},0,0,0}},
		{{{1,25},0,0,0},{{2,3},0,0,0},{{255,255},0,0,0}},
		{{{0,26},0,0,0},{{2,6},0,0,0},{{255,255},0,0,0}},
		{{{1,31},0,0,0},{{2,1},0,0,0},{{255,255},0,0,0}},
		{{{0,25},0,0,0},{{2,0},0,0,0},{{255,255},0,0,0}},
		{{{0,8},0,0,0},{{0,10},0,0,0},{{255,255},0,0,0}},
		{{{1,26},0,0,0},{{2,9},0,0,0},{{255,255},0,0,0}},
		{{{0,6},0,0,0},{{1,28},0,0,0},{{255,255},0,0,0}},
		{{{0,0},0,0,0},{{0,7},0,0,0},{{255,255},0,0,0}},
		{{{0,1},0,0,0},{{1,22},0,0,0},{{255,255},0,0,0}}
};

//---------------------------------------------------------------------------------------------------------------

int isValidGpio(struct GPIO * gpio)
{
  int rv = 0;

  if((gpio->port != 255)&&(gpio->pin!=255))
  {
	  rv = 1;
  }

  return rv;
}
//---------------------------------------------------------------------------------------------------------------

int print_cr_lf(void)
{
	printToUart("\r\n");
	return 0;
}

//---------------------------------------------------------------------------------------------------------------

typedef int (*callbackForAllGPIOs) (struct GPIO_test_state *);
typedef int (*callbackForAllTests) (void);

int doForAllGpios(callbackForAllGPIOs funcGPIO,callbackForAllTests funcTest)
{
	unsigned int i =0;
	int rv = 0;
	for(i =0; i<(int)(sizeof(test_tuple)/sizeof(struct GPIO_test)); i++)
	{
		struct GPIO_test_state * state = &(test_tuple[i].first);

		int local_rv = 0;

		unsigned int j;
		for(j=0;j<(sizeof(struct GPIO_test)/sizeof (struct GPIO_test_state));j++)
		{
			if(isValidGpio(&state->line))
			{
				if(funcGPIO){
					local_rv = funcGPIO(state);
					if(!rv)
					{
						rv = local_rv;
					}
				}
			}
			else
				break;
			state += 1;
		}
		if(funcTest)
		{
			local_rv = funcTest();
			if(!rv)
			{
				rv = local_rv;
			}
		}
	}
	return rv;
}

int readInValueAtZero(struct GPIO_test_state * state)
{
	uint32_t testValue;
	testValue = GPIO_ReadValue(state->line.port)&(GPIO_MAP_PIN(state->line.pin));
	if(testValue)
	{
		state->value_at_zero = 1;
	}
	else
	{
		state->value_at_zero = 0;
	}

	return 0;
}

int readInValueAtOne(struct GPIO_test_state * state)
{
	uint32_t testValue;
	testValue = GPIO_ReadValue(state->line.port)&(GPIO_MAP_PIN(state->line.pin));
	if(testValue)
	{
		state->value_at_one = 1;
	}
	else
	{
		state->value_at_one = 0;
	}

	return 0;
}

int setToInputAndClearState(struct GPIO_test_state * state)
{
	GPIO_SetDir((state->line.port),(GPIO_MAP_PIN(state->line.pin)), GPIO_IN);
	uint32_t testValue;
	testValue = GPIO_ReadValue(state->line.port)&(GPIO_MAP_PIN(state->line.pin));
	if(testValue)
	{
		state->default_state = 1;
	}
	else
	{
		state->default_state = 0;
	}
	state->value_at_one=0;
	state->value_at_zero=0;

	return 0;
}

void gpio_set_all_input()
{
	doForAllGpios(setToInputAndClearState,NULL);
}

//---------------------------------------------------------------------------------------------------------------

int isStateInTuple(struct GPIO_test_state * tested, struct GPIO_test * tuple)
{
	int rv = 0;

	unsigned int j;
	struct GPIO_test_state * current = &(tuple->first);
	for(j=0;j<(sizeof(struct GPIO_test)/sizeof (struct GPIO_test_state));j++)
	{
		if(isValidGpio(&current->line))
		{
			if(tested == current)
			{
				rv = 1;
			}
		}
		else
			break;
		current += 1;
	}

	return rv;
}

int checkTestInTuple(struct GPIO_test_state * tested, struct GPIO_test * tuple)
{
	int rv = 0;

	printGpio(&tested->line);

	unsigned int j;
	struct GPIO_test_state * current = &(tuple->first);
	for(j=0;j<(sizeof(struct GPIO_test)/sizeof (struct GPIO_test_state));j++)
	{
		if(isValidGpio(&current->line))
		{
			if(current != tested)
			{
				if((current->value_at_one == 1) && (current->value_at_zero == 0))
				{
					printToUart("=");
				}
				else
				{
					printToUart("!");
					rv = 1;
				}
				printGpio(&current->line);
			}
		}
		else
			break;
		current += 1;
	}

	return rv;
}

int checkTestNotInTuple(struct GPIO_test_state * tested, struct GPIO_test * tuple)
{
	int rv = 0;

	unsigned int j;
	struct GPIO_test_state * current = &(tuple->first);
	for(j=0;j<(sizeof(struct GPIO_test)/sizeof (struct GPIO_test_state));j++)
	{
		if(isValidGpio(&current->line))
		{
			if((current->default_state!=current->value_at_one)||(current->default_state!=current->value_at_zero))
			{
				printToUart(";");
				printGpio(&tested->line);
				printToUart("#");
				printGpio(&current->line);
				rv = 1;
			}
		}
		else
			break;
		current += 1;
	}

	return rv;
}

int checkTestResult(struct GPIO_test_state * tested)
{
	int rv = 0;

	unsigned int i;
	unsigned int location = 0;
	for(i=0; i<(int)(sizeof(test_tuple)/sizeof(struct GPIO_test)); i++)
	{
		int found = isStateInTuple(tested,&test_tuple[i]);

		if(found)
		{
			location = i;
			rv = checkTestInTuple(tested,&test_tuple[i]);
			break;
		}
	}

	for(i=0; i<(int)(sizeof(test_tuple)/sizeof(struct GPIO_test)); i++)
	{
		if(location != i)
		{
			int local_rv ;
			local_rv = checkTestNotInTuple(tested,&test_tuple[i]);
			if(!rv)
			{
				rv = local_rv;
			}
		}
	}

	print_cr_lf();

	return rv;
}

//---------------------------------------------------------------------------------------------------------------

int testLine(struct GPIO_test_state * state)
{
	int rv;

	if(((state->line.port)==0)&&((state->line.pin))==30)
	{
		// see comment [2] of Table 88. Pin Mode select register 1 of the programmers manual
		rv = 0;
	}
	else if(((state->line.port)==0)&&((state->line.pin))==29)
	{
		// see comment [2] of Table 88. Pin Mode select register 1 of the programmers manual
		rv = 0;
	}
	else
	{
		GPIO_SetDir((state->line.port), (GPIO_MAP_PIN(state->line.pin)), GPIO_OUT);

		GPIO_ClearValue((state->line.port), (GPIO_MAP_PIN(state->line.pin)));
		doForAllGpios(readInValueAtZero,NULL);

		GPIO_SetValue((state->line.port), (GPIO_MAP_PIN(state->line.pin)));
		doForAllGpios(readInValueAtOne,NULL);

		rv = checkTestResult(state);

		// reset all states
		doForAllGpios(setToInputAndClearState,NULL);
	}

	return rv;
}

//---------------------------------------------------------------------------------------------------------------
int gpio_command_test(char * unused)
{

	(void) unused;

	// Alle GPIO's auf Eingang setzen
	gpio_set_all_input();

	int rv;

	rv = doForAllGpios(testLine,NULL);

	if(rv)
	{
		printToUart("FAIL\r\n" );
		// don't return an error
		rv = 0;
	}
	else
	{
		printToUart("PASS\r\n" );
	}

	return rv;
}

//---------------------------------------------------------------------------------------------------

int printGpioState(struct GPIO_test_state * state)
{
	printToUart(" ");
	return printGpio(&state->line);
}

//---------------------------------------------------------------------------------------------------

void print_used_gpio_tuple()
{
	doForAllGpios(printGpioState,print_cr_lf);
}

//---------------------------------------------------------------------------------------------------

static char name[] = "gpiotest";

char * gpiotest_commandName(void)
{
	return name;
}

void gpiotest_help(void)
{
	 printToUart("'%s' will test the functionality of the GPIOs."
			 	 	 	 	"\r\nThe test will take tuples of GPIOs."
			 	 	 	 	"\r\nOne of the them will be set as output, all others as input."
			 	 	 	 	"\r\nIt is tested if the input receives a signal when something is send out."
			 	 	 	 	"\r\nAlso all other GPIOs mentioned in the list are tested to prove shorts."
			 	 	 	 	"\r\nAfter that the test is accomplished vice versa, but not for P0[30] or"
			 	 	 	 	"\r\nP0[29], because these pins must have the same direction since they"
	 	 	 				"\r\noperate as a unit for the USB function."
			 	 	 	 	"\r\nThe tupels of GPIOs are shown in the list below.\r\n\r\n",gpiotest_commandName());

	 print_used_gpio_tuple();

	 printToUart("\r\nOccurring results are displayed in the following form:"
 	 	 	 	 	 	 "\r\n  Px1[x2]=Py1[y2] : Port x1 Pin x2 is correctly connected to Port y1 Pin y2"
			 	 	 	 "\r\n  Px1[x2]!Py1[y2] : Port x1 Pin x2 is not connected to Port y1 Pin y2"
			 	 	 	 "\r\n  Px1[x2]#Py1[y2] : Port #1 Pin #2 and Port #3 Pin #4 are hot-wired\r\n");
}

static struct command gpiotest = {gpio_command_test,gpiotest_help, gpiotest_commandName};

struct command * gpiotestInit(void)
{
	return &gpiotest;
}

