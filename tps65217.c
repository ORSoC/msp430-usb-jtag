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
//// Usage                                                        ////
//// 1. Initialize the I2C bus by calling swi2cmst_init() in      ////
///     swi2cmst.c.                                               ////
//// 2. Optional: Clear potentially hung I2C slaves by calling    ////
////    swi2cmst_clrbus(). Not guaranteed to work on all slaves.  ////
////    Slaves may be hung if the CPU/MCU was reset during an     ////
////    ongoing I2C transaction. There may also be other reasons. ////
//// 3. Perform accesses to TPS65217 by calling one or more of    ////
////    tps65217_chipId(), tps65217_wrReg(), tps65217_rdReg().    ////
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

#include "tps65217.h"

unsigned int tps65217_wrReg(unsigned char reg, unsigned char data)
{
	return swi2cmst_wr2(TPS65217_SLAVEADDR, reg, data);
}

unsigned int tps65217_rdReg(unsigned char reg, unsigned char* pData)
{
	return swi2cmst_wr1rd1(TPS65217_SLAVEADDR, reg, pData);
}

unsigned int tps65217_chipId(unsigned char* pChipId)
{
	return tps65217_rdReg(TPS65217_CHIPID, pChipId);
}
