/*
 * main.c
 */

#include "cfg.h"
#include "defs.h"
#include "tps65217.h"

//----------------------------------------------------------------------------
unsigned int boardInit(void);
unsigned int boardInitOlimexino5510Orsoc8695ep4gx(void);
unsigned int boardInitOrdb3a(void);
void ledFlash(int numFlashes, int infiniteLoop);

void main(void) {
	unsigned int result;
	int numFlashes;

	WDTCTL = WDTPW + WDTHOLD;		// Stop watchdog timer

	if ( LED_AVAIL ) {
	  LED_INIT;       // Set LED PORT bit to output direction
      LED_OFF;
	}
	ledFlash(LED_NUMFLASH_STARTED, 0);

	while(1) { // TODO: remove this temporary infinite loop
	  result = boardInit();
	  numFlashes = (result == TPS65217_OK ? LED_NUMFLASH_OK : LED_NUMFLASH_ERROR);
  	  ledFlash(numFlashes, 0);
	}

	// Program flow should never get this far, except during development
	numFlashes = (result == OK ? LED_NUMFLASH_ENDED_OK : LED_NUMFLASH_ENDED_ERROR);
	ledFlash(numFlashes, 1);
}

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

unsigned int boardInitOlimexino5510Orsoc8695ep4gx(void)
{
	unsigned char chipId;
	unsigned char chipNum;
	unsigned char chipRev;
	unsigned int result;

	swi2cmst_init();
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

unsigned int boardInitOrdb3a(void)
{
	unsigned char chipId;
	unsigned char chipNum;
	unsigned char chipRev;
	unsigned int result;

	swi2cmst_init();
	result = tps65217_chipId(&chipId);
	if(result == TPS65217_OK) {
		chipRev = chipId & 0x0F;
		chipNum = chipId >> 4;
		if(chipNum != TPS65217A && chipNum != TPS65217B) {
			result = TPS65217_ERROR;
		}
	}

	PWR_EN_INIT;
	PWR_EN_OFF;

    // TODO: Check if there are power sequncing requirements
	result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_DEFDCDC1);
	result |= tps65217_wrReg(TPS65217_DEFDCDC1, TPS65217_DEFDCDC_1V5);
	result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_DEFDCDC1);
	result |= tps65217_wrReg(TPS65217_DEFDCDC1, TPS65217_DEFDCDC_1V5);

	result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_DEFDCDC2);
	result |= tps65217_wrReg(TPS65217_DEFDCDC2, TPS65217_DEFDCDC_3V3);
	result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_DEFDCDC2);
	result |= tps65217_wrReg(TPS65217_DEFDCDC2, TPS65217_DEFDCDC_3V3);

	result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_DEFDCDC3);
	result |= tps65217_wrReg(TPS65217_DEFDCDC3, TPS65217_DEFDCDC_1V1);
	result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_DEFDCDC3);
	result |= tps65217_wrReg(TPS65217_DEFDCDC3, TPS65217_DEFDCDC_1V1);

	result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_DEFLDO1);
	result |= tps65217_wrReg(TPS65217_DEFLDO1,  TPS65217_DEFLDO_3V3);
	result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_DEFLDO1);
	result |= tps65217_wrReg(TPS65217_DEFLDO1,  TPS65217_DEFLDO_3V3);

	result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_DEFLDO2);
	result |= tps65217_wrReg(TPS65217_DEFLDO2,  TPS65217_DEFLDO_1V1);
	result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_DEFLDO2);
	result |= tps65217_wrReg(TPS65217_DEFLDO2,  TPS65217_DEFLDO_1V1);

	result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_DEFLS1);
	result |= tps65217_wrReg(TPS65217_DEFLS1,   TPS65217_DEFLS_LDO | TPS65217_DEFLS_2V5);
	result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_DEFLS1);
	result |= tps65217_wrReg(TPS65217_DEFLS1,   TPS65217_DEFLS_LDO | TPS65217_DEFLS_2V5);

	result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_DEFLS2);
	result |= tps65217_wrReg(TPS65217_DEFLS2,   TPS65217_DEFLS_LS | TPS65217_DEFLS_3V3);
	result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_DEFLS2);
	result |= tps65217_wrReg(TPS65217_DEFLS2,   TPS65217_DEFLS_LS | TPS65217_DEFLS_3V3);

	result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_DEFSLEW);
	result |= tps65217_wrReg(TPS65217_DEFSLEW,  TPS65217_DEFSLEW_GO | TPS65217_DEFSLEW_FAST);
	result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_DEFSLEW);
	result |= tps65217_wrReg(TPS65217_DEFSLEW,  TPS65217_DEFSLEW_GO | TPS65217_DEFSLEW_FAST);

	result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_ENABLE);
	result |= tps65217_wrReg(TPS65217_ENABLE,   TPS65217_ENABLE_LS2_EN);
	result |= tps65217_wrReg(TPS65217_PASSWORD, TPS65217_PASSWORD_VALUE^TPS65217_ENABLE);
	result |= tps65217_wrReg(TPS65217_ENABLE,   TPS65217_ENABLE_LS2_EN);

	PWR_EN_ON;

	//TODO: init FPGA;

	return result;
}

//----------------------------------------------------------------------------
// ledFlash
//----------------------------------------------------------------------------
void ledFlash(int numFlashes, int infiniteLoop)
{
#ifdef LED_AVAIL

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
