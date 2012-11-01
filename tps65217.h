/*
 * tps65217.h
 *
 *  Created on: Oct 24, 2012
 *      Author: per
 */

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
