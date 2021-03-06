//////////////////////////////////////////////////////////////////////
////                                                              ////
//// MSP430_USB_JTAG                                              ////
////                                                              ////
//// This software controls functions for the following:          ////
//// * USB Device for control from a host computer                ////
//// * JTAG configuration of FPGAs                                ////
//// * Flash memory control                                       ////
//// * Control of power mangegement IC                            ////
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
////   - Lots                                                     ////
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

#include "cfg.h"
#include "defs.h"
#include "tps65217.h"

//----------------------------------------------------------------------------
unsigned int boardInit(void);
unsigned int boardInitOlimexino5510Orsoc8695ep4gx(void);
unsigned int boardInitOrdb3a(void);
void ledFlash(int numFlashes, int infiniteLoop);

//----------------------------------------------------------------------------
// boardInit
//----------------------------------------------------------------------------
unsigned int boardInit(void)
{
#if defined (OLIMEXINO5510_ORSOC8695EP4GX)
	return boardInitOlimexino5510Orsoc8695ep4gx();
#elif defined ( ORDB3A )
    return boardInitOrdb3a();
#else
#error Undefined board
#endif
}

#ifdef OLIMEXINO5510_ORSOC8695EP4GX
unsigned int boardInitOlimexino5510Orsoc8695ep4gx(void)
{
	unsigned char chipId;
	unsigned char chipNum;
	unsigned char chipRev;
	unsigned int result;

	swi2cmst_init();
	swi2cmst_clrbus();
	result = tps65217_chipId(&chipId);
	if(result == TPS65217_OK) {
		chipRev = chipId & 0x0F;
		chipNum = chipId >> 4;
		if(chipNum != TPS65217A && chipNum != TPS65217B) {
			result = TPS65217_ERROR;
		}
	}

	result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_DEFDCDC3);
	result |= tps65217_wrReg(TPS65217_DEFDCDC3, TPS65217_DEFDCDC_1V2);
	result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_DEFDCDC3);
	result |= tps65217_wrReg(TPS65217_DEFDCDC3, TPS65217_DEFDCDC_1V2);
	result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_DEFSLEW);
	result |= tps65217_wrReg(TPS65217_DEFSLEW,  TPS65217_DEFSLEW_GO | TPS65217_DEFSLEW_FAST);
	result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_DEFSLEW);
	result |= tps65217_wrReg(TPS65217_DEFSLEW,  TPS65217_DEFSLEW_GO | TPS65217_DEFSLEW_FAST);

	// TODO: init FPGA
	return result;
}
#endif

#ifdef ORDB3A
unsigned int boardInitOrdb3a(void)
{
	unsigned char chipId;
	unsigned char chipNum;
	unsigned char chipRev;
	unsigned int result;

	//PWR_EN_OFF;	// Critical; output level is undefined at startup in msp430!
	PWR_EN_INIT;

	do {
		LED_OFF;
		swi2cmst_init();
		swi2cmst_clrbus();
		result = tps65217_chipId(&chipId);
		if(result == TPS65217_OK) {
			chipRev = chipId & 0x0F;
			chipNum = chipId >> 4;
			if(chipNum != TPS65217A && chipNum != TPS65217B) {
				result = TPS65217_ERROR;
				continue;
			}
		} else {
			continue;
		}

		// TODO: Check if there are power sequncing requirements
		/* Macro to set a level 2 password-protected register, read back and verify */
#define SET_TPS_REG_PW2(reg,val) { unsigned char rv;			\
			result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^(reg)); \
			result |= tps65217_wrReg(reg, val);		\
			result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^(reg)); \
			result |= tps65217_wrReg(reg, val);		\
			result |= tps65217_rdReg(reg, &rv);		\
			if (rv != (val)) continue;			\
		}

		SET_TPS_REG_PW2(TPS65217_DEFDCDC1, TPS65217_DEFDCDC_1V5);
		SET_TPS_REG_PW2(TPS65217_DEFDCDC2, TPS65217_DEFDCDC_3V3);
		SET_TPS_REG_PW2(TPS65217_DEFDCDC3, TPS65217_DEFDCDC_1V1);
		SET_TPS_REG_PW2(TPS65217_DEFLDO1, TPS65217_DEFLDO_3V3);
		SET_TPS_REG_PW2(TPS65217_DEFLDO2, TPS65217_DEFLDO_1V1);
		SET_TPS_REG_PW2(TPS65217_DEFLS1, TPS65217_DEFLS_LDO | TPS65217_DEFLS_2V5);
		SET_TPS_REG_PW2(TPS65217_DEFLS2, TPS65217_DEFLS_LS | TPS65217_DEFLS_3V3);

		/* Now that all the voltages are set, we might want to turn the power on. */

		/* Copy the new values into active registers for all regulators */
		/* No readback here, because the GO bit autoclears */
	result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_DEFSLEW);
	result |= tps65217_wrReg(TPS65217_DEFSLEW,  TPS65217_DEFSLEW_GO | TPS65217_DEFSLEW_FAST);
	result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_DEFSLEW);
	result |= tps65217_wrReg(TPS65217_DEFSLEW,  TPS65217_DEFSLEW_GO | TPS65217_DEFSLEW_FAST);

	/* TODO: at this point, the MSP430 power voltage rises from 1.8V to 3.3V. Scale other 
	   functions? */

	/* Enable lines: FIXME use sequencer? */
	/* Sequencer will power up all other rails, just turn on uC and VBUS */
	SET_TPS_REG_PW2(TPS65217_ENABLE, 
			TPS65217_ENABLE_LDO1_EN | // MCU
			0x7f /* enable all */);
	// USB should be possible from here

	// TODO: Fix: It seems like the TP265217 hangs here, with both SCL and SDA low.
	//       This was found with a quick measurement, which may be inaccurate.

	// TODO: Set sequencing registers

		PWR_EN_ON;	// At this point, PM chip will power up all the other rails
		LED_ON;  // power management chip detected, so far so good
		break;  // If we got this far, things are looking good

	//TODO: init FPGA;
	} while (1);

	return result;
}

#define USE_SEQUENCER

void fpga_powerdown(void) {
#ifndef USE_SEQUENCER
	/* Simply shut everything off at once. Don't do this during flash programming. */
	tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_ENABLE);
	tps65217_wrReg(TPS65217_ENABLE, TPS65217_ENABLE_LDO1_EN //);  // Keep MCU power
		       | TPS65217_ENABLE_DC2_EN); // Main 3.3V for debugging purposes
#else
	// Alternatively, we could use SEQ6 bits SEQUP and SEQDWN if sequencer was 
	// set up nicely. The default setting turns on LS2 which we don't need, but 
	// does leave LDO1 alone, so we could use it. 
	tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_SEQ6);
	tps65217_wrReg(TPS65217_SEQ6, TPS65217_SEQ6_SEQDWN);
#endif	
}

void fpga_powerup(void) {
#ifndef USE_SEQUENCER
	/* Simply turn everything on at once. Don't do this during flash programming. */
	tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_ENABLE);
	tps65217_wrReg(TPS65217_ENABLE, 
		       TPS65217_ENABLE_LDO1_EN |  // MCU power
		       TPS65217_ENABLE_DC1_EN | // 1.5V power for DDR RAM
		       TPS65217_ENABLE_DC2_EN | // Main 3.3V, including NAND and FPGA IO
		       TPS65217_ENABLE_DC3_EN | // FPGA core
		       TPS65217_ENABLE_LDO2_EN | // FPGA transceivers
		       TPS65217_ENABLE_LS1_EN); // FPGA PLL power
	// Not included: LS2, which is not used inside the module. 
#else
	tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_SEQ6);
	tps65217_wrReg(TPS65217_SEQ6, TPS65217_SEQ6_SEQUP);
#endif
}

int check_pushbutton(void) {
	unsigned char statusReg = 0;
	tps65217_rdReg(TPS65217_STATUS, &statusReg);
	return statusReg & TPS65217_STATUS_PB;
}
#endif

//----------------------------------------------------------------------------
// ledFlash
//----------------------------------------------------------------------------
void ledFlash(int numFlashes, int infiniteLoop)
{
#if LED_AVAIL

	int i;
	volatile unsigned int j;	// volatile to prevent optimization

	LED_OFF;
	do {
      for ( i = 2 * numFlashes; i != 0; i-- ) {
		LED_TOGGLE;
		j = LED_FLASH_TIME;
        while( j-- );
      }
      if ( numFlashes > 0 ) {
    	for ( i = LED_FLASH_PAUSE; i != 0; i-- ) {
          j = LED_FLASH_TIME;
          while(j--);
    	}
      }
	} while ( infiniteLoop );

#else

	while( infiniteLoop );

#endif
}
