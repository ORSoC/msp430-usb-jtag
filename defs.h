//////////////////////////////////////////////////////////////////////
////                                                              ////
//// MSP430_USB_JTAG Definitions                                  ////
////                                                              ////                                                              ////
//// Description                                                  ////
//// Definintions for general and board specific constants.       ////
////                                                              ////
//// References                                                   ////
//// [1] MSP430F550x, MSP430F5510 Mixed Signal Microcontroller    ////
////     SLAS645G, July 2009 - October 2012,                      ////
////     Texas Instruments Incorporated                           ////
////     http://www.ti.com/lit/gpn/msp430f5510                    ////
////                                                              ////
//// [2] OLIMEXINO-5510 Development Board User's Manual           ////
////     https://www.olimex.com/Products/Duino/MSP430/            ////
////           OLIMEXINO-5510/resources/OLIMEXINO-5510_Rev_A.pdf  ////
////                                                              ////
//// To Do:                                                       ////
////   -                                                          ////
////                                                              ////
//// Author(s):                                                   ////
////   - Per Larsson, per.larsson@orsoc.se                        ////
////                                                              ////
//////////////////////////////////////////////////////////////////////
////                                                              ////
//// Copyright (C) 2012 Authors and ORSoC AB, Sweden              ////
////                                                              ////
//// This source file may be used and distributed without         ////
//// restriction provided that this copyright statement is not    ////
//// removed from the file and that any derivative work contains  ////
//// the original copyright notice and the associated disclaimer. ////
////                                                              ////
//// This source file is free software; you can redistribute it   ////
//// and/or modify it under the terms of the GNU Lesser General   ////
//// Public License as published by the Free Software Foundation; ////
//// either version 2.1 of the License, or (at your option) any   ////
//// later version.                                               ////
////                                                              ////
//// This source is distributed in the hope that it will be       ////
//// useful, but WITHOUT ANY WARRANTY; without even the implied   ////
//// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      ////
//// PURPOSE. See the GNU Lesser General Public License for more  ////
//// details.                                                     ////
////                                                              ////
//// You should have received a copy of the GNU Lesser General    ////
//// Public License along with this source; if not, download it   ////
//// from http://www.opencores.org/lgpl.shtml                     ////
////                                                              ////
//////////////////////////////////////////////////////////////////////

#ifndef DEFS_H_
#define DEFS_H_

#include "cfg.h"

//----------------------------------------------------------------------------
// OLIMEXINO5510_ORSOC8695EP4GX specific definitions
// See Ref [1] & [2]
//----------------------------------------------------------------------------
#if defined ( OLIMEXINO5510_ORSOC8695EP4GX )

#define LED_AVAIL       1
#define LED_PDIR        PJDIR
#define LED_POUT        PJOUT
#define LED_PBIT        BIT3

// LED is connected between LED port of MCU and GND.
#define LED_INIT        (LED_PDIR |=  LED_PBIT) // Set LED PORT bit to output direction
#define LED_OFF         (LED_POUT &= ~LED_PBIT)
#define LED_ON          (LED_POUT |=  LED_PBIT)
#define LED_TOGGLE      (LED_POUT ^=  LED_PBIT)
#define LED_FLASH_TIME  20000
#define LED_FLASH_PAUSE 4

#define PWR_EN_AVAIL    0
// TODO: Remove the following temporary PWR_EN_ defines
#define PWR_EN_PSEL     P6SEL
#define PWR_EN_PDIR     P6DIR
#define PWR_EN_POUT     P6OUT
#define PWR_EN_PINP     P6INP
#define PWR_EN_PBIT     BIT2

#define PWR_EN_INIT     ((PWR_EN_PDIR) |=  (PWR_EN_PBIT)) // Set output direction
#define PWR_EN_OFF      ((PWR_EN_POUT) &= ~(PWR_EN_PBIT))
#define PWR_EN_ON       ((PWR_EN_POUT) |=  (PWR_EN_PBIT))

// Note: Pins for I2C are defined in swi2cmst.h

//----------------------------------------------------------------------------
// ORDB3A specific definitions
// See ref [1]
//----------------------------------------------------------------------------
#elif defined( ORDB3A )

//#define LED_AVAIL     0 // TODO: uncomment

// TODO: Remove the following temporary LED_ defines
#define LED_AVAIL       0
#if LED_AVAIL
#define LED_PDIR        PJDIR
#define LED_POUT        PJOUT
#define LED_PBIT        BIT3

#define LED_INIT        (LED_PDIR |=  LED_PBIT) // Sets LED PORT bit to output direction
#define LED_OFF         (LED_POUT &= ~LED_PBIT)
#define LED_ON          (LED_POUT |=  LED_PBIT)
#define LED_TOGGLE      (LED_POUT ^=  LED_PBIT)
#define LED_FLASH_TIME  20000
#define LED_FLASH_PAUSE 4
#else
#define LED_ON	/* missing */
#define LED_OFF	/* missing */
#define LED_INIT	/* missing */
#define LED_TOGGLE	/* missing */
#define LED_FLASH_TIME	0
#define LED_FLASH_PAUSE	0
#endif

#define PWR_EN_AVAIL    1
#define PWR_EN_PSEL     P6SEL
#define PWR_EN_PDIR     P6DIR
#define PWR_EN_POUT     P6OUT
#define PWR_EN_PINP     P6INP
#define PWR_EN_PBIT     BIT2

#define PWR_EN_INIT	do {			\
		P6DIR &= ~PWR_EN_PBIT;		\
		P6OUT &= ~PWR_EN_PBIT;		\
		P6REN |= PWR_EN_PBIT;		\
	} while(0)
//#define PWR_EN_INIT     ((PWR_EN_PDIR) |=  (PWR_EN_PBIT)) // Set output direction
#define PWR_EN_OFF      ((PWR_EN_POUT) &= ~(PWR_EN_PBIT))
#define PWR_EN_ON       ((PWR_EN_POUT) |=  (PWR_EN_PBIT))

// Note: Pins for I2C are defined in swi2cmst.h


#else
#error No board defined.
#endif

//----------------------------------------------------------------------------
// Board independent definitions
//----------------------------------------------------------------------------
#define OK              (TPS65217_OK)
#define ERROR           (TPS65217_ERROR)

#define LED_NUMFLASH_STARTED        1
#define LED_NUMFLASH_OK             2
#define LED_NUMFLASH_ENDED_OK       3
#define LED_NUMFLASH_ERROR          5
#define LED_NUMFLASH_ENDED_ERROR    6

#endif /* DEFS_H_ */
