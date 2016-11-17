
#ifndef LCD_H_
#define LCD_H_

//void delay(int cnt);
//void sendNibble(char nibble);
//void Lcd_CmdWrite(char cmd);
//void Lcd_DataWrite(char dat);

/* Configure the data bus and Control bus as per the hardware connection */
#define LcdDataBusPort      LPC_GPIO2->FIOPIN
#define LcdControlBusPort   LPC_GPIO1->FIOPIN

#define LcdDataBusDirnReg   LPC_GPIO2->FIODIR
#define LcdCtrlBusDirnReg   LPC_GPIO1->FIODIR

#define LCD_D4     0
#define LCD_D5     1
#define LCD_D6     2
#define LCD_D7     3

#define LCD_RS     26
#define LCD_RW     28 /*not used*/
#define LCD_EN     31


#define FIRST_LINE 0x80
#define SECOND_LINE 0xC0
#define THIRD_LINE 0x94
#define FOURTH_LINE 0xD4



/* Masks for configuring the DataBus and Control Bus direction */
#define LCD_ctrlBusMask ((1<<LCD_RS)|(1<<LCD_RW)|(1<<LCD_EN))
#define LCD_dataBusMask   ((1<<LCD_D4)|(1<<LCD_D5)|(1<<LCD_D6)|(1<<LCD_D7))


void LCD_Init(void);
void LCD_Print(int, char*);
void set_custom_line4();

#endif /*LCD_H_*/
