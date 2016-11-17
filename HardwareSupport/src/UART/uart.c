/***********************************************************************
 * $Id::                                                               $
 *
 * Project:	uart: Simple UART echo for LPCXpresso 1700
 * File:	uart.c
 * Description:
 * 			LPCXpresso Baseboard uses pins mapped to UART3 for
 * 			its USB-to-UART bridge. This application simply echos
 * 			all characters received.
 *
 ***********************************************************************
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * products. This software is supplied "AS IS" without any warranties.
 * NXP Semiconductors assumes no responsibility or liability for the
 * use of the software, conveys no license or title under any patent,
 * copyright, or mask work right to the product. NXP Semiconductors
 * reserves the right to make changes in the software without
 * notification. NXP Semiconductors also make no representation or
 * warranty that such application will be suitable for the specified
 * use without further testing or modification.
 **********************************************************************/

/*****************************************************************************
 *   History
 *   2010.07.01  ver 1.01    Added support for UART3, tested on LPCXpresso 1700
 *   2009.05.27  ver 1.00    Prelimnary version, first Release
 *
******************************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <stdio.h>
#include <string.h>

#include "lpc_sc.h"
#include "LPC17xx.h"
#include "lpc17xx_gpio.h"
#include "uart.h"
#include "debug.h"

struct UART;
void (*IRQHandler) (struct UART * uart);

struct UART {
	xSemaphoreHandle rxSemaphore;
	volatile uint32_t status;
	volatile uint8_t empty;
	volatile uint8_t rx;
	volatile uint8_t buffer[BUFSIZE];
	volatile uint32_t count;
	volatile uint32_t overflow;
	void * registerPointer;
	IRQn_Type IRQn;
	void (*IRQHandler) (struct UART * uart);
	void (*UARTInit)(struct UART * uart, uint32_t baudrate );
	void (*UARTSend)(struct UART * uart, uint8_t *BufferPtr, uint32_t Length );
};

/*****************************************************************************
** Function name:		UART_Generic_Init
**
** Descriptions:		generic UART initialisation
**
** parameters:			struct UART * uart to use
** Returned value:		None
**
*****************************************************************************/
static void UART_Generic_Init (struct UART * uart )
{
	vSemaphoreCreateBinary( uart->rxSemaphore );

	uart->rx = 0;

	NVIC_EnableIRQ(uart->IRQn);
}
/*****************************************************************************
** Function name:		UART_Init_done
**
** Descriptions:		Empty function used after initialisation.
**
** parameters:			struct UART * uart to use, uint32_t baudrate
** Returned value:		None
**
*****************************************************************************/
static void UART_Init_done(struct UART * uart, uint32_t baudrate )
{
}
/*****************************************************************************
** Function name:		UART_0_Init
**
** Descriptions:		UART0 initialisation
**
** parameters:			struct UART * uart to use, uint32_t baudrate
** Returned value:		None
**
*****************************************************************************/
static void UART_0_Init (struct UART * uart, uint32_t baudrate )
{
	uint32_t Fdiv;
	uint32_t pclkdiv, pclk;
	pclkdiv = 1;

	LPC_UART_TypeDef      * registers = (LPC_UART_TypeDef *)uart->registerPointer;

	/* Power Up the UART0 controller. */
	LPC_SC->PCONP |= PCUART0;

	/* set the peripheral clock */
	LPC_SC->PCLKSEL0 &= (~(0x3<<6));
	LPC_SC->PCLKSEL0 |= (pclkdiv<<6);

	LPC_PINCON->PINSEL0 &= ~(0x3<<4); // clear P0.2
	LPC_PINCON->PINSEL0 &= ~(0x3<<6); // clear P0.3

	LPC_PINCON->PINSEL0 |= (0x01<<4); // use P0.2 as TxD0
	LPC_PINCON->PINSEL0 |= (0x01<<6); // use P0.3 as RxD0

	/* By default, the PCLKSELx value is zero, thus, the PCLK for
		all the peripherals is 1/4 of the SystemFrequency. */
	switch ( pclkdiv )
	{
	case 0x00:
	default:
		pclk = SystemCoreClock/4;
		break;
	case 0x01:
		pclk = SystemCoreClock;
		break;
	case 0x02:
		pclk = SystemCoreClock/2;
		break;
	case 0x03:
		pclk = SystemCoreClock/8;
		break;
	}

	registers->LCR = 0x83;		/* 8 bits, no Parity, 1 Stop bit */
	Fdiv = ( pclk / 16 ) / baudrate ;	/*baud rate */
	registers->DLM = Fdiv / 256;
	registers->DLL = Fdiv % 256;
	registers->LCR = 0x03;		/* DLAB = 0 */
	registers->FCR = 0x07;		/* Enable and reset TX and RX FIFO. */

	registers->IER = IER_RBR | IER_THRE | IER_RLS;	/* Enable UART interrupt */

	UART_Generic_Init(uart);
}

/*****************************************************************************
** Function name:		UART_1_Init
**
** Descriptions:		UART0 initialisation
**
** parameters:			struct UART * uart to use, uint32_t baudrate
** Returned value:		None
**
*****************************************************************************/
static void UART_1_Init (struct UART * uart, uint32_t baudrate )
{
	uint32_t Fdiv;
	uint32_t pclkdiv, pclk;
	pclkdiv = 1;

	LPC_UART1_TypeDef * registers = (LPC_UART1_TypeDef *)uart->registerPointer;

	/* Power Up the UART0 controller. */
	LPC_SC->PCONP |= PCUART1;

	/* set the peripheral clock */
	LPC_SC->PCLKSEL0 &= (~(0x3<<8));
	LPC_SC->PCLKSEL0 |= (pclkdiv<<8);

	LPC_PINCON->PINSEL4 &= ~(0x3<<0); // clear P2.0
	LPC_PINCON->PINSEL4 &= ~(0x3<<2); // clear P2.1

	LPC_PINCON->PINSEL4 |= (0x10<<0); // use P2.0 as TxD1
	LPC_PINCON->PINSEL4 |= (0x10<<2); // use P2.1 as RxD1

	/* By default, the PCLKSELx value is zero, thus, the PCLK for
		all the peripherals is 1/4 of the SystemFrequency. */
	switch ( pclkdiv )
	{
	case 0x00:
	default:
		pclk = SystemCoreClock/4;
		break;
	case 0x01:
		pclk = SystemCoreClock;
		break;
	case 0x02:
		pclk = SystemCoreClock/2;
		break;
	case 0x03:
		pclk = SystemCoreClock/8;
		break;
	}

	registers->LCR = 0x83;		/* 8 bits, no Parity, 1 Stop bit */
	Fdiv = ( pclk / 16 ) / baudrate ;	/*baud rate */
	registers->DLM = Fdiv / 256;
	registers->DLL = Fdiv % 256;
	registers->LCR = 0x03;		/* DLAB = 0 */
	registers->FCR = 0x07;		/* Enable and reset TX and RX FIFO. */

	registers->IER = IER_RBR | IER_THRE | IER_RLS;	/* Enable UART interrupt */

	UART_Generic_Init(uart);

}

/*****************************************************************************
** Function name:		UART_2_Init
**
** Descriptions:		UART2 initialisation
**
** parameters:			struct UART * uart to use, uint32_t baudrate
** Returned value:		None
**
*****************************************************************************/
static void UART_2_Init (struct UART * uart, uint32_t baudrate )
{
	uint32_t Fdiv;
	uint32_t pclkdiv, pclk;
	pclkdiv = 1;

	LPC_UART_TypeDef      * registers = (LPC_UART_TypeDef *)uart->registerPointer;

	/* Power Up the UART2 controller. */
	LPC_SC->PCONP |= PCUART2;

	/* set the peripheral clock */
	LPC_SC->PCLKSEL1 &= (~(0x3<<16));
	LPC_SC->PCLKSEL1 |= (pclkdiv<<16);

	LPC_PINCON->PINSEL0 &= ~(0x3<<20); // clear P0.10
	LPC_PINCON->PINSEL0 &= ~(0x3<<22); // clear P0.11

	LPC_PINCON->PINSEL0 |= (0x01<<20); // use P0.10 as TxD2
	LPC_PINCON->PINSEL0 |= (0x01<<22); // use P0.11 as RxD2

	/* By default, the PCLKSELx value is zero, thus, the PCLK for
		all the peripherals is 1/4 of the SystemFrequency. */
	switch ( pclkdiv )
	{
	case 0x00:
	default:
		pclk = SystemCoreClock/4;
		break;
	case 0x01:
		pclk = SystemCoreClock;
		break;
	case 0x02:
		pclk = SystemCoreClock/2;
		break;
	case 0x03:
		pclk = SystemCoreClock/8;
		break;
	}

	registers->LCR = 0x83;		/* 8 bits, no Parity, 1 Stop bit */
	Fdiv = ( pclk / 16 ) / baudrate ;	/*baud rate */
	registers->DLM = Fdiv / 256;
	registers->DLL = Fdiv % 256;
	registers->LCR = 0x03;		/* DLAB = 0 */
	registers->FCR = 0x07;		/* Enable and reset TX and RX FIFO. */

	registers->IER = IER_RBR | IER_THRE | IER_RLS;	/* Enable UART interrupt */

	UART_Generic_Init(uart);
}

/*****************************************************************************
** Function name:		UART_3_Init
**
** Descriptions:		UART2 initialisation
**
** parameters:			struct UART * uart to use, uint32_t baudrate
** Returned value:		None
**
*****************************************************************************/
static void UART_3_Init (struct UART * uart, uint32_t baudrate )
{
	uint32_t Fdiv;
	uint32_t pclkdiv, pclk;
	pclkdiv = 1;

	LPC_UART_TypeDef      * registers = (LPC_UART_TypeDef *)uart->registerPointer;

	/* Power Up the UART3 controller. */
	LPC_SC->PCONP |= PCUART3;

	/* set the peripheral clock */
	LPC_SC->PCLKSEL1 &= (~(0x3<<18));
	LPC_SC->PCLKSEL1 |= (pclkdiv<<18);

	LPC_PINCON->PINSEL9 &= ~(0x3<<24); // clear P4.28
	LPC_PINCON->PINSEL9 &= ~(0x3<<26); // clear P4.29

	LPC_PINCON->PINSEL9 |= (0x11<<24); // use P4.28 as TxD3
	LPC_PINCON->PINSEL9 |= (0x11<<26); // use P4.29 as RxD3

	/* By default, the PCLKSELx value is zero, thus, the PCLK for
		all the peripherals is 1/4 of the SystemFrequency. */
	switch ( pclkdiv )
	{
	case 0x00:
	default:
		pclk = SystemCoreClock/4;
		break;
	case 0x01:
		pclk = SystemCoreClock;
		break;
	case 0x02:
		pclk = SystemCoreClock/2;
		break;
	case 0x03:
		pclk = SystemCoreClock/8;
		break;
	}

	registers->LCR = 0x83;		/* 8 bits, no Parity, 1 Stop bit */
	Fdiv = ( pclk / 16 ) / baudrate ;	/*baud rate */
	registers->DLM = Fdiv / 256;
	registers->DLL = Fdiv % 256;
	registers->LCR = 0x03;		/* DLAB = 0 */
	registers->FCR = 0x07;		/* Enable and reset TX and RX FIFO. */

	registers->IER = IER_RBR | IER_THRE | IER_RLS;	/* Enable UART interrupt */

	UART_Generic_Init(uart);
}

/*****************************************************************************
** Function name:		UART_1_IRQHandler
**
** Descriptions:		UART interrupt handler for UART1
**
** parameters:			struct UART * uart to use
** Returned value:		None
**
*****************************************************************************/

static void UART_1_IRQHandler (struct UART * uart)
{
	uint8_t IIRValue, LSRValue;
	uint8_t Dummy = Dummy;

	LPC_UART1_TypeDef      * registers = (LPC_UART1_TypeDef *)uart->registerPointer;
	IIRValue = registers->IIR;
	long lHigherPriorityTaskWoken = pdFALSE;

	uint8_t giveSemaphore = 0;

	IIRValue >>= 1;			/* skip pending bit in IIR */
	IIRValue &= 0x07;			/* check bit 1~3, interrupt identification */
	if ( IIRValue == IIR_RLS )		/* Receive Line Status */
	{
		LSRValue = registers->LSR;
		/* Receive Line Status */
		if ( LSRValue & (LSR_OE|LSR_PE|LSR_FE|LSR_RXFE|LSR_BI) )
		{
			/* There are errors or break interrupt */
			/* Read LSR will clear the interrupt */
			uart->status = LSRValue;
			Dummy = registers->RBR;		/* Dummy read on RX to clear
							interrupt, then bail out */
			return;
		}
		if ( LSRValue & LSR_RDR )	/* Receive Data Ready */
		{
			/* If no error on RLS, normal ready, save into the data buffer. */
			/* Note: read RBR will clear the interrupt */
			uart->buffer[uart->count] = registers->RBR;
			uart->count+=1;
			if ( uart->count == BUFSIZE )
			{
				uart->count = 0;		/* buffer overflow */
				uart->overflow = 1;
			}
			giveSemaphore = 1;
		}
	}
	if ( IIRValue == IIR_RDA )	/* Receive Data Available */
	{
		/* Receive Data Available */
		uart->buffer[uart->count] = registers->RBR;
		uart->count+=1;
		if ( uart->count == BUFSIZE )
		{
			uart->count = 0;		/* buffer overflow */
			uart->overflow = 1;
		}
		giveSemaphore = 1;
	}
	if ( IIRValue == IIR_CTI )	/* Character timeout indicator */
	{
		/* Character Time-out indicator */
		uart->status |= 0x100;		/* Bit 9 as the CTI error */
	}
	if ( IIRValue == IIR_THRE )	/* THRE, transmit holding register empty */
	{
		/* THRE interrupt */
		LSRValue = registers->LSR;		/* Check status in the LSR to see if
										valid data in U0THR or not */
		if ( LSRValue & LSR_THRE )
		{
			uart->empty = 1;
		}
		else
		{
			uart->empty = 0;
		}
	}

	if(giveSemaphore)
	{
		if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
		{
			xSemaphoreGiveFromISR( uart->rxSemaphore, &lHigherPriorityTaskWoken );
		}
		uart->rx = 1;
	}

	if(xTaskGetSchedulerState() != taskSCHEDULER_RUNNING)
	{
		DEBUG_PRINT(DEBUG_ALL,"Scheduler not running:(1)%s\r\n",__func__);
	}

	portEND_SWITCHING_ISR( lHigherPriorityTaskWoken );
}

/*****************************************************************************
** Function name:		UART_0_2_3_IRQHandler
**
** Descriptions:		Generic UART interrupt handler for UART0,2,3
**
** parameters:			struct UART * uart to use
** Returned value:		None
**
*****************************************************************************/
static void UART_0_2_3_IRQHandler (struct UART * uart)
{
	uint8_t IIRValue, LSRValue;
	uint8_t Dummy = Dummy;

	LPC_UART_TypeDef      * registers = (LPC_UART_TypeDef *)uart->registerPointer;
	IIRValue = registers->IIR;
	long lHigherPriorityTaskWoken = pdFALSE;

	uint8_t giveSemaphore = 0;

	IIRValue >>= 1;			/* skip pending bit in IIR */
	IIRValue &= 0x07;			/* check bit 1~3, interrupt identification */
	if ( IIRValue == IIR_RLS )		/* Receive Line Status */
	{
		LSRValue = registers->LSR;
		/* Receive Line Status */
		if ( LSRValue & (LSR_OE|LSR_PE|LSR_FE|LSR_RXFE|LSR_BI) )
		{
			/* There are errors or break interrupt */
			/* Read LSR will clear the interrupt */
			uart->status = LSRValue;
			Dummy = registers->RBR;		/* Dummy read on RX to clear
							interrupt, then bail out */
			return;
		}
		if ( LSRValue & LSR_RDR )	/* Receive Data Ready */
		{
			/* If no error on RLS, normal ready, save into the data buffer. */
			/* Note: read RBR will clear the interrupt */
			uart->buffer[uart->count] = registers->RBR;
			uart->count+=1;
			if ( uart->count == BUFSIZE )
			{
				uart->count = 0;		/* buffer overflow */
				uart->overflow = 1;
			}
			giveSemaphore = 1;
		}
	}
	if ( IIRValue == IIR_RDA )	/* Receive Data Available */
	{
		/* Receive Data Available */
		uart->buffer[uart->count] = registers->RBR;
		uart->count+=1;
		if ( uart->count == BUFSIZE )
		{
			uart->count = 0;		/* buffer overflow */
			uart->overflow = 1;
		}
		giveSemaphore = 1;
	}
	if ( IIRValue == IIR_CTI )	/* Character timeout indicator */
	{
		/* Character Time-out indicator */
		uart->status |= 0x100;		/* Bit 9 as the CTI error */
	}
	if ( IIRValue == IIR_THRE )	/* THRE, transmit holding register empty */
	{
		/* THRE interrupt */
		LSRValue = registers->LSR;		/* Check status in the LSR to see if
										valid data in U0THR or not */
		if ( LSRValue & LSR_THRE )
		{
			uart->empty = 1;
		}
		else
		{
			uart->empty = 0;
		}
	}

	if(giveSemaphore)
	{
		if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
		{
			xSemaphoreGiveFromISR( uart->rxSemaphore, &lHigherPriorityTaskWoken );
		}
		uart->rx = 1;
	}

	if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
	{
		portEND_SWITCHING_ISR( lHigherPriorityTaskWoken );
	}
}

/*****************************************************************************
** Function name:		UART_0_2_3_Send
**
** Descriptions:		Send a block of data to the UART port based
**						on the data length
**
** parameters:			portNum, buffer pointer, and data length
** Returned value:		None
** 
*****************************************************************************/
void UART_0_2_3_Send( struct UART * uart, uint8_t *BufferPtr, uint32_t Length )
{
	LPC_UART_TypeDef      * registers = (LPC_UART_TypeDef *)uart->registerPointer;

	while ( Length != 0 )
	{
		/* THRE status, contain valid data */

		while ( !(uart->empty & 0x01) );

		uart->empty = 0;	/* not empty in the THR until it shifts out */
		int i;
		if(Length > 16)
		{
			i=16;
			Length-=16;
		}
		else
		{
			i=Length;
			Length=0;
		}
		for(;i;i-=1)
		{
			registers->THR = *BufferPtr;
			BufferPtr++;
		}
	}
}

/*****************************************************************************
** Function name:		UART_1_Send
**
** Descriptions:		Send a block of data to the UART port based
**						on the data length
**
** parameters:			portNum, buffer pointer, and data length
** Returned value:		None
** 
*****************************************************************************/
void UART_1_Send( struct UART * uart, uint8_t *BufferPtr, uint32_t Length )
{
	LPC_UART1_TypeDef      * registers = (LPC_UART1_TypeDef *)uart->registerPointer;

	while ( Length != 0 )
	{
		/* THRE status, contain valid data */

		while ( !(uart->empty & 0x01) );
		uart->empty = 0;	/* not empty in the THR until it shifts out */
		registers->THR = *BufferPtr;

		BufferPtr++;
		Length--;
	}
}


static struct UART uart[4] = {
		{ NULL,0,1,0, {0},0,0,LPC_UART0,UART0_IRQn,UART_0_2_3_IRQHandler ,UART_0_Init ,UART_0_2_3_Send},
		{ NULL,0,1,0, {0},0,0,LPC_UART1,UART1_IRQn,UART_1_IRQHandler     ,UART_1_Init ,UART_1_Send    },
		{ NULL,0,1,0, {0},0,0,LPC_UART2,UART2_IRQn,UART_0_2_3_IRQHandler ,UART_2_Init ,UART_0_2_3_Send},
		{ NULL,0,1,0, {0},0,0,LPC_UART3,UART3_IRQn,UART_0_2_3_IRQHandler ,UART_3_Init ,UART_0_2_3_Send},
};


/*****************************************************************************
** Function name:		UARTSend
**
** Descriptions:		Send a block of data to the UART port based
**						on the data length
**
** parameters:			portNum, buffer pointer, and data length
** Returned value:		None
**
*****************************************************************************/
void UARTSend( uint32_t portNum, uint8_t *BufferPtr, uint32_t Length )
{
	uart[portNum].UARTSend(&uart[portNum],BufferPtr, Length);
}

/*****************************************************************************
** Function name:		generic UARTReceive
**
** Descriptions:		Receive a block of data from the UART port based
**
** parameters:			portNum, buffer pointer, data length and timeout_ms
** Returned value:		Number of bytes received, negative on overflow
**
*****************************************************************************/
int UARTReceive( uint32_t portNum, uint8_t *buffer, uint16_t length, timeout_t timeout_ms)
{
	int rv = 0;

	struct UART * usedUart = &uart[portNum];

	signed portBASE_TYPE taken;
	if(uxTaskIsSchedulerRunning())
	{
		taken = xSemaphoreTake( uart->rxSemaphore, timeout_ms);
	}
	else
	{
		// TODO timeout_ms is not honored here ...
		while ( (uart->rx == 0) );
		taken = pdTRUE;
	}

	if (taken == pdTRUE )
	{
		{
			if(usedUart->count > length)
			{
				usedUart->count = length;
				usedUart->overflow = 1;
			}
			memcpy(buffer, (void *)usedUart->buffer, usedUart->count);
			rv = usedUart->count;
			usedUart->count = 0;
			uart->rx = 0;
			if(usedUart->overflow)
			{
				rv = -rv;
				usedUart->overflow = 0;
			}
		}
	}
	return rv;
}

/*****************************************************************************
** Function name:		UARTInit
**
** Descriptions:		Initialize UART port, setup pin select,
**						clock, parity, stop bits, FIFO, etc.
**
** parameters:			portNum and UART baudrate
** Returned value:		true or false, return false only if the 
**						interrupt handler can't be installed to the 
**						VIC table
** 
*****************************************************************************/
uint32_t UARTInit( uint32_t portNum, uint32_t baudrate )
{
	uint32_t rv;

	if(portNum<5)
	{
		rv = TRUE;
		uart[portNum].UARTInit(&uart[portNum], baudrate);
		/* ensure that the first initialisation is the only one; the next
		 * will simply not be executed */
		uart[portNum].UARTInit = UART_Init_done;
	}
	else
	{
		rv = FALSE;
	}

	return( rv );
}

/*****************************************************************************
** Function name:		UART0_IRQHandler
**
** Descriptions:		UART0 interrupt handler
**
** parameters:			None
** Returned value:		None
**
*****************************************************************************/
void UART0_IRQHandler (void)
{
	uart[0].IRQHandler(&uart[0]);
}
/*****************************************************************************
** Function name:		UART1_IRQHandler
**
** Descriptions:		UART1 interrupt handler
**
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void UART1_IRQHandler (void)
{
	uart[1].IRQHandler(&uart[1]);
}
/*****************************************************************************
** Function name:		UART1_IRQHandler
**
** Descriptions:		UART1 interrupt handler
**
** parameters:			None
** Returned value:		None
**
*****************************************************************************/
void UART2_IRQHandler (void)
{
	uart[2].IRQHandler(&uart[2]);
}
/*****************************************************************************
** Function name:		UART0_IRQHandler
**
** Descriptions:		UART0 interrupt handler
**
** parameters:			None
** Returned value:		None
**
*****************************************************************************/
void UART3_IRQHandler (void)
{
	uart[3].IRQHandler(&uart[3]);
}


/******************************************************************************
**                            End Of File
******************************************************************************/
