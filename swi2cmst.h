//////////////////////////////////////////////////////////////////////
////                                                              ////
//// Software driven I2C Master                                   ////
////                                                              ////
//// This is a simple I2C Master for use when not all of the      ////
//// advanced I2C featured are required.                          ////
//// It does not require specific I2C hardware. Instead it does   ////
//// bit-banging of GPIO ports.                                   ////
////                                                              ////
//// Description                                                  ////
//// Implementation of the I2C protocol according to ref [1].     ////
////                                                              ////
//// References                                                   ////
//// [1] UM10204 I2C-Bus Specification and User Manual            ////
////     Rev.4, 13 February 2012, NXP B.V.                        ////
////     http://www.nxp.com/documents/user_manual/um10204.pdf     ////
////                                                              ////
//// [2] MSP430F550x, MSP430F5510 Mixed Signal Microcontroller    ////
////     SLAS645G, July 2009 - October 2012,                      ////
////     Texas Instruments Incorporated                           ////
////     http://www.ti.com/lit/gpn/msp430f5510                    ////
////                                                              ////
//// [3] OLIMEXINO-5510 Development Board User's Manual           ////
////     https://www.olimex.com/Products/Duino/MSP430/            ////
////           OLIMEXINO-5510/resources/OLIMEXINO-5510_Rev_A.pdf  ////
////                                                              ////
//// To Do:                                                       ////
////  - Multi-master support (if and when there is a need)        ////
////  - Functions for reading and writing data blocks with arrays ////
////    (if and when there is a need)                             ////
////  - More calls to SWI2CMST_DELAY here and there               ////
////    (if there is a need)                                      ////
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

#ifndef SWI2CMST_H_
#define SWI2CMST_H_

#include "cfg.h"

//----------------------------------------------------------------------------
// Board/CPU specific defines
//----------------------------------------------------------------------------
#if defined( OLIMEXINO5510_ORSOC8695EP4GX ) || defined ( ORDB3A )

// Manufacturer: OLIMEX
// Board name  : OLIMEXINO-5510
// CPU         : TI MSP430F5510
// I2C SCL pin : P6.1 (pin 2) (for compatibility with ORDB3A)
// I2C SDA pin : P6.0 (pin 1) (for compatibility with ORDB3A)
// See ref [2], page 85, fig Port P6, P6.0 to P6.7, Input/Output Width Schmitt Trigger
//     ref [3]

// Manufacturer: ORSoC AB
// Board name  : ORDB3A
// CPU         : TI MSP430F5510
// I2C SCL pin : P6.1 (pin 2)
// I2C SDA pin : P6.0 (pin 1)
// See ref [2], page 85, fig Port P6, P6.0 to P6.7, Input/Output Width Schmitt Trigger

#include <msp430f5510.h>

#define SWI2CMST_PSEL   P6SEL // Port selection register
#define SWI2CMST_PREN   P6REN // Port internal pull-up/pull-down resistance reg
#define SWI2CMST_PDIR   P6DIR // Port direction register
#define SWI2CMST_POUT   P6OUT // Port output register
#define SWI2CMST_PINP   P6IN  // Port input register

#define SWI2CMST_BSDA   BIT0  // Bit in registers for SDA line
#define SWI2CMST_BSCL   BIT1  // Bit in registers for SCL line

#define SWI2CMST_INIT                                       \
                        do {                                \
                          SWI2CMST_PSEL &= ~(SWI2CMST_BSCL | SWI2CMST_BSDA); \
                          SWI2CMST_PREN |=  (SWI2CMST_BSCL | SWI2CMST_BSDA); \
                          SWI2CMST_PDIR &= ~(SWI2CMST_BSCL | SWI2CMST_BSDA); \
                          SWI2CMST_POUT |=  (SWI2CMST_BSCL | SWI2CMST_BSDA); \
                        } while ( 0 )

#define SWI2CMST_SCL_LO                                     \
                        do {                                \
                          SWI2CMST_POUT &= ~SWI2CMST_BSCL;  \
                          SWI2CMST_PDIR |=  SWI2CMST_BSCL;  \
                        } while ( 0 )
#define SWI2CMST_SCL_HI                                     \
	                    do {                                \
                          SWI2CMST_PDIR &= ~SWI2CMST_BSCL;  \
                          SWI2CMST_POUT |=  SWI2CMST_BSCL;  \
                        } while ( 0 )
#define SWI2CMST_SCL_GET                                    \
	                    (SWI2CMST_PINP & SWI2CMST_BSCL ? 1 : 0)
#define SWI2CMST_SDA_LO                                     \
	                    do {                                \
                          SWI2CMST_POUT &= ~SWI2CMST_BSDA;  \
                          SWI2CMST_PDIR |=  SWI2CMST_BSDA;  \
                        } while ( 0 )
#define SWI2CMST_SDA_HI                                     \
	                    do {                                \
                          SWI2CMST_PDIR &= ~SWI2CMST_BSDA;  \
                          SWI2CMST_POUT |=  SWI2CMST_BSDA;  \
                        } while ( 0 )
#define SWI2CMST_SDA_SET(x)                                 \
                        do {                                \
                          if(x) SWI2CMST_SDA_HI;            \
                          else  SWI2CMST_SDA_LO;            \
                        } while ( 0 )
#define SWI2CMST_SDA_GET                                    \
	                    (SWI2CMST_PINP & SWI2CMST_BSDA ? 1 : 0)
#define SWI2CMST_DELAY  _NOP()


#else
#error No board defined.
#endif

//----------------------------------------------------------------------------
// General
//----------------------------------------------------------------------------
#define SWI2CMST_WRITE          0
#define SWI2CMST_READ           1
#define SWI2CMST_ACK            0
#define SWI2CMST_NACK           1
#define SWI2CMST_MSB_MASK       0x80
#define SWI2CMST_OK             0
#define SWI2CMST_ERROR          1
#define SWI2CMST_IGNORE_NAK     1 // 0 for normal operation, 1 for debugging without attached slave

void swi2cmst_init(void);
void swi2cmst_start(void);
void swi2cmst_stop(void);
void swi2cmst_wrbit(unsigned char b);
unsigned char swi2cmst_rdbit(void);
void swi2cmst_wrbyte(unsigned char x);
unsigned char swi2cmst_rdbyte(void);
void swi2cmst_wrack( void );
void swi2cmst_wrnack( void );
unsigned int swi2cmst_rdack( void );
unsigned int swi2cmst_clrbus( void );
unsigned int swi2cmst_wr1(unsigned int slaveAddr, unsigned int data);
unsigned int swi2cmst_wr2(unsigned int slaveAddr, unsigned char data1, unsigned char data2);
unsigned int swi2cmst_rd1(unsigned int slaveAddr, unsigned char* pData);
unsigned int swi2cmst_wr1rd1(unsigned int slaveAddr, unsigned char wrData, unsigned char* pRdData);

#endif /* SWI2CMST_H_ */
