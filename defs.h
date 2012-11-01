/*
 * defs.h
 *
 *  Created on: Oct 29, 2012
 *      Author: per
 */

#ifndef DEFS_H_
#define DEFS_H_

#include "cfg.h"

//----------------------------------------------------------------------------
// OLIMEXINO5510_ORSOC8695EP4GX specific definitions
//----------------------------------------------------------------------------
#if defined ( OLIMEXINO5510_ORSOC8695EP4GX )

#define LED_AVAIL       1
#define LED_PDIR        PJDIR
#define LED_POUT        PJOUT
#define LED_PBIT        BIT3

// LED is connected between LED port of MCU and GND.
#define LED_INIT        (LED_PDIR |=  LED_PBIT) // Set LED PORT bit to output direction
#define LED_OFF         (LED_POUT &= ~LED_PBIT)
#define LED_ON          (LED_POUT |=  LED_PBIT)
#define LED_TOGGLE      (LED_POUT ^=  LED_PBIT)
#define LED_FLASH_TIME  20000
#define LED_FLASH_PAUSE 4

#define PWR_EN_AVAIL    0
// TODO: Remove the following temporary PWR_EN_ defines
#define PWR_EN_PSEL     P6SEL
#define PWR_EN_PDIR     P6DIR
#define PWR_EN_POUT     P6OUT
#define PWR_EN_PINP     P6INP
#define PWR_EN_PBIT     BIT3

#define PWR_EN_INIT     ((PWR_EN_PDIR) |=  (PWR_EN_PBIT)) // Set output direction
#define PWR_EN_OFF      ((PWR_EN_POUT) &= ~(PWR_EN_PBIT))
#define PWR_EN_ON       ((PWR_EN_POUT) |=  (PWR_EN_PBIT))

// Note: Pins for I2C are defined in swi2cmst.h

//----------------------------------------------------------------------------
// ORDB3A specific definitions
//----------------------------------------------------------------------------
#elif defined( ORDB3A )

//#define LED_AVAIL     0 // TODO: uncomment

// TODO: Remove the following temporary LED_ defines
#define LED_AVAIL       1
#define LED_PDIR        PJDIR
#define LED_POUT        PJOUT
#define LED_PBIT        BIT3

#define LED_INIT        (LED_PDIR |=  LED_PBIT) // Sets LED PORT bit to output direction
#define LED_OFF         (LED_POUT &= ~LED_PBIT)
#define LED_ON          (LED_POUT |=  LED_PBIT)
#define LED_TOGGLE      (LED_POUT ^=  LED_PBIT)
#define LED_FLASH_TIME  20000
#define LED_FLASH_PAUSE 4

#define PWR_EN_AVAIL    1
#define PWR_EN_PSEL     P6SEL
#define PWR_EN_PDIR     P6DIR
#define PWR_EN_POUT     P6OUT
#define PWR_EN_PINP     P6INP
#define PWR_EN_PBIT     BIT3

#define PWR_EN_INIT     ((PWR_EN_PDIR) |=  (PWR_EN_PBIT)) // Set output direction
#define PWR_EN_OFF      ((PWR_EN_POUT) &= ~(PWR_EN_PBIT))
#define PWR_EN_ON       ((PWR_EN_POUT) |=  (PWR_EN_PBIT))

// Note: Pins for I2C are defined in swi2cmst.h


#else
#error No board defined.
#endif

//----------------------------------------------------------------------------
// Board independent definitions
//----------------------------------------------------------------------------
#define OK              (TPS65217_OK)
#define ERROR           (TPS65217_ERROR)

#define LED_NUMFLASH_STARTED        1
#define LED_NUMFLASH_OK             2
#define LED_NUMFLASH_ENDED_OK       3
#define LED_NUMFLASH_ERROR          5
#define LED_NUMFLASH_ENDED_ERROR    6

#endif /* DEFS_H_ */
