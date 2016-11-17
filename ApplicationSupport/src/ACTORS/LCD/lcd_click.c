#include<lpc17xx.h>
#include "lcd_click.h"



/* local function to generate some delay */
void delay(int cnt)
{
    int i;
    for(i=0;i<cnt;i++);
}





/* Function send the a nibble on the Data bus (LCD_D4 to LCD_D7) */
void sendNibble(char nibble)
{
    LcdDataBusPort&=~(LCD_dataBusMask);                   // Clear previous data
    LcdDataBusPort|= (((nibble >>0x00) & 0x01) << LCD_D4);
    LcdDataBusPort|= (((nibble >>0x01) & 0x01) << LCD_D5);
    LcdDataBusPort|= (((nibble >>0x02) & 0x01) << LCD_D6);
    LcdDataBusPort|= (((nibble >>0x03) & 0x01) << LCD_D7);
}


/* Function to send the command to LCD.
   As it is 4bit mode, a byte of data is sent in two 4-bit nibbles */
void Lcd_CmdWrite(char cmd)
{
    sendNibble((cmd >> 0x04) & 0x0F);  //Send higher nibble
    LcdControlBusPort &= ~(1<<LCD_RS); // Send LOW pulse on RS pin for selecting Command register
    LcdControlBusPort &= ~(1<<LCD_RW); // Send LOW pulse on RW pin for Write operation
    LcdControlBusPort |= (1<<LCD_EN);  // Generate a High-to-low pulse on EN pin
    delay(1000);
    //vTaskDelay(1);
    LcdControlBusPort &= ~(1<<LCD_EN);

    delay(10000);
    //vTaskDelay(1);

    sendNibble(cmd & 0x0F);            //Send Lower nibble
    LcdControlBusPort &= ~(1<<LCD_RS); // Send LOW pulse on RS pin for selecting Command register
    LcdControlBusPort &= ~(1<<LCD_RW); // Send LOW pulse on RW pin for Write operation
    LcdControlBusPort |= (1<<LCD_EN);  // Generate a High-to-low pulse on EN pin
    delay(1000);
    //vTaskDelay(1);
    LcdControlBusPort &= ~(1<<LCD_EN);

    delay(30000);
    //vTaskDelay(3);
}



void Lcd_DataWrite(char dat)
{
    sendNibble((dat >> 0x04) & 0x0F);  //Send higher nibble
    LcdControlBusPort |= (1<<LCD_RS);  // Send HIGH pulse on RS pin for selecting data register
    LcdControlBusPort &= ~(1<<LCD_RW); // Send LOW pulse on RW pin for Write operation
    LcdControlBusPort |= (1<<LCD_EN);  // Generate a High-to-low pulse on EN pin
    delay(1000);
    //vTaskDelay(1);
    LcdControlBusPort &= ~(1<<LCD_EN);

    delay(10000);
    //vTaskDelay(1);

    sendNibble(dat & 0x0F);            //Send higher nibble
    LcdControlBusPort |= (1<<LCD_RS);  // Send HIGH pulse on RS pin for selecting data register
    LcdControlBusPort &= ~(1<<LCD_RW); // Send LOW pulse on RW pin for Write operation
    LcdControlBusPort |= (1<<LCD_EN);  // Generate a High-to-low pulse on EN pin
    delay(1000);
    //vTaskDelay(1);
    LcdControlBusPort &= ~(1<<LCD_EN);

    delay(10000);
    //vTaskDelay(1);
}

void LCD_Print(int row, char* text)
{
   int i;
   Lcd_CmdWrite(row);
   for(i=0;text[i]!=0;i++)
   {
       Lcd_DataWrite(text[i]);
   }
}

void LCD_Init(void)
{
    LcdDataBusDirnReg = LCD_dataBusMask;  // Configure all the LCD pins as output
    LcdCtrlBusDirnReg = LCD_ctrlBusMask;



    Lcd_CmdWrite(0x03);                   // Initialize Lcd in 4-bit mode
    Lcd_CmdWrite(0x03);                   // Initialize Lcd in 4-bit mode
    Lcd_CmdWrite(0x03);                   // Initialize Lcd in 4-bit mode
    Lcd_CmdWrite(0x02);                   // Initialize Lcd in 4-bit mode


    Lcd_CmdWrite(0x28);                   // enable 5x7 mode for chars
    Lcd_CmdWrite(0x0C);                   // Display ON, Cursor OFF
    Lcd_CmdWrite(0x01);                   // Clear Display
    Lcd_CmdWrite(0x06);                   // Entry mode: Move right, no shift


    /*LCD Test*/
    LCD_Print(FIRST_LINE,"devolo");
    LCD_Print(SECOND_LINE,"dLAN green PHY");
    LCD_Print(THIRD_LINE,"evalboard");
    LCD_Print(FOURTH_LINE,"Temperature");
}

int line4_flag = 0;
void set_custom_line4() {
    line4_flag = 1;
}

int get_custom_line4() {
    return line4_flag;
}


//int main()
//{
//    char i,a[]={"Good morning!"};
//    SystemInit();                         //Clock and PLL configuration
//
//    LcdDataBusDirnReg = LCD_dataBusMask;  // Configure all the LCD pins as output
//    LcdCtrlBusDirnReg = LCD_ctrlBusMask;
//
//
//    Lcd_CmdWrite(0x02);                   // Initialize Lcd in 4-bit mode
//    Lcd_CmdWrite(0x28);                   // enable 5x7 mode for chars
//    Lcd_CmdWrite(0x0E);                   // Display OFF, Cursor ON
//    Lcd_CmdWrite(0x01);                   // Clear Display
//    Lcd_CmdWrite(0x80);                   // Move the cursor to beginning of first line
//
//
//    Lcd_DataWrite('H');
//    Lcd_DataWrite('e');
//    Lcd_DataWrite('l');
//    Lcd_DataWrite('l');
//    Lcd_DataWrite('o');
//    Lcd_DataWrite(' ');
//    Lcd_DataWrite('w');
//    Lcd_DataWrite('o');
//    Lcd_DataWrite('r');
//    Lcd_DataWrite('l');
//    Lcd_DataWrite('d');
//
//    Lcd_CmdWrite(0xc0);
//    for(i=0;a[i]!=0;i++)
//    {
//        Lcd_DataWrite(a[i]);
//    }
//
//    while(1);
//}
