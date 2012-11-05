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
//// Usage                                                        ////
//// 1. Initialize by calling swi2cmst_init().                    ////
//// 2. Optional: Clear potentially hung I2C slaves by calling    ////
////    swi2cmst_clrbus(). Not guaranteed to work on all slaves.  ////
////    Slaves may be hung if the CPU/MCU was reset during an     ////
////    ongoing I2C transaction. There may also be other reasons. ////
//// 3. Perform I2C accesses by calling one or more of            ////
////    swi2cmst_wr1(), swi2cmst_wr2(), swi2cmst_rd1(),           ////
////    swi2cmst_wr1rd1().                                        ////
////                                                              ////
//// References                                                   ////
//// [1] UM10204 I2C-Bus Specification and User Manual            ////
////     Rev.4, 13 February 2012, NXP B.V.                        ////
////     http://www.nxp.com/documents/user_manual/um10204.pdf     ////
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

#include "swi2cmst.h"

void swi2cmst_init(void)
{
	SWI2CMST_INIT;
}

void swi2cmst_start(void)
{
	SWI2CMST_SDA_HI;
	SWI2CMST_SCL_HI;
	SWI2CMST_SDA_LO;
	SWI2CMST_SCL_LO;
}

void swi2cmst_stop(void)
{
	SWI2CMST_SDA_LO;
	SWI2CMST_SCL_HI;
	SWI2CMST_SDA_HI;
}

void swi2cmst_wrbit(unsigned char b)
{
	SWI2CMST_SDA_SET(b);
	SWI2CMST_SCL_HI;
	SWI2CMST_DELAY;
	SWI2CMST_SCL_LO;
}

unsigned char swi2cmst_rdbit(void)
{
	unsigned char b;

	SWI2CMST_SCL_HI;
	b = SWI2CMST_SDA_GET;
	SWI2CMST_DELAY;
	SWI2CMST_SCL_LO;
	return b;
}

void swi2cmst_wrbyte(unsigned char x)
{
  unsigned int i;

  for ( i = 8; i > 0; i-- ) {
    swi2cmst_wrbit(x & SWI2CMST_MSB_MASK);
    x <<= 1;
  }
  SWI2CMST_SDA_HI;
}

unsigned char swi2cmst_rdbyte(void)
{
	unsigned int i;
	unsigned char x = 0;

    for( i = 8; i > 0; i-- ) {
    	x = (x << 1) | swi2cmst_rdbit();
    }
    return x;
}

void swi2cmst_wrack( void )
{
	swi2cmst_wrbit( SWI2CMST_ACK );
}

void swi2cmst_wrnack( void )
{
	swi2cmst_wrbit( SWI2CMST_NACK );
}

unsigned int swi2cmst_rdack( void )
{
	if ( swi2cmst_rdbit() == SWI2CMST_ACK ) {
		return SWI2CMST_OK;
	}
	else {
		return SWI2CMST_ERROR;
	}
}

unsigned int swi2cmst_clrbus( void )
{
	int i;

	// At least nine STOPs, to release potentially hung slaves
	for(i = 10; i != 0; i--) {
	   SWI2CMST_SCL_LO;
       swi2cmst_stop();
	}
	if (SWI2CMST_SDA_GET == 1 && SWI2CMST_SDA_GET == 1) {
		return SWI2CMST_OK;
	}
	else {
		return SWI2CMST_ERROR;
	}
}

unsigned int swi2cmst_wr1(unsigned int slaveAddr, unsigned int data)
{
	unsigned int result;

	swi2cmst_start();
	swi2cmst_wrbyte((slaveAddr << 1) | SWI2CMST_WRITE);
	result = swi2cmst_rdack();
	if(result == SWI2CMST_OK || SWI2CMST_IGNORE_NAK) {
		swi2cmst_wrbyte(data);
		result = swi2cmst_rdack();
	}
	swi2cmst_stop();
	return result;
}

unsigned int swi2cmst_wr2(unsigned int slaveAddr, unsigned char data1, unsigned char data2)
{
	unsigned int result;

	swi2cmst_start();
	swi2cmst_wrbyte((slaveAddr << 1) | SWI2CMST_WRITE);
	result = swi2cmst_rdack();
	if(result == SWI2CMST_OK || SWI2CMST_IGNORE_NAK) {
		swi2cmst_wrbyte(data1);
		result = swi2cmst_rdack();
	}
	if( result == SWI2CMST_OK  || SWI2CMST_IGNORE_NAK) {
		swi2cmst_wrbyte(data2);
		result = swi2cmst_rdack();
	}
	swi2cmst_stop();
	return result;
}

unsigned int swi2cmst_rd1(unsigned int slaveAddr, unsigned char* pData)
{
	unsigned int result;

	swi2cmst_start();
	swi2cmst_wrbyte((slaveAddr << 1) | SWI2CMST_READ);
	result = swi2cmst_rdack();
	if(result == SWI2CMST_OK || SWI2CMST_IGNORE_NAK) {
		*pData = swi2cmst_rdbyte();
		swi2cmst_wrnack();
	}
	swi2cmst_stop();
	return result;
}

unsigned int swi2cmst_wr1rd1(unsigned int slaveAddr, unsigned char wrData, unsigned char* pRdData)
{
	unsigned int result;

	swi2cmst_start();
	swi2cmst_wrbyte((slaveAddr << 1) | SWI2CMST_WRITE );
	result = swi2cmst_rdack();
	if( result == SWI2CMST_OK  || SWI2CMST_IGNORE_NAK ) {
		swi2cmst_wrbyte( wrData );
		result = swi2cmst_rdack();
	}
	if( result == SWI2CMST_OK  || SWI2CMST_IGNORE_NAK ) {
		swi2cmst_start();
		swi2cmst_wrbyte((slaveAddr << 1) | SWI2CMST_READ );
		result = swi2cmst_rdack();
	}
	if( result == SWI2CMST_OK  || SWI2CMST_IGNORE_NAK ) {
		*pRdData = swi2cmst_rdbyte();
		swi2cmst_wrnack();
	}
	swi2cmst_stop();
	return result;
}


