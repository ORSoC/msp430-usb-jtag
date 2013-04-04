#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>

#include "jtag.h"

/* Support functions for JTAG via gpio and USCI SPI
   Intended for ORSoC Cyclone V board. Since the Cyclone V is capable of
   quite high JTAG clock speed, no limiting is implemented.
   Suggest setting SMCLK to top speed (24MHz is readily available) */

/* As per IEEE 1149.1:
   TCK may pause indefinitely when 0
   TMS is sampled at rising TCK and reads 1 if not driven
   TDI is sampled at rising TCK and reads 1 if not driven
   TDO changes only on falling TCK

   In recent spec, TDO also shall go inactive (tristate) when not in use.
 */

/* On our msp438f5507rgz, jtag is connected to UCB1:
   TCK on P4.3 UCB1CLK
   TDO on P4.2 UCB1SOMI
   TDI on P4.1 UCB1SIMO
   TMS on P4.0 (UCB1STE)

   Other P4 pins are used as GPIO and UART, do not change them.
 */

/* As per usual, TI software engineers simply made a mess of things. I will
   not even use their GPIO code because it requires undocumented and 
   inconsistent arguments to work at all for other ports than PA. */

/* As used by OpenOCD, JTAG shifts are done:
   JTAG_MODE (LSB_FIRST | POS_EDGE_IN | NEG_EDGE_OUT) */

/* TODO:
   1. Implement USB Blaster protocol - this means get USB stack working.
      Example code from ixo-jtag may help.
   2. Implement an XSVF player for flash based configuration. Example 
      code available from libxsvf, xsvfexec or bus pirate. 
*/

/* USB Blaster:
   Protocol described in ixo-jtag/usb_jtag/trunk/device/c51/usbjtag.c
   The two header bytes for incoming packets are modem line status, per libftdi:
    0.0-0.3  always 0 per libftdi, always 1 per linux kernel!
    0.4      CTS
    0.5      DTS
    0.6      RI
    0.7      RLSD (receive line signal detect)
    1.0      data ready
    1.1      overrun error
    1.2      parity error
    1.3      framing error
    1.4      break interrupt
    1.5      transmitter holding register
    1.6      transmitter empty
    1.7      error in receiver fifo
   usbjtag.c uses 0x31 0x60, which indicates CTS and DTS high,
   transmitter empty and THRE. This is nonsense. Find the class docs. 
   (Looks like the FTDI chip is vendor specific in our descriptors!)
   Also, a status packet is generated every 40ms by ftdi chips, even if no data.
   Decide to do this based on USB heartbeat.

   Summary of Blaster protocol:
   Bit 7 selects byte mode: In this case, bits 5:0 select how many bytes to shift.
   Bit 6 selects read mode, i.e. enables sending result back. Result is {dataout,tdo}
   in bit mode, data bits in byte mode. 
   Bit 5 sets OE/LED active
   Bit 4 sets TDI
   Bit 3 sets nCS
   Bit 2 sets nCE
   Bit 1 sets TMS/nCONFIG
   Bit 0 sets TCK/DCLK
 */

void jtag_init() {
	/* We need to fall back to GPIO for bit or TMS transfers */
#if OLIMEXINO_5510
	PJOUT |= BIT0;   // TMS on #UEXT_CS PJ.0
	PJDIR |= BIT0 | BIT3;  // LED on PJ.3
	P4OUT = (P4OUT&0b11110001)|0b0010;
	P4DIR = (P4DIR&0b11110001)|0b1010;
	P4REN &= 0b11110001;
	// P4DS &= 0b11110000;
	P4SEL &= 0b11110001;
#elif ORDB3A
	P4OUT = (P4OUT&0b11110000)|0b0011;
	P4DIR = (P4DIR&0b11110000)|0b1011;
	P4REN &= 0b11110000;
	// P4DS &= 0b11110000;
	P4SEL &= 0b11110000;
#else
#error Unknown board!
#endif
	/* Could perhaps enable a pullup on TDO in case nothing is connected. */

	/* Set up USCI to do large data shifts */
	UCB1CTL1 = UCSWRST;  // disable/reset for configuration
	/* LSB first, 8-bit, master, 3-pin SPI */
	UCB1CTL0 = UCCKPH | UCMST | UCMODE_0 | UCSYNC;
	UCB1BRW = 1; /* maximum bit rate */
	//UCB1STAT = 0;
	UCB1IE = 0;
	UCB1CTL1 = UCSSEL1 | UCSWRST; // keep in reset until used

	usb_jtag_state.bytes_to_shift = 0;
	usb_jtag_state.read = 0;
}

/* Shift up to 8 bits on both TMS and TDI, reading TDO.
   Expects GPIO configuration of jtag port. */
uint8_t jtag_shift_bits(uint8_t tdi, uint8_t tms, uint8_t len) {
	uint8_t tdo=0;

	// Note: this routine handles both TMS and TDI sequences.
	// UsbBlaster protocol does not alter TMS during byte shifts.
	while (len--) {
		/* Set output pins TMS and TDI */
		uint8_t mask=0xff, set=0x00;
		if (tdi&1)
			set |= BIT1;
		else
			mask &= ~BIT1;
#if ORDB3A
		if (tms&1)
			set |= BIT0;
		else
			mask &= ~BIT0;
#elif OLIMEXINO_5510
		if (tms&1)
			PJOUT |= BIT0;
		else
			PJOUT &= ~BIT0;
#else
#error Unknown board!
#endif
		P4OUT = (P4OUT&mask)|set;	// Set TMS and TDI
		tdo = (tdo>>1) | (P4IN&BIT2?0x80:0x00);	// read TDO
		
		/* Raise TCK */
		P4OUT |= BIT3;
		
		/* Shift the other registers */
		tdi >>= 1;
		tms >>= 1;
		/* Lower TCK */
		P4OUT &= ~BIT3;
	}
	return tdo;
}

void jtag_spi_on(void) {
	/* Enable SPI function */
	UCB1CTL1 = UCSSEL1;  // releases reset
	P4SEL |= 0b00001110; // map pins to SPI
}
void jtag_spi_off(void) {
	/* Disable SPI function */
	P4SEL &= ~0b00001110; // revert pins to GPIO
	UCB1CTL1 = UCSSEL1 | UCSWRST;  // Halt SPI unit
}

/* Shift large amounts of data at top speed, uses SPI and DMA functions
   Uses DMA1 for Tx and DMA0 for Rx, both set up here
   Buffers may be the same, but must exist
 */
void jtag_shift_bytes_start(const uint8_t *bytes_out, uint8_t *bytes_in, uint16_t len) {
	jtag_spi_on();
#define ERRATUM_DMA10 1
#if ERRATUM_DMA10
		// Avoiding DMA mode for the moment because of erratum DMA10
		// Accessing USB module becomes unreliable because DMA unit can break transfers
	while (len--) {
		UCB1TXBUF=*bytes_out++;
		while (!(UCB1IFG & UCRXIFG))
			/* wait */;
		*bytes_in++=UCB1RXBUF;
	}
#else
	/* Set DMA up to feed SPI */
	DMA0CTL = DMA1CTL = 0;  // disable both channels
	DMACTL0 = DMA1TSEL__USCIB1TX | DMA0TSEL__USCIB1RX;  // Trigger source for both DMAs
	DMA1SA = (uintptr_t)bytes_out;
	DMA1DA = (uintptr_t)&UCB1TXBUF;
	DMA1SZ = len;
	DMA0SA = (uintptr_t)&UCB1RXBUF;
	DMA0DA = (uintptr_t)bytes_in;
	DMA0SZ = len;
	// set Rx up before Tx
	DMA0CTL = DMADT_0 | DMADSTINCR_3 | DMADSTBYTE | DMASRCBYTE | DMALEVEL |
		/* DMAIE | */ DMAEN;
	DMA1CTL = DMADT_0 | DMASRCINCR_3 | DMADSTBYTE | DMASRCBYTE | DMALEVEL | DMAEN;

	/* DMA channels auto-disable when done, but P4SEL and UCB1CTL1 need to be reset */
	/* Could enable an interrupt from DMA0 to report finish */
#endif
}

void jtag_shift_bytes_finish() {
#ifndef ERRATUM_DMA10
	/* Await the transfer to finish then switch back to GPIO */
	while ((DMA0CTL|DMA1CTL) & DMAEN) {
		/* Wait for DMA to finish */;
	}
#endif
	jtag_spi_off();
}

struct usbblaster_state /* {
	//uint8_t *bytes_in, *bytes_out;
	uint8_t bytes_to_shift, read;
	} */ usb_jtag_state = {.bytes_to_shift = 0};

// TODO: optimize with SPI port
// This implements a lot of the usb blaster protocol (cpld side). 
// The return value should be send to host if .read unless .bytes_to_shift went from 0 to non-0
uint8_t usbblaster_byte(uint8_t fromhost) {
	if (usb_jtag_state.bytes_to_shift) {
		uint8_t tms;
		usb_jtag_state.bytes_to_shift--;
#if ORDB3A
		tms = P4OUT&BIT0;
#elif OLIMEXINO_5510
		tms = PJOUT&BIT0;
#else
#error Unknown board!
#endif
		return jtag_shift_bits(fromhost, tms?0xff:0x00, 8);
	} else {
		usb_jtag_state.read = fromhost&0x40;
		if (fromhost&0x80) {
			usb_jtag_state.bytes_to_shift = fromhost&0x3f;
			return 0;
		} else {
			uint8_t mask=0xff, set=0x00, tdo;
			if (fromhost & BIT0)  // TCK
				set |= BIT3;
			else
				mask &= ~BIT3;
			if (fromhost & BIT4)  // TDI
				set |= BIT1;
			else
				mask &= ~BIT1;
			tdo = P4IN&BIT2;    // Simultaneous in real device, we'll just read before write
#if OLIMEXINO_5510
			if (fromhost & BIT1)  // TMS
				PJOUT |= BIT0;
			else
				PJOUT &= ~BIT0;
#elif ORDB3A
			if (fromhost & BIT1)  // TMS
				set |= BIT0;
			else
				mask &= ~BIT0;
#else
#error Unknown board!
#endif

			P4OUT = (P4OUT&mask)|set;

#if OLIMEXINO_5510
			if (fromhost & BIT5)	// LED
				PJOUT |= BIT3;
			else
				PJOUT &= ~BIT3;
#endif

			return tdo?1:0;  // TDO
		}
	}
}

/* Process data in a buffer. Returns the size of data that should be sent back to host. */
/* TODO: extend for fully asynchronous operation. */
int usbblaster_process_buffer(uint8_t *buf, int len) {
	int i=0, o=0;

	while (i<len) {
		uint8_t bts=usb_jtag_state.bytes_to_shift, ret;

		if (bts==0) {  // bitbang / command byte
			ret=usbblaster_byte(buf[i++]);
			if (usb_jtag_state.read &&
			    !(usb_jtag_state.bytes_to_shift && !bts)) {
				buf[o++]=ret;
			}
		} else {  // Byte shift mode, use the SPI with DMA
			ret=len-i>bts ? bts : len-i;  // Size of data that can be shifted directly
			jtag_shift_bytes_start(buf+i, buf+o, ret);
			jtag_shift_bytes_finish();
			if (usb_jtag_state.read)
				o+=ret;
			i+=ret;
			usb_jtag_state.bytes_to_shift-=ret;
		}
	}

	return o;
}

int pulse_tck(struct libxsvf_host *h, int tms, int tdi, int tdo, int rmask, int sync) {
	int line_tdo;
	if (tms)
		P4OUT |= BIT0;
	else
		P4OUT &=~BIT0;
	if (tdi)	/* may be -1 for don't care */
		P4OUT |= BIT1;
	else
		P4OUT &=~BIT1;
	/* Pulse TCK low (this is odd, as it idles low in spec) */
	P4OUT &= ~BIT3;
	P4OUT |= BIT3;
	line_tdo = (P4IN&BIT2)>>2;
	return tdo < 0 || line_tdo == tdo ? line_tdo : -1;
}

