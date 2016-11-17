/*
 * dali_click.c
 *
 *  Created on: 20.08.2015
 *      Author: Sebastian Sura
 */
/*******************************************************************************
 *
 * dali_master.c
 *
 * DALI forward frame format:
 *
 *  | S |        8 address bits         |        8 command bits         | stop  |
 *  | 1 | 1 | 0 | 0 | 0 | 0 | 0 | 1 | 1 | 0 | 1 | 1 | 1 | 1 | 0 | 0 | 0 |   |   |
 *
 * -+ +-+ +---+ +-+ +-+ +-+ +-+   +-+ +---+   +-+ +-+ +-+ +---+ +-+ +-+ +--------
 *  | | | |   | | | | | | | | |   | | |   |   | | | | | | |   | | | | | |
 *  +-+ +-+   +-+ +-+ +-+ +-+ +---+ +-+   +---+ +-+ +-+ +-+   +-+ +-+ +-+
 *
 *  |2TE|2TE|2TE|2TE|2TE|2TE|2TE|2TE|2TE|2TE|2TE|2TE|2TE|2TE|2TE|2TE|2TE|  4TE  |
 *
 *
 * DALI slave backward frame format:
 *
 *                   | S |         8 data bits           | stop  |
 *                   | 1 | 1 | 0 | 0 | 0 | 0 | 0 | 1 | 1 |   |   |
 *
 *   +---------------+ +-+ +---+ +-+ +-+ +-+ +-+   +-+ +-------------
 *   |               | | | |   | | | | | | | | |   | | |
 *  -+               +-+ +-+   +-+ +-+ +-+ +-+ +---+ +-+
 *
 *   |4 + 7 to 22 TE |2TE|2TE|2TE|2TE|2TE|2TE|2TE|2TE|2TE|  4TE  |
 *
 * 2TE = 834 usec (1200 bps)
 *
 ********************************************************************************
 *  commands supported
 *  ------------------
 *  Type				Range			Repeat		Answer from slave
 *  Power control	0 - 31 			N			N
 *
 *  Configuration	32-129			Y			N
 *  Reserved			130-143			N			N
 *
 *  Query			144-157			N			Y
 *  Reserved			158-159			N			N
 *  Query			160-165			N			Y
 *  Reserved			166-175			N			N
 *  Query			176-197			N			Y
 *  Reserved			198-223			N			N
 *  Query,2xx Std.	224-254			?			?
 *  Query			255				N			Y
 *
 *  Special			256-257			N			N
 *  Special			258-259			Y			N
 *  Special			260-261			N			N
 *  Special			262-263			N			N
 *  Special			264-267			N			N
 *  Special			268-269			N			Y
 *  Special			270				N			N
 *  Reserved			271				N			N
 *  Special			272				N			N
 *******************************************************************************/

#include "dali_click.h"
#include "FreeRTOS.h"
#include "task.h"
#include "debug.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pwm.h"

#define DALI_TASK_PRIORITY			( tskIDLE_PRIORITY + 2 )

/***********************************************************/
/* Configuration flags                                     */
/***********************************************************/

/* in case of inverted RX path define INVERTED_RX */
//#define INVERTED_RX
/***********************************************************/
/* Microcontroller and Board specific defines              */
/***********************************************************/

#define TIMEOUT_INFINITE     0xFFFFFFFF

/* PIO pin P2.6 is used as DALI send (tx) pin */
//#define DALI_SetOutputHigh() { LPC_GPIO2->DATA |=  (1<<6); }
#define DALI_SetOutputHigh() { GPIO_SetValue(TX_PORT, (1<<TX_PIN)); }
//#define DALI_SetOutputLow()  { LPC_GPIO2->DATA &= ~(1<<6); }
#define DALI_SetOutputLow() {GPIO_ClearValue(TX_PORT, (1<<TX_PIN));}
//#define DALI_ConfigOutput()  { LPC_GPIO2->DIR  |=  (1<<6); }
#define DALI_ConfigOutput() {GPIO_SetDir(TX_PORT, (1<<TX_PIN), 1);}

/* PIO pin P1.1 is used as DALI receive (rx) pin */
#ifdef INVERTED_RX
//#define DALI_GetInput(x)     { x = LPC_GPIO1->MASKED_ACCESS[(1<<1)] ? 0 : 1; }
#define DALI_GetInput(x)	{ x = ((GPIO_ReadValue(RX_PORT) >> RX_PIN) & 1) ? 0 : 1; } //32bit wert holen, um TX_PIN nach rechts bitshiften und mit 1 undieren um Wert zu erhalten
#else
#define DALI_GetInput(x)     { x = ((GPIO_ReadValue(RX_PORT) >> RX_PIN) & 1) ? 1 : 0; } //32bit wert holen, um TX_PIN nach rechts bitshiften und mit 1 undieren um Wert zu erhalten
#endif

/* For receive, this module uses TMR32B0-CAP0 input (capture and interrupt on both edges) */
/* TMR32B0-CAP0 input (P1.5) is connected to P1.1 (to check high / low level by software) */
/* So set P1.5 as CT32B0.CAP0 (= DALI receive pin). Bit 7:6 (reserved) are also set to 1  */
//#define DALI_ConfigInput()   { LPC_IOCON->PIO1_5 = 0xD2; } ???
#define DALI_ConfigInput() {GPIO_SetDir(RX_PORT, (1<<RX_PIN),0);}

/* TMR32B0 is used for DALI timing and capturing of DALI input */
//#define TIMER_IRQ            TIMER_32_0_IRQn
//#define TIMER_IRQ		TIMER0_IRQn
//#define GET_TIMER_REG_CR0(x) { x = LPC_TMR32B0->CR0; }
//#define GET_TIMER_REG_IR(x)  { x = LPC_TMR32B0->IR;  }
//#define SET_TIMER_REG_IR(x)  { LPC_TMR32B0->IR  = x; }
//#define SET_TIMER_REG_PR(x)  { LPC_TMR32B0->PR  = x; }
//#define SET_TIMER_REG_TC(x)  { LPC_TMR32B0->TC  = x; }
//#define SET_TIMER_REG_CCR(x) { LPC_TMR32B0->CCR = x; }
//#define SET_TIMER_REG_TCR(x) { LPC_TMR32B0->TCR = x; }
//#define SET_TIMER_REG_MCR(x) { LPC_TMR32B0->MCR = x; }
//#define SET_TIMER_REG_MR0(x) { LPC_TMR32B0->MR0 = x; }
//#define GET_TIMER_REG_CR0(x) { x = LPC_TIM0->CR0; }
//#define GET_TIMER_REG_IR(x)  { x = LPC_TIM0->IR;  }
//#define SET_TIMER_REG_IR(x)  { LPC_TIM0->IR  = x; }
//#define SET_TIMER_REG_PR(x)  { LPC_TIM0->PR  = x; }
//#define SET_TIMER_REG_TC(x)  { LPC_TIM0->TC  = x; }
//#define SET_TIMER_REG_CCR(x) { LPC_TIM0->CCR = x; }
//#define SET_TIMER_REG_TCR(x) { LPC_TIM0->TCR = x; }
//#define SET_TIMER_REG_MCR(x) { LPC_TIM0->MCR = x; }
//#define SET_TIMER_REG_MR0(x) { LPC_TIM0->MR0 = x; }
/*PWM TIMER*/

#define TIMER_IRQ PWM1_IRQn
#define GET_TIMER_REG_CR0(x) { x = LPC_PWM1->CR0; }
#define GET_TIMER_REG_IR(x)  { x = LPC_PWM1->IR;  }
#define SET_TIMER_REG_IR(x)  { LPC_PWM1->IR  = x; }
#define SET_TIMER_REG_PR(x)  { LPC_PWM1->PR  = x; }
#define SET_TIMER_REG_TC(x)  { LPC_PWM1->TC  = x; }
#define SET_TIMER_REG_CCR(x) { LPC_PWM1->CCR = x; }
#define SET_TIMER_REG_TCR(x) { LPC_PWM1->TCR = x; }
#define SET_TIMER_REG_MCR(x) { LPC_PWM1->MCR = x; }
#define SET_TIMER_REG_MR0(x) { LPC_PWM1->MR0 = x; }

/***********************************************************/
/* Type definitions and defines                            */
/***********************************************************/

#define MAX_BF_EDGES      18     // max 18 edges per backward frame

/* protocol timing definitions */
//Anpassen ???
#define TE          (417)                   // half bit time = 417 usec
#if 0 /* strict receive timing according to specification (+/- 10%) */
#define MIN_TE      (TE     - (TE/10))      // minimum half bit time
#define MAX_TE      (TE     + (TE/10))      // maximum half bit time
#define MIN_2TE     ((2*TE) - ((2*TE)/10))  // minimum full bit time
#define MAX_2TE     ((2*TE) + ((2*TE)/10))  // maximum full bit time
#else /* More relaxed receive timing (+/- 25%) */
#define MIN_TE      (TE     - (TE/3)) 		// minimum half bit time
#define MAX_TE      (TE     + (TE/3))  		// maximum half bit time
#define MIN_2TE     ((2*TE) - ((2*TE)/3))   // minimum full bit time
#define MAX_2TE     ((2*TE) + ((2*TE)/3))   // maximum full bit time
#endif

// Configuration commands
#define CMD32                (0x0120)
#define CMD129               (0x0181)

// Query commands
#define CMD144               (0x0190)
#define CMD157               (0x019D)
#define CMD160               (0x01A0)
#define CMD165               (0x01A5)
#define CMD176               (0x01B0)
#define CMD197               (0x01C5)
#define CMD255               (0x01FF)

/*DALI COMMANDS*/
#define TERMINATE             0xA100 //256
#define INITIALISE            0xA500 //258 - starting initialization mode
#define RANDOMISE             0xA700 //259 - generating a random address
#define COMPARE               0xA900 //260
#define WITHDRAW              0xAB00 //261
#define SEARCHADDRH           0xB100 //264
#define SEARCHADDRM           0xB300 //265
#define SEARCHADDRL           0xB500 //266
#define PROGRAM_SHORT_ADDRESS 0xB701 //267
#define VERIFY_SHORT_ADDRESS  0xB901
#define QUERY_SHORT_ADDRESS   0xBB00
#define STOREDTR              0xFF80 //128

typedef enum daliMsgTypeTag {
	DALI_MSG_UNDETERMINED = 0,
	DALI_MSG_SHORT_ADDRESS = 1,
	DALI_MSG_GROUP_ADDRESS = 2,
	DALI_MSG_BROADCAST = 4,
	DALI_MSG_SPECIAL_COMMAND = 8
} daliMsgType_t;

typedef enum answerTypeTag {
	ANSWER_NOT_AVAILABLE = 0,
	ANSWER_NOTHING_RECEIVED,
	ANSWER_GOT_DATA,
	ANSWER_INVALID_DATA,
	ANSWER_TOO_EARLY
} answer_t;

/* state machine related definitions */
typedef enum stateTag {
	MS_IDLE = 0,                        // bus idle
	MS_TX_SECOND_HALF_START_BIT,        //
	MS_TX_DALI_FORWARD_FRAME,           // sending the dali forward frame
	MS_TX_STOP_BITS,                    //
	MS_SETTLING_BEFORE_BACKWARD, // settling between forward and backward - stop bits
	MS_SETTLING_BEFORE_IDLE, // settling before going to idle, after forward frame
	MS_WAITING_FOR_SLAVE_START_WINDOW, // waiting for 7Te, start of slave Tx window
	MS_WAITING_FOR_SLAVE_START,         // start of slave Tx window
	MS_RECEIVING_ANSWER                 // receiving slave message
} MASTER_STATE;

/* definition of the captured edge data */
typedef struct capturedDataType_tag {
	uint16_t capturedTime;             // time stamp of signal edge
	uint8_t bitLevel;                 // bit level *after* the edge
	uint8_t levelType;  // indication of long or short duration *after* the edge
} capturedDataType;

typedef struct capturedFrameType_tag {
	capturedDataType capturedData[MAX_BF_EDGES];
	uint8_t capturedItems;    // counter of the captured edges
} capturedFrameType;

/***********************************************************/
/* Global variables                                        */
/***********************************************************/

static volatile bool usbForwardFrameReceived;
static volatile uint16_t usbForwardFrame;
static volatile uint8_t BackwardFrame; // DALI slave answer
static volatile answer_t BackwardStatus;
static volatile MASTER_STATE masterState;
static volatile bool waitForAnswer;
static volatile bool earlyAnswer;
static volatile uint32_t daliForwardFrame; // converted DALI master command
static volatile capturedFrameType capturedFrame; // data structure for the capture
uint16_t b1 = 0, b3 = 0, b5 = 0;
static short slavesno;
int daliport = 1;
int TX_PIN = 26;
int RX_PIN = 3;
int BUTTON_PIN = 2;
xTaskHandle xHandle_dali = NULL;
bool dali_isactive = false;

/***********************************************************/
/* Local functions                                         */
/***********************************************************/

static inline bool DALI_CheckLogicalError(void) {
	uint8_t bitLevel;
	uint16_t receivedFrame;
	uint32_t bitStream, i, item, pattern, bitPair;

	// build frame from captured bit levels in bitStream
	bitStream = 0;
	for (i = 0, item = 0;
			((i < MAX_BF_EDGES) && (item < capturedFrame.capturedItems));
			item++) {
		bitLevel = capturedFrame.capturedData[item].bitLevel;
		bitStream |= (bitLevel << ((MAX_BF_EDGES - 1) - i));
		i++;
		// shift another bit in case of long symbol
		if (capturedFrame.capturedData[item].levelType == 'l') {
			bitStream |= (bitLevel << ((MAX_BF_EDGES - 1) - i));
			i++;
		}
	}
	// check if there are 3 zeros or 3 ones in a row
	for (i = 0; i < (MAX_BF_EDGES - 2); i++) {
		pattern = 7 << i;
		if (((bitStream & pattern) == 0)
				|| ((bitStream & pattern) == pattern)) {
			return true; // error, invalid data, so return immediately
		}
	}
	// compose answer byte in receivedFrame
	receivedFrame = 0;
	for (i = 0; i < MAX_BF_EDGES; i += 2) {
		receivedFrame <<= 1;
		bitPair = (bitStream >> ((MAX_BF_EDGES - 2) - i)) & 3;
		if ((bitPair == 0) || bitPair == 3) {
			return true; // error '00' or '11' is not a valid bit
		}
		if (bitPair == 1)
			receivedFrame |= 1;
	}
	// need to have the start bit in position 9 for a valid frame
	if (!(receivedFrame & 0x100))
		return true;
	// cast out the start bit for the answer byte
	BackwardFrame = (uint8_t) receivedFrame;
	return false;
}

static inline bool DALI_CheckTimingError(void) {
	uint32_t i, capT1, capT2, interval;

	for (i = 0; i < (capturedFrame.capturedItems - 1); i++) {
		capT1 = capturedFrame.capturedData[i].capturedTime;
		capT2 = capturedFrame.capturedData[i + 1].capturedTime;
		interval = capT2 - capT1;
		if ((interval >= MIN_TE) && (interval <= MAX_TE)) {
			capturedFrame.capturedData[i].levelType = 's';
		} else if ((interval >= MIN_2TE) && (interval <= MAX_2TE)) {
			capturedFrame.capturedData[i].levelType = 'l';
		} else {
			return true; // timing error, so stop check immediately
		}
	}
	capturedFrame.capturedData[i].levelType = 'x'; // terminate the frame
	return false;
}

static inline bool DALI_Decode(void) {
	if (DALI_CheckTimingError())
		return false;
	if (DALI_CheckLogicalError())
		return false;
	return true;
}

static inline uint32_t DALI_ConvertForwardFrame(uint16_t forwardFrame) {
	uint32_t convertedForwardFrame = 0;
	int8_t i;

	for (i = 15; i >= 0; i--) {
		if (forwardFrame & (1 << i)) {   // shift in bits values '0' and '1'
			convertedForwardFrame <<= 1;
			convertedForwardFrame <<= 1;
			convertedForwardFrame |= 1;
		} else {   // shift in bits values '1' and '0'
			convertedForwardFrame <<= 1;
			convertedForwardFrame |= 1;
			convertedForwardFrame <<= 1;
		}
	}
	return convertedForwardFrame;
}

static inline daliMsgType_t DALI_CheckMsgType(uint16_t forwardFrame) {
	daliMsgType_t type = DALI_MSG_UNDETERMINED;

	if ((forwardFrame & 0x8000) == 0) {
		type = DALI_MSG_SHORT_ADDRESS;
	} else if ((forwardFrame & 0xE000) == 0x8000) {
		type = DALI_MSG_GROUP_ADDRESS;
	} else if ((forwardFrame & 0xFE00) == 0xFE00) {
		type = DALI_MSG_BROADCAST;
	} else if (((forwardFrame & 0xFF00) >= 0xA000)
			&& ((forwardFrame & 0xFF00) <= 0xFD00)) {
		type = DALI_MSG_SPECIAL_COMMAND;
	}
	return type;
}

static inline bool DALI_CheckWaitForAnswer(uint16_t forwardFrame,
		daliMsgType_t type) {
	bool waitFlag = false;

	if (type == DALI_MSG_SPECIAL_COMMAND) {
		// Special commands
		if ((forwardFrame == COMPARE)
				|| ((forwardFrame & 0xFF81) == VERIFY_SHORT_ADDRESS)
				|| (forwardFrame == QUERY_SHORT_ADDRESS)) {
			waitFlag = true;
		}
	} else {
		// Query commands
		if ((((forwardFrame & 0x01FF) >= CMD144)
				&& ((forwardFrame & 0x01FF) <= CMD157))
				|| (((forwardFrame & 0x01FF) >= CMD160)
						&& ((forwardFrame & 0x01FF) <= CMD165))
				|| (((forwardFrame & 0x01FF) >= CMD176)
						&& ((forwardFrame & 0x01FF) <= CMD197))
				|| ((forwardFrame & 0x01FF) == CMD255)) {
			waitFlag = true;
		}
	}
	return waitFlag;
}

static inline bool DALI_CheckRepeatCmd(uint16_t forwardFrame,
		daliMsgType_t type) {
	bool repeatCmd = false;

	if (type == DALI_MSG_SPECIAL_COMMAND) {
		// Special commands 'initialize' and 'randomize' shall be repeated within 100 ms
		if (((forwardFrame & 0xFF00) == INITIALISE)
				|| (forwardFrame == RANDOMISE)) {
			repeatCmd = true;
		}
	} else {
		// Configuration commands (32 - 129) shall all be repeated within 100 ms
		if (((forwardFrame & 0x01FF) >= CMD32)
				&& ((forwardFrame & 0x01FF) <= CMD129)) {
			repeatCmd = true;
		}
	}
	return repeatCmd;
}

static inline void DALI_DoTransmission(uint32_t convertedForwardFrame,
bool waitFlag) {
	//bsp_set_led(LED_RTX_DALI_BUS, 0); // LED OFF MEANS TX TO DALI BUS
	// Claim the bus and setup global variables
	masterState = MS_TX_SECOND_HALF_START_BIT;
	waitForAnswer = waitFlag;
	daliForwardFrame = convertedForwardFrame;
	DALI_SetOutputLow()
	;
	// Activate the timer module to output the forward frame
	SET_TIMER_REG_TC(0);       // clear timer
	SET_TIMER_REG_MR0(TE);     // ~ 2400 Hz (half bit time)
	SET_TIMER_REG_CCR(0);      // disable capture
	SET_TIMER_REG_MCR(3);      // interrupt on MR0, reset timer on match 0
	SET_TIMER_REG_TCR(1);      // enable the timer
	while (masterState != MS_IDLE) {
		// wait till transmission is completed
		// __WFI();
	}
	if (waitForAnswer) {
		if (capturedFrame.capturedItems == 0) {
			BackwardStatus = ANSWER_NOTHING_RECEIVED;
		} else if (earlyAnswer) {
			BackwardStatus = ANSWER_TOO_EARLY;
		} else {
			if (DALI_Decode()) {
				BackwardStatus = ANSWER_GOT_DATA;
			} else {
				BackwardStatus = ANSWER_INVALID_DATA;
			}
		}
//		while (BackwardStatus != ANSWER_NOT_AVAILABLE) {
//			// wait till answer is send to USB host (PC)
//			// __WFI();
//		}
	}
}

void MQTTtoDALI(const char* value) {
	uint16_t forwardFrame;
	char array[5];
	int temp;

	array[0] = value[0];
	array[1] = value[1];
	array[2] = value[2];
	array[3] = value[3];
	array[4] = '\0';
	temp = hexToInt(array);
	DALI_Send((uint16_t)temp);
}

void DALI_Send(uint16_t forwardFrame) {
	uint8_t i = 0;
	uint8_t n = 1;
	uint32_t convertedForwardFrame = DALI_ConvertForwardFrame(forwardFrame);
	daliMsgType_t daliMsgType = DALI_CheckMsgType(forwardFrame);
	bool waitFlag = DALI_CheckWaitForAnswer(forwardFrame, daliMsgType);

	if (DALI_CheckRepeatCmd(forwardFrame, daliMsgType))
		n = 2;
	while (i < n) {
		DALI_DoTransmission(convertedForwardFrame, waitFlag);
		i++;
	}
}

static inline void DALI_Init(void) {
	// First init ALL the global variables
	uint32_t clock;
	usbForwardFrameReceived = false;
	usbForwardFrame = 0;
	BackwardFrame = 0;
	BackwardStatus = ANSWER_NOT_AVAILABLE;
	masterState = MS_IDLE;
	waitForAnswer = false;
	earlyAnswer = false;
	daliForwardFrame = 0;
	capturedFrame.capturedItems = 0;

	// Initialize the physical layer of the dali master
	DALI_SetOutputHigh()
	DALI_ConfigOutput()
	DALI_ConfigInput()
		//SET_TIMER_REG_PR((SystemCoreClock / 1000000) - 1); // timer runs at 1 MHz - 1usec per tick
		//SET_TIMER_REG_PR(99);
	NVIC_ClearPendingIRQ(PWM1_IRQn);
	NVIC_EnableIRQ(PWM1_IRQn);
}

//static void DALI_SetOutFrame(uint16_t frame) {
//	usbForwardFrame = frame;
//	usbForwardFrameReceived = true;
//
//}
//
//static int DALI_GetAnswer() {
//	return 0;
//}

/***********************************************************/
/* Exported Counter/Timer IRQ handler                      */
/***********************************************************/

/* the handling of the protocol is done in the IRQ */
void PWM1_IRQHandler(void) {
	static uint8_t bitcount;
	uint8_t irq_stat;
	uint16_t capture;

	GET_TIMER_REG_CR0(capture);
	GET_TIMER_REG_IR(irq_stat);
	if (irq_stat & 1) {   // match 0 interrupt
		SET_TIMER_REG_IR(1);   // clear MR0 interrupt flag
		if (masterState == MS_TX_SECOND_HALF_START_BIT) {
			DALI_SetOutputHigh()
			;
			bitcount = 0;
			masterState = MS_TX_DALI_FORWARD_FRAME;
		} else if (masterState == MS_TX_DALI_FORWARD_FRAME) {
			if (daliForwardFrame & 0x80000000) {
				DALI_SetOutputHigh()
				;
			} else {
				DALI_SetOutputLow()
				;
			}
			daliForwardFrame <<= 1;
			bitcount++;
			if (bitcount == 32)
				masterState = MS_TX_STOP_BITS;
		} else if (masterState == MS_TX_STOP_BITS) {
			DALI_SetOutputHigh()
			;
			// the first half of the first stop bit has just been output.
			// do we have to wait for an answer?
			if (waitForAnswer) { // elapse until the end of the last half of the second stop bit
				SET_TIMER_REG_MR0(4*TE);
				BackwardFrame = 0;
				earlyAnswer = false;
				capturedFrame.capturedItems = 0;
				masterState = MS_SETTLING_BEFORE_BACKWARD;

			} else { // no answer from slave expected, need to wait for the remaining
					 // bus idle time before next forward frame
					 // add additional 3 TE to minimum specification to be not at the edge of the timing specification
				SET_TIMER_REG_MR0((4*TE) + (22*TE) + (3*TE));
				masterState = MS_SETTLING_BEFORE_IDLE;
			}
		} else if (masterState == MS_SETTLING_BEFORE_BACKWARD) {
			//bsp_set_led(LED_RTX_DALI_BUS, 1); // LED ON MEANS RX FROM DALI BUS
			// setup the first window limit for the slave answer
			// slave should not respond before 7TE
			SET_TIMER_REG_MR0(7*MIN_TE);
			SET_TIMER_REG_CCR(7);   // enable receive, capture on both edges
			masterState = MS_WAITING_FOR_SLAVE_START_WINDOW;

		} else if (masterState == MS_WAITING_FOR_SLAVE_START_WINDOW) { // setup the second window limit for the slave answer,
																	   // slave must start transmit within the next 23TE window
			SET_TIMER_REG_MR0(23*MAX_TE);
			masterState = MS_WAITING_FOR_SLAVE_START;
		} else if (masterState == MS_WAITING_FOR_SLAVE_START) { // if we still get here, got 'no' or too early answer from slave
																// idle time of 23TE was already elapsed while waiting, so
																// immediately release the bus
			SET_TIMER_REG_TCR(2);   // reset and stop the timer
			SET_TIMER_REG_CCR(0);   // disable capture
			SET_TIMER_REG_IR(0x10); // clear possible capture interrupt flag
			masterState = MS_IDLE;
		} else if (masterState == MS_RECEIVING_ANSWER) {   // stop receiving
														   // now idle the bus between backward and next forward frame
														   // since we don't track the last edge of received frame,
														   // conservatively we wait for 23 TE (>= 22 TE as for specification)
														   // Receive interval considered anyway the max tolerance for
														   // backward frame duration so >22TE should already be asserted
			SET_TIMER_REG_MR0(23*TE);
			SET_TIMER_REG_CCR(0);   // disable capture
			SET_TIMER_REG_IR(0x10); // clear possible capture interrupt flag
			masterState = MS_SETTLING_BEFORE_IDLE;
		} else if (masterState == MS_SETTLING_BEFORE_IDLE) {
			//bsp_set_led(LED_RTX_DALI_BUS, 1); // LED ON MEANS RX FROM DALI BUS
			SET_TIMER_REG_TCR(2);   // reset and stop the timer
			masterState = MS_IDLE;
		}
	} else if (irq_stat & 0x10) {   // capture interrupt
		SET_TIMER_REG_IR(0x10);     // clear capture interrupt flag
		if (masterState == MS_WAITING_FOR_SLAVE_START_WINDOW) { // slave should not answer yet, it came too early!!!!
			SET_TIMER_REG_CCR(0);   // disable capture
			earlyAnswer = true;
		} else if (masterState == MS_WAITING_FOR_SLAVE_START) { // we got an edge, so the slave is transmitting now
																// allowed remaining answering time is 22TE
			SET_TIMER_REG_MR0(22*MAX_TE);
			SET_TIMER_REG_TC(0);
			SET_TIMER_REG_IR(1);    // clear possible MR0 interrupt flag
			// first pulse is begin of the start bit
			DALI_GetInput(capturedFrame.capturedData[0].bitLevel);
			capturedFrame.capturedData[0].capturedTime = 0;
			capturedFrame.capturedItems = 1;
			masterState = MS_RECEIVING_ANSWER;
		} else if (masterState == MS_RECEIVING_ANSWER) { // this part just captures the frame data, evaluation is done
														 // at the end of max backward frame duration
			if (capturedFrame.capturedItems < MAX_BF_EDGES) {
				uint32_t index = capturedFrame.capturedItems;
				DALI_GetInput(capturedFrame.capturedData[index].bitLevel);
				if (capturedFrame.capturedData[index].bitLevel == 1) {
					capture -= 120;
				}
				capturedFrame.capturedData[index].capturedTime = capture;
				capturedFrame.capturedItems++;
			}
		}
	}
}

/********************************************************************//**
 * @brief		seperates searchaddress into High, Middle and Low Addressbit of the 24bit Searchaddress
 * @param[in]	24bit searchaddress, which is send to the slaves
 * @return 		None
 *********************************************************************/
void sendsearchaddress(uint32_t searchaddress) {
	uint16_t b1_temp, b3_temp, b5_temp;
	uint32_t searchaddress_temp = searchaddress;
	b1_temp = (uint8_t) (searchaddress >> 16);
	b1_temp |= 0xb100;
	b3_temp = (uint8_t) (searchaddress >> 8);
	b3_temp |= 0xb300;
	b5_temp = (uint16_t) searchaddress;
	b5_temp &= 0xff;
	b5_temp |= 0xb500;
	/*Send only changed bytes*/
	if (b1_temp != b1)
		DALI_Send(b1_temp);
	if (b3_temp != b3)
		DALI_Send(b3_temp);
	if (b5_temp != b5)
		DALI_Send(b5_temp);
}
/********************************************************************//**
 * @brief		set address as the slaves shortaddress, which responded to the COMPARE command during the initialization process
 * @param[in]	8bit short address
 * @return 		None
 *********************************************************************/
void assignshortaddress(uint8_t address) {
	uint16_t shortaddress = 0xb701;
	shortaddress |= (address << 1);
	DALI_Send(shortaddress);
}

/********************************************************************//**
 * @brief		sends 'address' as shortaddress to every slave on the bus, only use it when there is only one slave
 * @param[in]	8bit short address
 * @return 		None
 *********************************************************************/
void DALI_Set_Address(uint8_t address) {
	uint16_t dtr = 0xa301;
	DALI_Send(TERMINATE);
	dtr |= (address << 1);
	DALI_Send(dtr);
	DALI_Send(STOREDTR);
}

void senddtr(uint8_t data) {
	uint16_t dtr = 0xA300;
	dtr |= data;
	DALI_Send(dtr);
}

void storefaderate(uint8_t address, uint8_t data) {
	uint16_t fr = 0x12F | (address << 9);
	senddtr(data);
	DALI_Send(fr);
}

void storefadetime(uint8_t address, uint8_t data) {
	uint16_t fr = 0x12E | (address << 9);
	senddtr(data);
	DALI_Send(fr);
}

void flash(uint8_t address) {
	uint16_t max = 0x105 | (address << 9);
	uint16_t min = 0x106 | (address << 9);
	DALI_Send(max);
	vTaskDelay(700);
	DALI_Send(min);
	vTaskDelay(700);
}

/********************************************************************//**
 * @brief		Auto-Addressing: automatically find an assign addresses to all slaves on the bus starting with 1
 * @param[in]	None
 * @return 		None
 *********************************************************************/
void DALI_Find_Slaves() {
	uint32_t mid = 0xFFFFFE;
	uint32_t max = 0xFFFFFD;
	uint32_t min = 0;
	uint16_t verify = 0xb901;
	uint8_t address = 0x1;
	slavesno = 0;
	int i;
	/*ALL OFF*/
	DALI_Send(0xFF00);
	/*If there are any unterminated INITIALISE processes on the bus, terminate them*/
	DALI_Send(TERMINATE);
	/*1. Send '258' INITIALISE and wait for 15 Minutes*/
	DALI_Send(INITIALISE);
	/*2. Send 259 RANDOMISE, all Slaves gen. random 24bit number*/
	DALI_Send(RANDOMISE);
	/*3. search slaves binary*/
	sendsearchaddress(mid);
	DALI_Send(COMPARE);
	/*if there are any slaves in the network, they should have responded now*/
	while (getBackwardFrame() == 0xFF) {
		/*We have Slaves on the BUS, so look for the lowest*/
		while (max >= min) {
			mid = (min + max) / 2;
			sendsearchaddress(mid);
			DALI_Send(COMPARE);
			if (getBackwardFrame() == 0xFF) {
				max = mid;
			} else {
				min = mid;
				if ((mid + 1) == max)
					break;
			}

		}
		/*Here we have our first SLAVE*/
		sendsearchaddress(max);
		DALI_Send(COMPARE);

		/*4. Program short adress to slave*/
		assignshortaddress(address);
		/*5. Verify short adress*/
		verify |= (address << 1);
		DALI_Send(verify);
		/*6. WITHDRAW slaves */
		DALI_Send(WITHDRAW);

		/*Found Slave flashes*/
		for (i = 0; i < 4; i++) {
			flash(address);
		}
		address += 1;
		slavesno++;
		/*NEW ROUND*/
		/*Reset max to find the next higher slave*/
		max = 0xFFFFFE;
		mid = 0xFFFFFE;
		sendsearchaddress(mid);
		DALI_Send(COMPARE);
	}
	if (slavesno) {
		printToUart("FOUND %i Slaves, addressed from 1 to %i", slavesno,
				slavesno);
	} else {
		printToUart("NO SLAVES FOUND");
	}
	/*8. stop process with 256 TERMINATE*/
	DALI_Send(TERMINATE);
	/*ALL OFF*/
	DALI_Send(0xFF00);
}

bool getwaitForAnswer() {
	if (waitForAnswer == true)
		return 1;
	return 0;
}
/*actual DATA received in uint8_t*/
int getBackwardFrame() {
	return BackwardFrame;
}

/*answer from slave in statuscode*/
int getBackwardStatus() {
	return BackwardStatus;
}

int getmasterState() {
	return masterState;
}

void setmasterState(int s) {
	masterState = s;
}

void setbackwardframe(capturedFrameType f) {
	capturedFrame = f;
}

short getSlaves() {
	return slavesno;
}

bool getdali_isactive() {
	return dali_isactive;
}

/*DALI config Commands*/

void pulse(uint16_t address, int time, int wait) {
	uint16_t on = 0xfe | (address << 9);
	uint16_t off = 0x00 | (address << 9);
	DALI_Send(on);
	vTaskDelay(time);
	DALI_Send(off);
	if (wait)
		vTaskDelay(time);

}

void pulseloop(uint16_t address, int time, int quantity, int wait) {
	int i;
	uint16_t on = 0x1fe | (address << 9);
	uint16_t off = 0x100 | (address << 9);
	for (i = 0; i < quantity; i++) {
		DALI_Send(on);
		vTaskDelay(time);
		DALI_Send(off);
		if (wait)
			vTaskDelay(time);
	}
}

void demo1() {
	pulse(1, 707, 0);
	pulse(2, 707, 0);
	pulse(1, 707, 0);
	pulse(2, 707, 0);
}

void demo2() {

	pulse(1, 1000, 1);
	pulse(2, 1000, 1);
	pulse(1, 1000, 1);
	pulse(2, 1000, 1);

}

void demo3() {

	DALI_Send(0x02fe);
	DALI_Send(0x04fe);
	vTaskDelay(707);
	DALI_Send(0x0200);
	DALI_Send(0x0400);
	vTaskDelay(707);
	DALI_Send(0x02fe);
	DALI_Send(0x04fe);
	vTaskDelay(707);
	DALI_Send(0x0200);
	DALI_Send(0x0400);
	vTaskDelay(707);
	DALI_Send(0x02fe);
	DALI_Send(0x04fe);
	vTaskDelay(707);
	DALI_Send(0x0200);
	DALI_Send(0x0400);
	vTaskDelay(707);
	DALI_Send(0x02fe);
	DALI_Send(0x04fe);
	vTaskDelay(707);
	DALI_Send(0x0200);
	DALI_Send(0x0400);
	vTaskDelay(707);
	DALI_Send(0x02fe);
	DALI_Send(0x04fe);
	vTaskDelay(707);
	DALI_Send(0x0200);
	DALI_Send(0x0400);

}

void demo4() {
	int i = 500;
	DALI_Send(0x0305);
	vTaskDelay(i);
	DALI_Send(0x0300);
	//vTaskDelay(i);
	DALI_Send(0x0305);
	vTaskDelay(i);
	DALI_Send(0x0300);
	//vTaskDelay(i);
	DALI_Send(0x0305);
	vTaskDelay(i);
	DALI_Send(0x0300);
	//vTaskDelay(i);
	DALI_Send(0x0305);
	vTaskDelay(i);
	DALI_Send(0x0300);
	//vTaskDelay(i);
	DALI_Send(0x0305);
	vTaskDelay(i);
	DALI_Send(0x0300);
	//vTaskDelay(i);
	DALI_Send(0x0505);
	vTaskDelay(i);
	DALI_Send(0x0500);
	//vTaskDelay(i);
	DALI_Send(0x0505);
	vTaskDelay(i);
	DALI_Send(0x0500);
	//vTaskDelay(i);
	DALI_Send(0x0505);
	vTaskDelay(i);
	DALI_Send(0x0500);
	//vTaskDelay(i);
	DALI_Send(0x0505);
	vTaskDelay(i);
	DALI_Send(0x0500);
	//vTaskDelay(i);
	DALI_Send(0x0505);
	vTaskDelay(i);
	DALI_Send(0x0500);
	//vTaskDelay(i);

}

void setup_dali() {

	if (daliport == 1) {
		TX_PIN = 26;
		BUTTON_PIN = 2;
		RX_PIN = 3;

	} else if (daliport == 2) {

		TX_PIN = 28;
		BUTTON_PIN = 7;
		RX_PIN = 6;

	}

	PINSEL_CFG_Type PinCfg;
	PinCfg.Funcnum = 0;
	PinCfg.Portnum = TX_PORT;
	PinCfg.Pinnum = TX_PIN;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Portnum = BUTTON_PORT;
	PinCfg.Pinnum = BUTTON_PIN;
	PINSEL_ConfigPin(&PinCfg);
	/*Assign RX_Port to PCAP1*/
	PinCfg.Funcnum = 1;
	PinCfg.Portnum = RX_PORT;
	PinCfg.Pinnum = RX_PIN;
	PINSEL_ConfigPin(&PinCfg);
	/*P2.2 as Input Button*/
	/*Initialize GPIO Function of PINs*/
	GPIO_SetDir(TX_PORT, (1 << TX_PIN), 1);
	/*Button*/
	GPIO_SetDir(BUTTON_PORT, (1 << BUTTON_PIN), 0);
	/*Prepare RX pin for reading*/
	FIO_SetMask(RX_PORT, 1 << RX_PIN, 0);
	/*Initialize PWM Block*/
	PWM_TIMERCFG_Type PWMCfgDat;
	PWMCfgDat.PrescaleOption = PWM_TIMER_PRESCALE_USVAL;
	PWMCfgDat.PrescaleValue = 1;
	PWM_Init(LPC_PWM1, PWM_MODE_TIMER, (void *) &PWMCfgDat);
	PWM_Cmd(LPC_PWM1, ENABLE);

}

static void Dali_Task(void *pvParameters) {

	printToUart("Dali_Task running..\r\n");

	setup_dali();
	DALI_Init();

	for (;;) {

	    vTaskDelay(1000);

		//Only use for Demo purposes, Slaves with address 1 and 2
//		DALI_Send(0x02fe);
//		vTaskDelay(4000);
//		DALI_Send(0x04fe);
//		vTaskDelay(4000);
//		DALI_Send(0x0200); //OFF
//		vTaskDelay(4000);
//		DALI_Send(0x0400); //OFF
//		vTaskDelay(4000);
		//demo1();
		//demo2();
		//demo3();
		//demo4();
		//flash(1);
	}
}

void init_dali(int i) {

	dali_isactive = true;
	daliport = i;

	xTaskCreate(Dali_Task, (signed char * )"DALI", 512, NULL,
			DALI_TASK_PRIORITY, &xHandle_dali);
}

void deinit_dali() {

	dali_isactive = false;
	NVIC_ClearPendingIRQ(PWM1_IRQn);
	PWM_Cmd(LPC_PWM1, DISABLE);
	PWM_DeInit(LPC_PWM1);
	vTaskDelete(xHandle_dali);
}
