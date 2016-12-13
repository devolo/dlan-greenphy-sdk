/*
===============================================================================
 Name        : ClickboardIO.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include <cr_section_macros.h>

// TODO: insert other include files here

// TODO: insert other definitions and declarations here

/*******************************************************************************
* Function Get_Temperature()
* ------------------------------------------------------------------------------
* Overview: Function gets temperature data from TMP102 sensor
* Input: Nothing
* Output: Temperature data
*******************************************************************************/
float Get_Temperature() {
  I2C_XFER_T xfer;

  xfer.slaveAddr = 0x48;                    // TMP102 I2C address (ADD0 pin is connected to ground)
  //xfer.slaveAddr = 0x49;                    // TMP102 I2C address (ADD0 pin is connected to ground)
  #define TEMP_OFFSET  -3.5

  int TemperatureSum;
  float Temperature;
  int i2cDev = I2C1;
  uint8_t tmp_data[2];

  tmp_data[0] = 0;
  xfer.txBuff = tmp_data;
  xfer.rxBuff = tmp_data;
  xfer.txSz = 1;
  xfer.rxSz = 2;

  Chip_I2C_MasterSend(i2cDev, xfer.slaveAddr, xfer.txBuff, xfer.txSz);
  Chip_I2C_MasterRead(i2cDev, xfer.slaveAddr, xfer.rxBuff, xfer.rxSz);

  TemperatureSum = ((tmp_data[0] << 8) | tmp_data[1]) >> 4;  // Justify temperature values

  if(TemperatureSum & (1 << 11))                             // Test negative bit
    TemperatureSum |= 0xF800;                                // Set bits 11 to 15 to logic 1 to get this reading into real two complement

  Temperature = (float)TemperatureSum * 0.0625;              // Multiply temperature value with 0.0625 (value per bit)

  return (Temperature + TEMP_OFFSET);                                        // Return temperature data
}

int main(void) {
	float Temperature_read;

#if defined (__USE_LPCOPEN)
    // Read clock settings and update SystemCoreClock variable
    SystemCoreClockUpdate();
#if !defined(NO_BOARD_LIB)
    // Set up and initialize all required blocks and
    // functions related to the board hardware
    Board_Init();
    // Set the LED to the state of "On"
    Board_LED_Set(1, true);
#endif
#endif

    // TODO: insert code here

    // Force the counter to be placed into memory
    volatile static int i = 0 ;
    // Enter an infinite loop, just incrementing a counter

    //Click Init



	printInit(LPC_USART0);
	debug_init(LPC_USART0);

	printToUart("\r\nClickBoard Test\r\n");
	Board_I2C_Init(I2C1);  // <--- configures the I2C pins
	Chip_I2C_Init(I2C1);
	Chip_I2C_SetClockRate(I2C1, 100000/*SPEED_400KHZ*/);
	Chip_I2C_SetMasterEventHandler(I2C1, Chip_I2C_EventHandlerPolling);

    while(1) {

        i++ ;
        if (i>500000)
		{
        	Temperature_read = Get_Temperature();                    // Store temperature value in Temperature_read
        	Board_LED_Toggle(1);
        	printToUart("Temp: %d", (int)(Temperature_read));
        	printToUart(".%d deg \r\n", ((int)(Temperature_read*10)%10));
        	i=0;
		}
    }
    return 0 ;
}
