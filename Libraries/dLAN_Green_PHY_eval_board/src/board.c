/*
 * @brief devolo dLAN Green PHY Module eval board
 *
 * @note
 * Copyright(C) devolo AG, 2016
 * All rights reserved.
 *
 *
 *
 */

#include "board.h"
#include "string.h"

#include "retarget.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/
#define GPIO_OUTPUT true
#define GPIO_INPUT false

#define BUTTONS_BUTTON1_GPIO_PORT_NUM             2
#define BUTTONS_BUTTON1_GPIO_BIT_NUM              10

#define CLICKBOARD1_CS_GPIO_PORT_NUM              2
#define CLICKBOARD1_CS_GPIO_BIT_NUM               2

#define CLICKBOARD1_INT_GPIO_PORT_NUM             2
#define CLICKBOARD1_INT_GPIO_BIT_NUM              3

#define CLICKBOARD1_PWM_GPIO_PORT_NUM             2
#define CLICKBOARD1_PWM_GPIO_BIT_NUM              4

#define CLICKBOARD1_RST_GPIO_PORT_NUM             1
#define CLICKBOARD1_RST_GPIO_BIT_NUM              26

#define CLICKBOARD1_TX_GPIO_PORT_NUM              2
#define CLICKBOARD1_TX_GPIO_BIT_NUM               0

#define CLICKBOARD1_RX_GPIO_PORT_NUM              2
#define CLICKBOARD1_RX_GPIO_BIT_NUM               1

#define CLICKBOARD1_AN_GPIO_PORT_NUM              1
#define CLICKBOARD1_AN_GPIO_BIT_NUM               31

#define CLICKBOARD2_CS_GPIO_PORT_NUM              2
#define CLICKBOARD2_CS_GPIO_BIT_NUM               7

#define CLICKBOARD2_INT_GPIO_PORT_NUM             2
#define CLICKBOARD2_INT_GPIO_BIT_NUM              6

#define CLICKBOARD2_PWM_GPIO_PORT_NUM             2
#define CLICKBOARD2_PWM_GPIO_BIT_NUM              5

#define CLICKBOARD2_RST_GPIO_PORT_NUM             1
#define CLICKBOARD2_RST_GPIO_BIT_NUM              28

#define CLICKBOARD2_TX_GPIO_PORT_NUM              0
#define CLICKBOARD2_TX_GPIO_BIT_NUM               10

#define CLICKBOARD2_RX_GPIO_PORT_NUM              0
#define CLICKBOARD2_RX_GPIO_BIT_NUM               11

#define CLICKBOARD2_AN_GPIO_PORT_NUM              0
#define CLICKBOARD2_AN_GPIO_BIT_NUM               25

#define J12_GPP_GPIO_PORT_NUM                     0
#define J12_GPP_GPIO_BIT_NUM                      26

#define J10_PIN4_TX_GPIO_PORT_NUM                 0
#define J10_PIN4_TX_GPIO_BIT_NUM                  3

#define J10_PIN5_RX_GPIO_PORT_NUM                 0
#define J10_PIN5_RX_GPIO_BIT_NUM                  2

#define USB_PLUS_RX_GPIO_PORT_NUM                 0
#define USB_PLUS_RX_GPIO_BIT_NUM                  29

#define USB_MINUS_RX_GPIO_PORT_NUM                 0
#define USB_MINUS_RX_GPIO_BIT_NUM                  30

#define CLICKBOARD_SCK_SPI1_PORT_NUM                 0
#define CLICKBOARD_SCK_SPI1_BIT_NUM                  7

#define CLICKBOARD_MISO_SPI1_PORT_NUM                 0
#define CLICKBOARD_MISO_SPI1_BIT_NUM                  8

#define CLICKBOARD_MOSI_SPI1_PORT_NUM                 0
#define CLICKBOARD_MOSI_SPI1_BIT_NUM                  9

#define CLICKBOARD1_CS_SPI1_PORT_NUM                 2
#define CLICKBOARD1_CS_SPI1_BIT_NUM                  2

#define CLICKBOARD2_CS_SPI1_PORT_NUM                 2
#define CLICKBOARD2_CS_SPI1_BIT_NUM                  7

#define GreenPHY_SCK_SPI0_PORT_NUM                 0
#define GreenPHY_SCK_SPI0_BIT_NUM                  15

#define GreenPHY_MISO_SPI0_PORT_NUM                 0
#define GreenPHY_MISO_SPI0_BIT_NUM                  17

#define GreenPHY_MOSI_SPI0_PORT_NUM                 0
#define GreenPHY_MOSI_SPI0_BIT_NUM                  18

#define GreenPHY_SSEL_SPI0_PORT_NUM                 0
#define GreenPHY_SSEL_SPI0_BIT_NUM                  16

#define GreenPHY_INT_SPI0_PORT_NUM                 0
#define GreenPHY_INT_SPI0_BIT_NUM                  22

#define CLICKBOARD_SDA_I2C1_PORT_NUM                 0
#define CLICKBOARD_SDA_I2C1_BIT_NUM                  0

#define CLICKBOARD_SCL_I2C1_PORT_NUM                 0
#define CLICKBOARD_SCL_I2C1_BIT_NUM                  1

#define CLICKBOARD2_SDA_I2C2_PORT_NUM                 0
#define CLICKBOARD2_SDA_I2C2_BIT_NUM                  10

#define CLICKBOARD2_SCL_I2C2_PORT_NUM                 0
#define CLICKBOARD2_SCL_I2C2_BIT_NUM                  11

#define POSITIVE_USB_PORT_NUM                 0
#define POSITIVE_USB_BIT_NUM                  30

#define NEGATIVE_USB_PORT_NUM                 0
#define NEGATIVE_USB_BIT_NUM                  29

#define VBUS_USB_PORT_NUM                 1
#define VBUS_USB_BIT_NUM                  30


//#define JOYSTICK_UP_GPIO_PORT_NUM               2  //not used on devolo evaluation board
//#define JOYSTICK_UP_GPIO_BIT_NUM                3  //not used on devolo evaluation board
//#define JOYSTICK_DOWN_GPIO_PORT_NUM             0  //not used on devolo evaluation board
//#define JOYSTICK_DOWN_GPIO_BIT_NUM              15 //not used on devolo evaluation board
//#define JOYSTICK_LEFT_GPIO_PORT_NUM             2  //not used on devolo evaluation board
//#define JOYSTICK_LEFT_GPIO_BIT_NUM              4  //not used on devolo evaluation board
//#define JOYSTICK_RIGHT_GPIO_PORT_NUM            0  //not used on devolo evaluation board
//#define JOYSTICK_RIGHT_GPIO_BIT_NUM             16 //not used on devolo evaluation board
//#define JOYSTICK_PRESS_GPIO_PORT_NUM            0  //not used on devolo evaluation board
//#define JOYSTICK_PRESS_GPIO_BIT_NUM             17 //not used on devolo evaluation board
#define LED0_GPIO_PORT_NUM                      0
#define LED0_GPIO_BIT_NUM                       6

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/* System oscillator rate and RTC oscillator rate */
const uint32_t OscRateIn = 12000000;
const uint32_t RTCOscRateIn = 32768;

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* Initializes board LED(s) */
static void Board_LED_Init(void)
{
	/* Pin PIO0_6 is configured as GPIO pin during SystemInit */
	/* Set the PIO_6 as output */
	Chip_GPIO_WriteDirBit(LPC_GPIO, LED0_GPIO_PORT_NUM, LED0_GPIO_BIT_NUM, GPIO_OUTPUT);
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Initialize UART pins */
void Board_UART_Init(LPC_USART_T *pUART)
{
	/* Pin Muxing has already been done during SystemInit */
}

/* Initialize debug output via UART for board */
void Board_Debug_Init(void)
{
#if defined(DEBUG_ENABLE)
	Board_UART_Init(DEBUG_UART);

	Chip_UART_Init(DEBUG_UART);
	Chip_UART_SetBaud(DEBUG_UART, 115200);
	Chip_UART_ConfigData(DEBUG_UART, UART_LCR_WLEN8 | UART_LCR_SBS_1BIT | UART_LCR_PARITY_DIS);

	/* Enable UART Transmit */
	Chip_UART_TXEnable(DEBUG_UART);
#endif
}

/* Sends a character on the UART */
void Board_UARTPutChar(char ch)
{
#if defined(DEBUG_ENABLE)
	while ((Chip_UART_ReadLineStatus(DEBUG_UART) & UART_LSR_THRE) == 0) {}
	Chip_UART_SendByte(DEBUG_UART, (uint8_t) ch);
#endif
}

/* Gets a character from the UART, returns EOF if no character is ready */
int Board_UARTGetChar(void)
{
#if defined(DEBUG_ENABLE)
	if (Chip_UART_ReadLineStatus(DEBUG_UART) & UART_LSR_RDR) {
		return (int) Chip_UART_ReadByte(DEBUG_UART);
	}
#endif
	return EOF;
}

/* Outputs a string on the debug UART */
void Board_UARTPutSTR(char *str)
{
#if defined(DEBUG_ENABLE)
	while (*str != '\0') {
		Board_UARTPutChar(*str++);
	}
#endif
}

/* Sets the state of a board LED to on or off */
void Board_LED_Set(uint8_t LEDNumber, bool Status)
{
	/* There is only one LED */
	if (LEDNumber == LEDS_LED1) {
		Chip_GPIO_WritePortBit(LPC_GPIO, LED0_GPIO_PORT_NUM, LED0_GPIO_BIT_NUM, Status);
	}
}

/* Returns the current state of a board LED */
bool Board_LED_Test(uint8_t LEDNumber)
{
	bool state = false;

	if (LEDNumber == LEDS_LED1) {
		state = Chip_GPIO_ReadPortBit(LPC_GPIO, LED0_GPIO_PORT_NUM, LED0_GPIO_BIT_NUM);
	}

	return state;
}

void Board_LED_Toggle(uint8_t LEDNumber)
{
	if (LEDNumber == LEDS_LED1) {
		Board_LED_Set(LEDNumber, !Board_LED_Test(LEDNumber));
	}
}

/* Set up and initialize all required blocks and functions related to the
   board hardware */
void Board_Init(void)
{
	/* Sets up DEBUG UART */
	DEBUGINIT();

	/* Initializes GPIO */
	Chip_GPIO_Init(LPC_GPIO);
	Chip_IOCON_Init(LPC_IOCON);

	/* Initialize LEDs */
	Board_LED_Init();
}

/* Returns the MAC address assigned to this board */
void Board_ENET_GetMacADDR(uint8_t *mcaddr)
{
	const uint8_t boardmac[] = {0x00, 0x60, 0x37, 0x12, 0x34, 0x56};

	memcpy(mcaddr, boardmac, 6);
}

/* Initialize pin muxing for SSP interface
 * The SSP peripheral is a multi protocol peripheral which is also able to function as an SPI interface.
 * The CS(SSEL) line in SPI remains low for whole frame transfer, in SSP but it toggles after each frame
 * (a frame = 4-16bits _configurable_) in SSP. This apllies only for SPI0, because SPI1 has no soft CS on our board.
 * http://www.keil.com/forum/24071/ssp-vs-spi-interrupt-code-not-compatible/
 * https://www.lpcware.com/content/forum/why-prefer-ssp-instead-spi-everywhere-confused-s*/
void Board_SSP_Init(LPC_SSP_T *pSSP)
{
	if (pSSP == LPC_SSP1) {
		/* Set up clock and muxing for SSP1 interface */
		Chip_IOCON_PinMux(LPC_IOCON, CLICKBOARD_SCK_SPI1_PORT_NUM, CLICKBOARD_SCK_SPI1_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC2);
		Chip_IOCON_PinMux(LPC_IOCON, CLICKBOARD_MISO_SPI1_PORT_NUM, CLICKBOARD_MISO_SPI1_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC2);
		Chip_IOCON_PinMux(LPC_IOCON, CLICKBOARD_MOSI_SPI1_PORT_NUM, CLICKBOARD_MOSI_SPI1_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC2);
		//Chip_IOCON_PinMux(LPC_IOCON, 0, 6, IOCON_MODE_INACT, IOCON_FUNC2);
		/* we do not use standard SSEL Pin here. Instead we use traditional GPIO Pin for SSEL feature and we call it CS:
		 * Set the CS pins as output */
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD1_CS_SPI1_PORT_NUM, CLICKBOARD1_CS_SPI1_BIT_NUM, GPIO_OUTPUT);
		Chip_GPIO_WriteDirBit(LPC_GPIO, CLICKBOARD2_CS_SPI1_PORT_NUM, CLICKBOARD2_CS_SPI1_BIT_NUM, GPIO_OUTPUT);
	}
	else {
		/*!!! It is better to use the SPI init for SPI0 --> void Board_SPI_Init(bool isMaster) which is in this *.h file
		 *  Set up clock and muxing for SSP0 interface  */
		Chip_IOCON_PinMux(LPC_IOCON, GreenPHY_SCK_SPI0_PORT_NUM,  GreenPHY_SCK_SPI0_BIT_NUM,  IOCON_MODE_INACT, IOCON_FUNC2);
		Chip_IOCON_PinMux(LPC_IOCON, GreenPHY_SSEL_SPI0_PORT_NUM,   GreenPHY_SSEL_SPI0_BIT_NUM,   IOCON_MODE_INACT, IOCON_FUNC2);
		Chip_IOCON_PinMux(LPC_IOCON, GreenPHY_MISO_SPI0_PORT_NUM, GreenPHY_MISO_SPI0_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC2);
		Chip_IOCON_PinMux(LPC_IOCON, GreenPHY_MOSI_SPI0_PORT_NUM, GreenPHY_MOSI_SPI0_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC2);
		/* we also have an external interrupt from external greenPHY module which can be handled by the application
		 * Set the INT pin as input */
		Chip_GPIO_WriteDirBit(LPC_GPIO, GreenPHY_INT_SPI0_PORT_NUM, GreenPHY_INT_SPI0_BIT_NUM, GPIO_INPUT);
	}
}

/* Initialize pin muxing for SPI0 GreenPHY interface */
void Board_SPI_Init(bool isMaster)
{
	/* Set up clock and muxing for SSP0 interface */
	Chip_IOCON_PinMux(LPC_IOCON, GreenPHY_SCK_SPI0_PORT_NUM, GreenPHY_SCK_SPI0_BIT_NUM, IOCON_MODE_PULLDOWN, IOCON_FUNC3);

	if (isMaster) {
		Chip_IOCON_PinMux(LPC_IOCON, GreenPHY_SSEL_SPI0_PORT_NUM, GreenPHY_SSEL_SPI0_BIT_NUM, IOCON_MODE_PULLUP, IOCON_FUNC0);
		Chip_GPIO_WriteDirBit(LPC_GPIO, GreenPHY_SSEL_SPI0_PORT_NUM, GreenPHY_SSEL_SPI0_BIT_NUM, GPIO_OUTPUT);
		Board_SPI_DeassertSSEL();

	}
	else {
		Chip_IOCON_PinMux(LPC_IOCON, GreenPHY_SSEL_SPI0_PORT_NUM, GreenPHY_SSEL_SPI0_BIT_NUM, IOCON_MODE_PULLUP, IOCON_FUNC3);
	}
	Chip_IOCON_PinMux(LPC_IOCON, GreenPHY_MISO_SPI0_PORT_NUM, GreenPHY_MISO_SPI0_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC3);
	Chip_IOCON_PinMux(LPC_IOCON, GreenPHY_MOSI_SPI0_PORT_NUM, GreenPHY_MOSI_SPI0_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC3);
}

/* Assert SSEL pin */
void Board_SPI_AssertSSEL(void)
{
	Chip_GPIO_WritePortBit(LPC_GPIO, GreenPHY_SSEL_SPI0_PORT_NUM, GreenPHY_SSEL_SPI0_BIT_NUM, GPIO_INPUT);
}

/* De-Assert SSEL pin */
void Board_SPI_DeassertSSEL(void)
{
	Chip_GPIO_WritePortBit(LPC_GPIO, GreenPHY_SSEL_SPI0_PORT_NUM, GreenPHY_SSEL_SPI0_BIT_NUM, GPIO_OUTPUT);
}



/*not used on devolo evaluation board*/
//void Board_Audio_Init(LPC_I2S_T *pI2S, int micIn)
//{
//	I2S_AUDIO_FORMAT_T I2S_Config;
//
//	/* Chip_Clock_EnablePeripheralClock(SYSCTL_CLOCK_I2S); */
//
//	I2S_Config.SampleRate = 48000;
//	I2S_Config.ChannelNumber = 2;	/* 1 is mono, 2 is stereo */
//	I2S_Config.WordWidth =  16;		/* 8, 16 or 32 bits */
//	Chip_I2S_Init(pI2S);
//	Chip_I2S_TxConfig(pI2S, &I2S_Config);
//}

/* Sets up board specific I2C interface */
void Board_I2C_Init(I2C_ID_T id)
{
	switch (id) {
	case I2C0:
		/*not available on devolo evaluation board*/
		Chip_IOCON_PinMux(LPC_IOCON, 0, 27, IOCON_MODE_INACT, IOCON_FUNC1);
		Chip_IOCON_PinMux(LPC_IOCON, 0, 28, IOCON_MODE_INACT, IOCON_FUNC1);
		Chip_IOCON_SetI2CPad(LPC_IOCON, I2CPADCFG_STD_MODE);
		break;

	case I2C1:
		/*official clickboard I2C interface*/
		Chip_IOCON_PinMux(LPC_IOCON, CLICKBOARD_SDA_I2C1_PORT_NUM, CLICKBOARD_SDA_I2C1_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC3);
		Chip_IOCON_PinMux(LPC_IOCON, CLICKBOARD_SCL_I2C1_PORT_NUM, CLICKBOARD_SCL_I2C1_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC3);
		Chip_IOCON_EnableOD(LPC_IOCON, CLICKBOARD_SDA_I2C1_PORT_NUM, CLICKBOARD_SDA_I2C1_BIT_NUM);
		Chip_IOCON_EnableOD(LPC_IOCON, CLICKBOARD_SCL_I2C1_PORT_NUM, CLICKBOARD_SCL_I2C1_BIT_NUM);
		break;

	case I2C2:
		/*RX and TX pin on clickboard2 I2C interface, normally used by UART*/
		Chip_IOCON_PinMux(LPC_IOCON, CLICKBOARD2_SDA_I2C2_PORT_NUM, CLICKBOARD2_SDA_I2C2_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC2);
		Chip_IOCON_PinMux(LPC_IOCON, CLICKBOARD2_SCL_I2C2_PORT_NUM, CLICKBOARD2_SCL_I2C2_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC2);
		Chip_IOCON_EnableOD(LPC_IOCON, CLICKBOARD2_SDA_I2C2_PORT_NUM, CLICKBOARD2_SDA_I2C2_BIT_NUM);
		Chip_IOCON_EnableOD(LPC_IOCON, CLICKBOARD2_SCL_I2C2_PORT_NUM, CLICKBOARD2_SCL_I2C2_BIT_NUM);
		break;
	case I2C_NUM_INTERFACE:
		break;
	}
}

void Board_Buttons_Init(void)
{
	Chip_GPIO_WriteDirBit(LPC_GPIO, BUTTONS_BUTTON1_GPIO_PORT_NUM, BUTTONS_BUTTON1_GPIO_BIT_NUM, false);
}

uint32_t Buttons_GetStatus(void)
{
	uint8_t ret = NO_BUTTON_PRESSED;
	if (Chip_GPIO_ReadPortBit(LPC_GPIO, BUTTONS_BUTTON1_GPIO_PORT_NUM, BUTTONS_BUTTON1_GPIO_BIT_NUM) == 0x00) {
		ret |= BUTTONS_BUTTON1;
	}
	return ret;
}

/*not used on devolo evaluation board*/
///* Baseboard joystick buttons */
//#define NUM_BUTTONS 5
//static const uint8_t portButton[NUM_BUTTONS] = {
//	JOYSTICK_UP_GPIO_PORT_NUM,
//	JOYSTICK_DOWN_GPIO_PORT_NUM,
//	JOYSTICK_LEFT_GPIO_PORT_NUM,
//	JOYSTICK_RIGHT_GPIO_PORT_NUM,
//	JOYSTICK_PRESS_GPIO_PORT_NUM
//};

/*not used on devolo evaluation board*/
//static const uint8_t pinButton[NUM_BUTTONS] = {
//	JOYSTICK_UP_GPIO_BIT_NUM,
//	JOYSTICK_DOWN_GPIO_BIT_NUM,
//	JOYSTICK_LEFT_GPIO_BIT_NUM,
//	JOYSTICK_RIGHT_GPIO_BIT_NUM,
//	JOYSTICK_PRESS_GPIO_BIT_NUM
//};

/*not used on devolo evaluation board*/
//static const uint8_t stateButton[NUM_BUTTONS] = {
//	JOY_UP,
//	JOY_DOWN,
//	JOY_LEFT,
//	JOY_RIGHT,
//	JOY_PRESS
//};
//

/*not used on devolo evaluation board*/
///* Initialize Joystick */
//void Board_Joystick_Init(void)
//{
//	int ix;
//
//	/* IOCON states already selected in SystemInit(), GPIO setup only. Pullups
//	   are external, so IOCON with no states */
//	for (ix = 0; ix < NUM_BUTTONS; ix++) {
//		Chip_GPIO_SetPinDIRInput(LPC_GPIO, portButton[ix], pinButton[ix]);
//	}
//}
//

/*not used on devolo evaluation board*/
///* Get Joystick status */
//uint8_t Joystick_GetStatus(void)
//{
//	uint8_t ix, ret = 0;
//
//	for (ix = 0; ix < NUM_BUTTONS; ix++) {
//		if ((Chip_GPIO_GetPinState(LPC_GPIO, portButton[ix], pinButton[ix])) == false) {
//			ret |= stateButton[ix];
//		}
//	}
//
//	return ret;
//}


void Serial_CreateStream(void *Stream)
{}

void Board_USBD_Init(uint32_t port)
{
	Chip_IOCON_PinMux(LPC_IOCON, VBUS_USB_PORT_NUM, VBUS_USB_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC2);/* USB VBUS */
	
	Chip_IOCON_PinMux(LPC_IOCON, NEGATIVE_USB_PORT_NUM, NEGATIVE_USB_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC1);	//D1+
	Chip_IOCON_PinMux(LPC_IOCON, POSITIVE_USB_PORT_NUM, POSITIVE_USB_BIT_NUM, IOCON_MODE_INACT, IOCON_FUNC1);   //D1-

	LPC_USB->USBClkCtrl = 0x12;                /* Dev, AHB clock enable */
	while ((LPC_USB->USBClkSt & 0x12) != 0x12); 
}

