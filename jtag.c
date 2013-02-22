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
	P4OUT = (P4OUT&0b11110000)|0b0011;
	P4DIR = (P4DIR&0b11110000)|0b1011;
	P4REN &= 0b11110000;
	// P4DS &= 0b11110000;
	P4SEL &= 0b11110000;
	/* Could perhaps enable a pullup on TDO in case nothing is connected. */

	/* LED on PJ3 */
	PJDIR |= BIT3;

	/* Set up USCI to do large data shifts */
	UCB1CTL1 = UCSWRST;  // disable/reset for configuration
	/* UCCKPH=0, UCCKPL=0, LSB first, 8-bit, master, 3-pin SPI */
	UCB1CTL0 = UCMST | UCMODE_0 | UCSYNC;
	UCB1BRW = 1; /* maximum bit rate */
	//UCB1STAT = 0;
	UCB1IE = 0;
	UCB1CTL1 = UCSSEL1 | UCSWRST; // keep in reset until used
}

/* Shift up to 8 bits on both TMS and TDI, reading TDO.
   Expects GPIO configuration of jtag port. */
uint8_t jtag_shift_bits(uint8_t tdi, uint8_t tms, uint8_t len) {
	uint8_t tdo=0;

	while (len--) {
		/* Set output pins TMS and TDI */
		uint8_t mask=0xff, set=0x00;
		if (tdi&1)
			set |= BIT1;
		else
			mask &= ~BIT1;
		if (tms&1)
			set |= BIT0;
		else
			mask &= ~BIT0;
		P4OUT = (P4OUT&mask)|set;
		
		/* Raise clock */
		P4OUT |= BIT3;
		
		/* Read TDO */
		tdo = (tdo>>1) | (P4IN&BIT2?0x80:0x00);
		/* Shift the other registers */
		tdi >>= 1;
		tms >>= 1;
		/* Lower clock */
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
}

void jtag_shift_bytes_finish() {
	/* Await the transfer to finish then switch back to GPIO */
	while ((DMA0CTL|DMA1CTL) & DMAEN)
		/* Wait for DMA to finish */;
	jtag_spi_off();
}

void jtag_just_clock(uint16_t count) {
	while (count--) {
		/* Raise clock */
		P4OUT |= BIT3;
		/* Lower clock */
		P4OUT &= ~BIT3;
	}
}

// shift through a wide register and check one bit (e.g. CONF_DONE)
bool jtag_check_one_bit(int16_t total, int16_t ourbit) {
	bool bit=false;

	while (total>0) {
		uint8_t tdo;
		tdo = jtag_shift_bits(0x00, 0x00, total>8?8:total);
		if (ourbit>=0 && ourbit<8)
			bit = (tdo>>ourbit)&1;
		ourbit-=8;
		total-=8;
	}
	return bit;
}

#if 0
/* Routine (almost) replicating the SVF behaviour. Intend to replace this with 
   XSVF player/executor instead, to make the code less specialised. */
bool configure_cyclonev() {
	uint8_t *rbfdata;
	uint16_t rbflen;
	bool success=false;
	
	readrbf(&rbfdata, &rbflen);

	/* Reset JTAG bus */
	jtag_init();
	/* Move through JTAG state machine to test-logic-reset */
	jtag_shift_bits(0xff, 0xff, 8);   // STATE RESET;
	/* TODO: identify devices on chain */

	// SIR 10 TDI (002);
	/* move: (reset)-idle-seldr-selir-capir-shiftir */
	jtag_shift_bits(0xff, 0b00110, 5);
	/* Load instruction for configuration - 10'b0000000010 */
	jtag_shift_bits(0b00000010, 0b00000000, 8);
	jtag_shift_bits(0b1100, 0b0110, 4); // 2 bits, -exit1ir-updateir-run/idle

	// equivalent to RUNTEST IDLE 25000 TCK ENDSTATE IDLE;
	jtag_just_clock(25000);  // allow fpga to reset itself before configuration

	// Load bitstream via DR
	// (idle)-seldr-capdr-shiftdr
	jtag_shift_bits(0xff, 0b001, 3);  // Move into shift-DR

	while (rbflen) {
		// TODO: double-buffer so we can read while working
		jtag_shift_bytes_start(rbfdata,rbfdata,rbflen);
		jtag_shift_bytes_finish();
		readrbf(&rbfdata, &rbflen);
	}

	// TODO: runtest uses the last state specified in runtest, not the current state.
	// Also, check RBF vs jtag bitstream format. Probably identical when uncompressed..
	// but certainly backwards in SVF. Also, bitswapping occurs for rbf to spi flash.
	// is bit order preserved?
	// Sample RBF (compressed, so not good example): 
	//     ..11111011010101111011111110111 (msb first)
	// or  ..11111010101101110111111101111 (lsb first)
	// SVF ..11111010101101111111111111111  Quite possibly LSB first sync pattern+compr flag.
	// Length of initial 1s stream differs.

	// Check CONF_DONE via internal scan to verify bitstream
	// SIR 10 TDI (004);  (shiftdr)-1exit1dr-1updatedr-1seldr-1selir-0capir-0shiftir
	jtag_shift_bits(0xff, 0b001111, 6);  // move into shift-IR
	jtag_shift_bits(0x04, 0x00, 8);  // low 8 bits of IR
	jtag_shift_bits(0x00, 0b010, 3);  // move to IRPAUSE (per ENDIR IRPAUSE);
	jtag_just_clock(125);  // RUNTEST 125 TCK;

	// SDR widh masked tdo check
	jtag_shift_bits(0xff, 0b00111, 5);  // (IRPAUSE)-1exit2ir-1updateir-1seldr-0capdr-0shiftdr
	success=jtag_check_one_bit(732,286);  // bit 286 for EP4CE22 .. need to update for Cyclone V
	
	// Finally, instructions 3 and 3ff are shifted in to run user mode and set to bypass
	// (shiftdr)-1exit1dr-1updatedr-1seldr-1selir-0capir-0shiftir
	jtag_shift_bits(0xff, 0b001111, 6);  // move to shiftir
	jtag_shift_bits(0x03, 0x00, 8);    // instruction 10'h003, low 8 bits
	jtag_shift_bits(0x00, 0b10, 2);    // high 2 bits... FIXME: not finished.
	// May never be, as XSVF would be a cleaner choice.
}
#endif

struct usbblaster_state /* {
	//uint8_t *bytes_in, *bytes_out;
	uint8_t bytes_to_shift, read;
	} */ usb_jtag_state = {.bytes_to_shift = 0};

// TODO: optimize with SPI port
// This implements a lot of the usb blaster protocol (cpld side). 
// The return value should be send to host if .read unless .bytes_to_shift went from 0 to non-0
uint8_t usbblaster_byte(uint8_t fromhost) {
	if (usb_jtag_state.bytes_to_shift) {
		usb_jtag_state.bytes_to_shift--;
		return jtag_shift_bits(P4OUT&BIT0?0xff:0x00, fromhost, 8);
	} else {
		usb_jtag_state.read = fromhost&0x40;
		if (fromhost&0x80) {
			usb_jtag_state.bytes_to_shift = fromhost&0x3f;
			return 0;
		} else {
			uint8_t mask=0xff, set=0x00;
			if (fromhost & BIT0)  // TCK
				set |= BIT3;
			else
				mask &= ~BIT3;
			if (fromhost & BIT1)  // TMS
				set |= BIT0;
			else
				mask &= ~BIT0;
			if (fromhost & BIT1)  // TDI
				set |= BIT1;
			else
				mask &= ~BIT1;

			P4OUT = (P4OUT&mask)|set;

			if (fromhost & BIT5)	// LED
				PJOUT |= BIT3;
			else
				PJOUT &= ~BIT3;

			return P4IN&BIT2?1:0;  // TDO
		}
	}
}
