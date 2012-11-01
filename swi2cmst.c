/*
 * swi2cmst.c
 *
 *  Created on: Oct 24, 2012
 *      Author: per
 */

#include "swi2cmst.h"

// TODO: More SWI2CMST_DELAY here and there
// TODO: Multi-master support

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
	swi2cmst_wrbyte((slaveAddr << 1) | SWI2CMST_WRITE);
	result = swi2cmst_rdack();
	if( result == SWI2CMST_OK  || SWI2CMST_IGNORE_NAK) {
		swi2cmst_wrbyte( wrData );
		result = swi2cmst_rdack();
	}
	if( result == SWI2CMST_OK  || SWI2CMST_IGNORE_NAK) {
		swi2cmst_start();
		swi2cmst_wrbyte((slaveAddr << 1) | SWI2CMST_READ);
		result = swi2cmst_rdack();
	}
	if( result == SWI2CMST_OK  || SWI2CMST_IGNORE_NAK) {
		*pRdData = swi2cmst_rdbyte();
		swi2cmst_wrnack();
	}
	swi2cmst_stop();
	return result;
}

// TODO: functions for writing and reading larger blocks of data with arrays (when there is a need).

