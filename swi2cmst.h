/*
 * swi2cmst.h
 *
 *  Created on: Oct 24, 2012
 *      Author: per
 */

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
// I2C SCL pin :
// I2C SDA pin :
// See ref [1], page 85, fig Port P6, P6.0 to P6.7, Input/Output Width Scmitt Trigger

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
#define SWI2CMST_SDA_GET                                     \
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
unsigned int swi2cmst_wr1(unsigned int slaveAddr, unsigned int data);
unsigned int swi2cmst_wr2(unsigned int slaveAddr, unsigned char data1, unsigned char data2);
unsigned int swi2cmst_rd1(unsigned int slaveAddr, unsigned char* pData);
unsigned int swi2cmst_wr1rd1(unsigned int slaveAddr, unsigned char wrData, unsigned char* pRdData);

#endif /* SWI2CMST_H_ */
