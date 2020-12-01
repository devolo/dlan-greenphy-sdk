/*
 * Copyright (c) 2017, devolo AG, Aachen, Germany.
 * All rights reserved.
 *
 * This Software is part of the devolo GreenPHY-SDK.
 *
 * Usage in source form and redistribution in binary form, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Usage in source form is subject to a current end user license agreement
 *    with the devolo AG.
 * 2. Neither the name of the devolo AG nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 3. Redistribution in binary form is limited to the usage on the GreenPHY
 *    module of the devolo AG.
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

#ifndef INC_EXPAND2CLICK_H_
#define INC_EXPAND2CLICK_H_

//#define EXPAND_TEST

#define BANK 0
#define AddressCode (0b0010 << 4)
#define DefaultAddress (0b000 << 1)
#define OpCodeRead  0x01
#define OpCodeWrite 0xFE

#define SeqModeBank0 0x20
#define SeqModeBank1 0xA0
#define ToggleMode   0x5F
#define PollMode     0x80

  // BANK 1 register configuration
#define IODIRA_BANK1   0x00
#define IPOLA_BANK1    0x01
#define GPINTENA_BANK1 0x02
#define DEFVALA_BANK1  0x03
#define INTCONA_BANK1  0x04
#define IOCON_BANK1    0x05
#define GPPUA_BANK1    0x06
#define INTFA_BANK1    0x07
#define INTCAPA_BANK1  0x08
#define GPIOA_BANK1    0x09
#define OLATA_BANK1    0x0A
#define IODIRB_BANK1   0x10
#define IPOLB_BANK1    0x11
#define GPINTENB_BANK1 0x12
#define DEFVALB_BANK1  0x13
#define INTCONB_BANK1  0x14
//#define IOCON_BANK1    0x15
#define GPPUB_BANK1    0x16
#define INTFB_BANK1    0x17
#define INTCAPB_BANK1  0x18
#define GPIOB_BANK1    0x19
#define OLATB_BANK1    0x1A

  // BANK 0 register configuration
#define IODIRA_BANK0   0x00
#define IODIRB_BANK0   0x01
#define IPOLA_BANK0    0x02
#define IPOLB_BANK0    0x03
#define GPINTENA_BANK0 0x04
#define GPINTENB_BANK0 0x05
#define DEFVALA_BANK0  0x06
#define DEFVALB_BANK0  0x07
#define INTCONA_BANK0  0x08
#define INTCONB_BANK0  0x09
#define IOCON_BANK0   0x0A
#define GPPUA_BANK0    0x0C
#define GPPUB_BANK0    0x0D
#define INTFA_BANK0    0x0E
#define INTFB_BANK0    0x0F
#define INTCAPA_BANK0  0x10
#define INTCAPB_BANK0  0x11
#define GPIOA_BANK0    0x12
#define GPIOB_BANK0    0x13
#define OLATA_BANK0    0x14
#define OLATB_BANK0    0x15

  // register bits defines
// register IODIRA
#define IODIRA_IO0 1
#define IODIRA_IO1 (1<<1)
#define IODIRA_IO2 (1<<2)
#define IODIRA_IO3 (1<<3)
#define IODIRA_IO4 (1<<4)
#define IODIRA_IO5 (1<<5)
#define IODIRA_IO6 (1<<6)
#define IODIRA_IO7 (1<<7)

//register IODIRB
#define IODIRB_IO0 1
#define IODIRB_IO1 (1<<1)
#define IODIRB_IO2 (1<<2)
#define IODIRB_IO3 (1<<3)
#define IODIRB_IO4 (1<<4)
#define IODIRB_IO5 (1<<5)
#define IODIRB_IO6 (1<<6)
#define IODIRB_IO7 (1<<7)

// register IPOLA
#define IPOLA_IP0 1
#define IPOLA_IP1 (1<<1)
#define IPOLA_IP2 (1<<2)
#define IPOLA_IP3 (1<<3)
#define IPOLA_IP4 (1<<4)
#define IPOLA_IP5 (1<<5)
#define IPOLA_IP6 (1<<6)
#define IPOLA_IP7 (1<<7)

// register IPOLB
#define IPOLB_IP0 1
#define IPOLB_IP1 (1<<1)
#define IPOLB_IP2 (1<<2)
#define IPOLB_IP3 (1<<3)
#define IPOLB_IP4 (1<<4)
#define IPOLB_IP5 (1<<5)
#define IPOLB_IP6 (1<<6)
#define IPOLB_IP7 (1<<7)

// register GPINTENA
#define GPINTENA_GPINT0 1
#define GPINTENA_GPINT1 (1<<1)
#define GPINTENA_GPINT2 (1<<2)
#define GPINTENA_GPINT3 (1<<3)
#define GPINTENA_GPINT4 (1<<4)
#define GPINTENA_GPINT5 (1<<5)
#define GPINTENA_GPINT6 (1<<6)
#define GPINTENA_GPINT7 (1<<7)

// register GPINTENB
#define GPINTENB_GPINT0 1
#define GPINTENB_GPINT1 (1<<1)
#define GPINTENB_GPINT2 (1<<2)
#define GPINTENB_GPINT3 (1<<3)
#define GPINTENB_GPINT4 (1<<4)
#define GPINTENB_GPINT5 (1<<5)
#define GPINTENB_GPINT6 (1<<6)
#define GPINTENB_GPINT7 (1<<7)

// register DEFVALA
#define DEFVALA_DEF0 1
#define DEFVALA_DEF1 (1<<1)
#define DEFVALA_DEF2 (1<<2)
#define DEFVALA_DEF3 (1<<3)
#define DEFVALA_DEF4 (1<<4)
#define DEFVALA_DEF5 (1<<5)
#define DEFVALA_DEF6 (1<<6)
#define DEFVALA_DEF7 (1<<7)

// register DEFVALB
#define DEFVALB_DEF0 1
#define DEFVALB_DEF1 (1<<1)
#define DEFVALB_DEF2 (1<<2)
#define DEFVALB_DEF3 (1<<3)
#define DEFVALB_DEF4 (1<<4)
#define DEFVALB_DEF5 (1<<5)
#define DEFVALB_DEF6 (1<<6)
#define DEFVALB_DEF7 (1<<7)

// register INTCONA
#define INTCONA_IOC0 1
#define INTCONA_IOC1 (1<<1)
#define INTCONA_IOC2 (1<<2)
#define INTCONA_IOC3 (1<<3)
#define INTCONA_IOC4 (1<<4)
#define INTCONA_IOC5 (1<<5)
#define INTCONA_IOC6 (1<<6)
#define INTCONA_IOC7 (1<<7)

// register INTCONB
#define INTCONB_IOC0 1
#define INTCONB_IOC1 (1<<1)
#define INTCONB_IOC2 (1<<2)
#define INTCONB_IOC3 (1<<3)
#define INTCONB_IOC4 (1<<4)
#define INTCONB_IOC5 (1<<5)
#define INTCONB_IOC6 (1<<6)
#define INTCONB_IOC7 (1<<7)

// register IOCON
#define IOCON_INTPOL (1<<1)
#define IOCON_ODR    (1<<2)
#define IOCON_HAEN   (1<<3)
#define IOCON_DISSLW (1<<4)
#define IOCON_SEQOP  (1<<5)
#define IOCON_MIRROR (1<<6)
#define IOCON_BANK   (1<<7)

// register GPPUA
#define GPPUA_PU0 1
#define GPPUA_PU1 (1<<1)
#define GPPUA_PU2 (1<<2)
#define GPPUA_PU3 (1<<3)
#define GPPUA_PU4 (1<<4)
#define GPPUA_PU5 (1<<5)
#define GPPUA_PU6 (1<<6)
#define GPPUA_PU7 (1<<7)

// register GPPUB
#define GPPUB_PU0 1
#define GPPUB_PU1 (1<<1)
#define GPPUB_PU2 (1<<2)
#define GPPUB_PU3 (1<<3)
#define GPPUB_PU4 (1<<4)
#define GPPUB_PU5 (1<<5)
#define GPPUB_PU6 (1<<6)
#define GPPUB_PU7 (1<<7)

// register INTFA
#define INTFA_INT0 1
#define INTFA_INT1 (1<<1)
#define INTFA_INT2 (1<<2)
#define INTFA_INT3 (1<<3)
#define INTFA_INT4 (1<<4)
#define INTFA_INT5 (1<<5)
#define INTFA_INT6 (1<<6)
#define INTFA_INT7 (1<<7)

// register INTFB
#define INTFB_INT0 1
#define INTFB_INT1 (1<<1)
#define INTFB_INT2 (1<<2)
#define INTFB_INT3 (1<<3)
#define INTFB_INT4 (1<<4)
#define INTFB_INT5 (1<<5)
#define INTFB_INT6 (1<<6)
#define INTFB_INT7 (1<<7)

// register INTCAPA
#define INTCAPA_ICP0 1
#define INTCAPA_ICP1 (1<<1)
#define INTCAPA_ICP2 (1<<2)
#define INTCAPA_ICP3 (1<<3)
#define INTCAPA_ICP4 (1<<4)
#define INTCAPA_ICP5 (1<<5)
#define INTCAPA_ICP6 (1<<6)
#define INTCAPA_ICP7 (1<<7)

// register INTCAPB
#define INTCAPB_ICP0 1
#define INTCAPB_ICP1 (1<<1)
#define INTCAPB_ICP2 (1<<2)
#define INTCAPB_ICP3 (1<<3)
#define INTCAPB_ICP4 (1<<4)
#define INTCAPB_ICP5 (1<<5)
#define INTCAPB_ICP6 (1<<6)
#define INTCAPB_ICP7 (1<<7)

// register GPIOA
#define GPIOA_GP0 1
#define GPIOA_GP1 (1<<1)
#define GPIOA_GP2 (1<<2)
#define GPIOA_GP3 (1<<3)
#define GPIOA_GP4 (1<<4)
#define GPIOA_GP5 (1<<5)
#define GPIOA_GP6 (1<<6)
#define GPIOA_GP7 (1<<7)

// register GPIOB
#define GPIOB_GP0 1
#define GPIOB_GP1 (1<<1)
#define GPIOB_GP2 (1<<2)
#define GPIOB_GP3 (1<<3)
#define GPIOB_GP4 (1<<4)
#define GPIOB_GP5 (1<<5)
#define GPIOB_GP6 (1<<6)
#define GPIOB_GP7 (1<<7)

// register OLATA
#define OLATA_OL0 1
#define OLATA_OL1 (1<<1)
#define OLATA_OL2 (1<<2)
#define OLATA_OL3 (1<<3)
#define OLATA_OL4 (1<<4)
#define OLATA_OL5 (1<<5)
#define OLATA_OL6 (1<<6)
#define OLATA_OL7 (1<<7)

// register OLATB
#define OLATB_OL0 1
#define OLATB_OL1 (1<<1)
#define OLATB_OL2 (1<<2)
#define OLATB_OL3 (1<<3)
#define OLATB_OL4 (1<<4)
#define OLATB_OL5 (1<<5)
#define OLATB_OL6 (1<<6)
#define OLATB_OL7 (1<<7)

#endif /* INC_EXPAND2CLICK_H_ */
