/**
  Sleep routines for MSP430. Written because TI made incorrect assumptions 
  about how interrupts work; it is crucial for consistent reaction to events 
  that the main thread not go to sleep before processing events. 

  This code tries to accomplish that by making the "go to sleep" step an 
  atomic operation whose effects can be modified by interrupt routines. 

  The technique can be further optimized for some cases:
   1. If only one sleep mode is used, there's no need for SR_sleep_mode.
   2. SR_sleep could be stored in a register, if you can sacrifice it
      (in the main thread at least, possibly everywhere). 
   3. If the main thread does no work, this is all moot. Just set the 
      sleep mode and never wake things. 
*/

#include <intrinsics.h>
#include <msp430.h>

/* TODO: Figure out a safe way to select sleep mode from main thread.
   As it is, if exactly one event occurs, the next sleep will be based
   on previous sleep mode. */


volatile unsigned char SR_sleep, SR_sleep_mode;

/* Routine for interrupts to wake main thread *and* keep it from falling asleep */
/* This is a macro for __bic_status_register_on_exit */
#define WAKEUP_IRQ(sleepbits) do {				\
		SR_sleep = 0;					\
		__bic_status_register_on_exit(sleepbits);	\
	} while (0)

static inline void enter_sleep(void) {
	/* Ordinarily, we would use __bis_status_register(SR_sleep);
	   However, at least in mspgcc LTS 20120406 this can compile to load+or.
	   It is vital that this step be atomic, so we use asm. */
	asm volatile ("bis.b %0,r2;"::"m"(SR_sleep):"r2","cc");
	/* The next step does not need to be atomic, but does need to occur before 
	   anything we want to wake up for. Function+volatile ought to do it, but 
	   here's the assembly version just in case. */
	asm volatile ("mov.b %1,%0":"=m"(SR_sleep):"m"(SR_sleep_mode));
	//SR_sleep = SR_sleep_mode;
}

static inline void set_sleep_mode(uint8_t sleepmode) {
	if (SR_sleep!=sleepmode)
		SR_sleep=0;	// Make sure the next sleep isn't the wrong mode
	SR_sleep_mode = sleepmode;
}

static inline void init_sleep(uint8_t sleepmode) {
	set_sleep_mode(sleepmode);
	SR_sleep = SR_sleep_mode;
}
