//////////////////////////////////////////////////////////////////////
////                                                              ////
//// MSP430_USB_JTAG Board Configurations                         ////
////                                                              ////                                                              ////
//// Description                                                  ////
//// Defines board configuration.                                 ////
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

#ifndef CFG_H_
#define CFG_H_

// Make sure only one of the following configurations are uncommented

// OLIMEX OLIMEXINO-5510 board (with an MSP430F5510 MCU) connected to an ORSOC_8695_EP4GX board as follows:
// TODO: describe connections and modifications
//#define OLIMEXINO5510_ORSOC8695EP4GX

// ORSoC ORDDB3A board
//#define ORDB3A
#if ! (defined(ORDB3A) || defined(OLIMEXINO5510_ORSOC8695EP4GX))
#error Undefined board
#endif

#endif /* CFG_H_ */
