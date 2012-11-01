/*
 * tps65217.c
 *
 *  Created on: Oct 24, 2012
 *      Author: per
 */

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


