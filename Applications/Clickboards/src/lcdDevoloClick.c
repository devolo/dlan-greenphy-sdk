/*
 * Copyright (c) 2023, devolo GmbH, Aachen, Germany.
 * All rights reserved.
 *
 * This Software is part of the devolo GreenPHY-SDK.
 *
 * Usage in source form and redistribution in binary form, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Usage in source form is subject to a current end user license agreement
 *    with the devolo GmbH.
 * 2. Neither the name of the devolo GmbH nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 3. Redistribution in binary form is limited to the usage on the GreenPHY
 *    module of the devolo GmbH.
 * 4. Redistribution in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <string.h>
#include <stdlib.h>
#include "board.h"
#include "chip.h"
#include "clickboardIO.h"



#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "debug.h"
#include "lcdDevoloClick.h"


int iChannel = 0;


/* Function send the a nibble on the Data bus (LCD_D4 to LCD_D7) */
void sendNibble(char nibble)
{
	if (iChannel == SLOT1)
{
		Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD1_TX_GPIO_PORT_NUM,  CLICKBOARD1_TX_GPIO_BIT_NUM,  ((nibble >>0x00) & 0x01) );
		Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD1_RX_GPIO_PORT_NUM,  CLICKBOARD1_RX_GPIO_BIT_NUM,  ((nibble >>0x01) & 0x01) );
		Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD1_CS_GPIO_PORT_NUM,  CLICKBOARD1_CS_GPIO_BIT_NUM,  ((nibble >>0x02) & 0x01) );
		Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD1_INT_GPIO_PORT_NUM, CLICKBOARD1_INT_GPIO_BIT_NUM, ((nibble >>0x03) & 0x01) );
}else if (iChannel == SLOT2)
{
		Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD2_TX_GPIO_PORT_NUM,  CLICKBOARD2_TX_GPIO_BIT_NUM,  ((nibble >>0x00) & 0x01) );
		Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD2_RX_GPIO_PORT_NUM,  CLICKBOARD2_RX_GPIO_BIT_NUM,  ((nibble >>0x01) & 0x01) );
		Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD2_CS_GPIO_PORT_NUM,  CLICKBOARD2_CS_GPIO_BIT_NUM,  ((nibble >>0x02) & 0x01) );
		Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD2_INT_GPIO_PORT_NUM, CLICKBOARD2_INT_GPIO_BIT_NUM, ((nibble >>0x03) & 0x01) );
}
}

void highToLow(bool rs){
	if (iChannel == SLOT1){
	Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD1_RST_GPIO_PORT_NUM, CLICKBOARD1_RST_GPIO_BIT_NUM, rs);// Send HIGH pulse on RS pin for selecting data register
	Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD1_AN_GPIO_PORT_NUM, CLICKBOARD1_AN_GPIO_BIT_NUM, true); // Generate a High-to-low pulse on EN pin
	vTaskDelay(1);
	Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD1_AN_GPIO_PORT_NUM, CLICKBOARD1_AN_GPIO_BIT_NUM, false);
	}
	else if (iChannel == SLOT2){
	Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD2_RST_GPIO_PORT_NUM, CLICKBOARD2_RST_GPIO_BIT_NUM, rs);// Send HIGH pulse on RS pin for selecting data register
	Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD2_AN_GPIO_PORT_NUM, CLICKBOARD2_AN_GPIO_BIT_NUM, true); // Generate a High-to-low pulse on EN pin
	vTaskDelay(1);
	Chip_GPIO_WritePortBit(LPC_GPIO, CLICKBOARD2_AN_GPIO_PORT_NUM, CLICKBOARD2_AN_GPIO_BIT_NUM, false);
	}
	vTaskDelay(1/*10*/);
}
/* Function to send the command to LCD.
   As it is 4bit mode, a byte of data is sent in two 4-bit nibbles */
void Lcd_CmdWrite(char cmd)
{
    sendNibble((cmd >> 0x04) & 0x0F);  //Send higher nibble
    highToLow(false);
    sendNibble(cmd & 0x0F);            //Send Lower nibble
    highToLow(false);
    vTaskDelay(1/*20*/);
}

void Lcd_DataWrite(char dat)
{
    sendNibble((dat >> 0x04) & 0x0F);  //Send higher nibble
    highToLow(true);
    sendNibble(dat & 0x0F);            //Send higher nibble
    highToLow(true);
}

int LCD_Print(int row, char* text)
{
#define FIRST_LINE 0x80   //128
#define SECOND_LINE 0xC0  //192
#define THIRD_LINE 0x94  //148
#define FOURTH_LINE 0xD4  //212

int i;

   switch(row){
   case 1: Lcd_CmdWrite(FIRST_LINE);break;
   case 2: Lcd_CmdWrite(SECOND_LINE);break;
   case 3: Lcd_CmdWrite(THIRD_LINE);break;
   case 4: Lcd_CmdWrite(FOURTH_LINE);break;
   default : return -1;
   }


   for(i=0;text[i]!=0;i++)
   {
       if (i >1000) return -1;  //return error if endless string
	   Lcd_DataWrite(text[i]);
   }

   return 1;
}

void GpioLcdInit(){
	if (iChannel == SLOT1)
	{
		Chip_IOCON_PinMux(LPC_IOCON, CLICKBOARD1_TX_GPIO_PORT_NUM, CLICKBOARD1_TX_GPIO_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC0);
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD1_TX_GPIO_PORT_NUM, CLICKBOARD1_TX_GPIO_BIT_NUM, GPIO_OUTPUT);   //lcd data bit 4

		Chip_IOCON_PinMux(LPC_IOCON, CLICKBOARD1_RX_GPIO_PORT_NUM, CLICKBOARD1_RX_GPIO_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC0);
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD1_RX_GPIO_PORT_NUM, CLICKBOARD1_RX_GPIO_BIT_NUM, GPIO_OUTPUT);   //lcd data bit 5

		Chip_IOCON_PinMux(LPC_IOCON, CLICKBOARD1_CS_GPIO_PORT_NUM, CLICKBOARD1_CS_GPIO_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC0);
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD1_CS_GPIO_PORT_NUM, CLICKBOARD1_CS_GPIO_BIT_NUM, GPIO_OUTPUT);   //lcd data bit 6

		Chip_IOCON_PinMux(LPC_IOCON, CLICKBOARD1_INT_GPIO_PORT_NUM, CLICKBOARD1_INT_GPIO_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC0);
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD1_INT_GPIO_PORT_NUM, CLICKBOARD1_INT_GPIO_BIT_NUM, GPIO_OUTPUT); //lcd data bit 7

		Chip_IOCON_PinMux(LPC_IOCON, CLICKBOARD1_RST_GPIO_PORT_NUM, CLICKBOARD1_RST_GPIO_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC0);
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD1_RST_GPIO_PORT_NUM, CLICKBOARD1_RST_GPIO_BIT_NUM, GPIO_OUTPUT); //lcd control rs

		Chip_IOCON_PinMux(LPC_IOCON, CLICKBOARD1_AN_GPIO_PORT_NUM, CLICKBOARD1_AN_GPIO_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC0);
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD1_AN_GPIO_PORT_NUM, CLICKBOARD1_AN_GPIO_BIT_NUM, GPIO_OUTPUT);   //lcd control en
	}
	else if (iChannel == SLOT2)
	{
		Chip_IOCON_PinMux(LPC_IOCON, CLICKBOARD2_TX_GPIO_PORT_NUM, CLICKBOARD2_TX_GPIO_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC0);
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD2_TX_GPIO_PORT_NUM, CLICKBOARD2_TX_GPIO_BIT_NUM, GPIO_OUTPUT);   //lcd data bit 4

		Chip_IOCON_PinMux(LPC_IOCON, CLICKBOARD2_RX_GPIO_PORT_NUM, CLICKBOARD2_RX_GPIO_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC0);
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD2_RX_GPIO_PORT_NUM, CLICKBOARD2_RX_GPIO_BIT_NUM, GPIO_OUTPUT);   //lcd data bit 5

		Chip_IOCON_PinMux(LPC_IOCON, CLICKBOARD2_CS_GPIO_PORT_NUM, CLICKBOARD2_CS_GPIO_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC0);
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD2_CS_GPIO_PORT_NUM, CLICKBOARD2_CS_GPIO_BIT_NUM, GPIO_OUTPUT);   //lcd data bit 6

		Chip_IOCON_PinMux(LPC_IOCON, CLICKBOARD2_INT_GPIO_PORT_NUM, CLICKBOARD2_INT_GPIO_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC0);
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD2_INT_GPIO_PORT_NUM, CLICKBOARD2_INT_GPIO_BIT_NUM, GPIO_OUTPUT); //lcd data bit 7

		Chip_IOCON_PinMux(LPC_IOCON, CLICKBOARD2_RST_GPIO_PORT_NUM, CLICKBOARD2_RST_GPIO_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC0);
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD2_RST_GPIO_PORT_NUM, CLICKBOARD2_RST_GPIO_BIT_NUM, GPIO_OUTPUT); //lcd control rs

		Chip_IOCON_PinMux(LPC_IOCON, CLICKBOARD2_AN_GPIO_PORT_NUM, CLICKBOARD2_AN_GPIO_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC0);
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD2_AN_GPIO_PORT_NUM, CLICKBOARD2_AN_GPIO_BIT_NUM, GPIO_OUTPUT);   //lcd control en
	}
}


int LCD_Init()
{
	GpioLcdInit();



    Lcd_CmdWrite(0x03);                   // Initialize Lcd in 4-bit mode
    Lcd_CmdWrite(0x03);                   // Initialize Lcd in 4-bit mode
    Lcd_CmdWrite(0x03);                   // Initialize Lcd in 4-bit mode
    Lcd_CmdWrite(0x02);                   // Initialize Lcd in 4-bit mode


    Lcd_CmdWrite(0x28);                   // enable 5x7 mode for chars
    Lcd_CmdWrite(0x0C);                   // Display ON, Cursor OFF
    Lcd_CmdWrite(0x01);                   // Clear Display
    Lcd_CmdWrite(0x06);                   // Entry mode: Move right, no shift


    /*LCD Test*/
    LCD_Print(1,  "devolo");
    LCD_Print(2, "dLAN green PHY");
    LCD_Print(3,  "evalboard");
    LCD_Print(4, "clickboard support");

  return 1;
}



void fToLCD( int row, char *name, float f, char *unit){
char tmp[4];
char ch[25];
itoa((int)f,tmp,10);
strcpy(ch,name);
if (f < 10) strcat(ch," ");
if (f < 100) strcat(ch," ");
strcat(ch,tmp);
strcat(ch,".");
itoa(((int)(f*10)%10),tmp,10);
strcat(ch,tmp);
strcat(ch,unit);
LCD_Print(row, ch);
}

void iToLCD( int row, char *name, int i, char *unit){
char tmp[8];
char ch[25];
	itoa(i,tmp,10);
	strcpy(ch,name);
	if (i < 10) strcat(ch," ");
	if (i < 100) strcat(ch," ");
	if (i < 1000) strcat(ch," ");
	if (i < 10000) strcat(ch," ");
	strcat(ch,tmp);
	if (i < 100000) strcat(ch," ");
	strcat(ch,unit);
	LCD_Print(row, ch);
}

void Devolo_Lcd_Click_Task(void *pvParameters){
	iChannel = (int)pvParameters;
	LCD_Init();

	int i = 0;

	while (1){

		vTaskDelay(1000);
		i++;
		if (i > 9999999) i = 0;
		iToLCD(1,"Uptime:",i,"seconds");
	}
}


