//////////////////////////////////////////////////////////////////////
////                                                              ////
//// TPS65217 control                                             ////
////                                                              ////
//// This file contains functions to control the power management ////
//// IC TPS65217 from Texas Instruments.                          ////
//// It uses swi2cmst.h and swi2cmst.c for I2C communtication     ////
//// with the device.                                             ////
////                                                              ////
//// Description                                                  ////
//// Implementation according to ref [1].                         ////
////                                                              ////
//// References                                                   ////
//// [1] TPS65217A, TPS65217B, TPS65217C                          ////
////     SINGLE-CHIP PMIC FOR BATTERY-POWERED SYSTEMS             ////
////     SLVSB64E,  November 2011 – July 2012,                    ////
////     Texas Instruments Incorporated                           ////
////     http://www.ti.com/lit/gpn/tps65217a                      ////
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

#ifndef TPS65217_H_
#define TPS65217_H_

#include "swi2cmst.h"

#define TPS65217_SLAVEADDR      0x24

#define TPS65217_CHIPID		    0x00
#define TPS65217A               0x7
#define TPS65217B               0xF

#define TPS65217_PASSWORD		0x0b
#define TPS65217_PASSWORD_VALUE	0x7d

#define TPS65217_DEFDCDC1		0x0E
#define TPS65217_DEFDCDC2		0x0F
#define TPS65217_DEFDCDC3		0x10
#define TPS65217_DEFDCDC_1V1	0x08
#define TPS65217_DEFDCDC_1V2	0x0c
#define TPS65217_DEFDCDC_1V5	0x18
#define TPS65217_DEFDCDC_3V3	0x38

#define TPS65217_DEFSLEW		0x11
#define TPS65217_DEFSLEW_GO     0x80
#define TPS65217_DEFSLEW_FAST   0x06

#define TPS65217_DEFLDO1	    0x12
#define TPS65217_DEFLDO2	    0x13
#define TPS65217_DEFLDO_1V1	    0x01
#define TPS65217_DEFLDO_3V3	    0x0F

#define TPS65217_DEFLS1  	    0x14
#define TPS65217_DEFLS2  	    0x15
#define TPS65217_DEFLS_LS       0x00
#define TPS65217_DEFLS_LDO      0x20
#define TPS65217_DEFLS_2V5      0x0F
#define TPS65217_DEFLS_3V3      0x1F

#define TPS65217_ENABLE  	    0x16
#define TPS65217_ENABLE_LS1_EN  0x40
#define TPS65217_ENABLE_LS2_EN  0x20

#define TPS65217_OK             SWI2CMST_OK
#define TPS65217_ERROR          SWI2CMST_ERROR

unsigned int tps65217_wrReg(unsigned char reg, unsigned char  data);
unsigned int tps65217_rdReg(unsigned char reg, unsigned char* pData);
unsigned int tps65217_chipId(unsigned char* pChipId);

#endif /* TPS65217_H_ */
