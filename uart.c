#include <stdint.h>

#include "uart.h"
#include "safesleep.h"

#include <F5xx_F6xx_Core_Lib/HAL_PMAP.h>

#include "USB_API/USB_Common/types.h"
#include "USB_API/USB_CDC_API/UsbCdc.h"
#include <usbConstructs.h>
#include <descriptors.h>

#include <msp430.h>

static struct uart_state {
	// Not quite ring buffers. ?xsiz indicate fill level of buffers,
	// ?xo indicate consumed data level. 
	// Probably should convert to ring buffer behaviour.
	unsigned char rxbuf[64], txbuf[64];
	unsigned char rxsiz, txsiz;
	unsigned char rxo, txo;
	volatile unsigned char sending;
} state;

/*
 * This event indicates that a send operation on interface intfNum has just been completed.
 * returns TRUE to keep CPU awake
 */
BYTE USBCDC_handleSendCompleted (BYTE intfNum)
{
	state.sending=0;
	return (TRUE);   // Wake up
}

#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR (void)
{
	// Handle UART transfers
	switch (UCA1IV) {
	case 2: // Data received, store it in buffer
		if (state.rxsiz < sizeof state.rxbuf)
			state.rxbuf[state.rxsiz++] = UCA1RXBUF;
		if (state.rxsiz==sizeof state.rxbuf) {
			// Ran out of space, stop interrupt
			UCA1IE &= ~UCRXIE;
		}
		// Wake up!
		WAKEUP_IRQ(LPM3_bits);
		break;
	case 4: // Tx ready, send more from buffer
		if (state.txo<state.txsiz) {
			UCA1TXBUF = state.txbuf[state.txo++];
		}
		if (state.txo==state.txsiz) {
			state.txo=0;
			state.txsiz=0;
			// Sent all available data, stop interrupt
			UCA1IE &= ~UCTXIE;
			// Wake up in case USB layer has more data
			WAKEUP_IRQ(LPM3_bits);
		}
		break;
	}
}

void init_uart(void) {
	// Set USCI_A1 to 115200 8N1
	UCA1CTL1 |= UCSWRST;
	UCA1CTL0 = UCMODE_0;  // 8N1 uart mode
	UCA1CTL1 = BRCLK_SEL | UCSWRST;  // clock source per uart.h

	// for our case of 24MHz input, 115200 baud, we have an error of +0.16%(?)
	// when not using any modulation. start with that for now. 
	// Comparable baud rate setting from SLAU208:
	// 12MHz 115200baud UCOS16=1 UCBR=6 UCBRS=0 UCBRF=8  (???)
	// 12MHz 57600baud UCOS16=1 UCBR=13 UCBRS=0 UCBRF=0
	// We may get best results using 57600 settings
	UCA1BRW = ((BRCLK_FREQ+UART_BAUD*8)/UART_BAUD)/16;   // oversampling mode
	UCA1MCTL= UCOS16;  // Turns out modulation won't help at this combination
	//UCA1STAT= UCLISTEN;   // Loopback mode for initial testing
	// ignore IR registers for now
	UCA1IFG = /*UCTXIFG | UCRXIFG*/0; // TODO enable tx interrupt when data available
	UCA1CTL1 &=~UCSWRST;

	// Enable USCI_A1 UART pin functions
	// P4.4 = PM_UCA1TXD, P4.5 = PM_UCA1RXD
	// Not! Turns out they are swapped in the FPGA. Luckily we can easily swap too.
	const uint8_t p4_45_map[2] = {PM_UCA1RXD, PM_UCA1TXD};
	configure_ports(&p4_45_map, &P4MAP4, sizeof p4_45_map, 1);
	P4SEL |= BIT4 | BIT5;

	/* Debug: Make P4.6 another TX pin for oscilloscope using port mapper pmap */
	//unsigned char p4_6_map = PM_UCA1TXD;
	//configure_ports(&p4_6_map, &P4MAP6, 1, 1);
	//P4SEL |= BIT6;

	state.rxsiz=0;
	state.txsiz=0;
	state.txo=0;
	state.rxo=0;
	state.sending=0;

	UCA1IE = UCRXIE;   // enable rx interrupt
}

void handle_uart(void) {
	// Mark this whole function as a critical region; it interacts only with 
	// uart and usb interrupts. 
	// TODO: make this whole routine redundant by guaranteeing that the relevant
	// interrupts are called. 
	unsigned short bGIE  = (__get_SR_register() & GIE);
	__disable_interrupt();

	// receive tx chars from usb
	if (state.txsiz<sizeof state.txbuf) {
		// Main time waster
		state.txsiz+=cdcReceiveDataInBuffer(state.txbuf+state.txsiz,
						    sizeof state.txbuf-state.txsiz,
						    CDC0_INTFNUM);
		if (state.txsiz>state.txo)
			UCA1IE |= UCTXIE;
		else {
			state.txsiz=0;
			state.txo=0;
		}
		// Debug: USBCDC_sendData(state.txbuf, state.txsiz, CDC0_INTFNUM);
	}

	// Pass rx chars to USB
	if (state.rxsiz>state.rxo) {
		char result, len=state.rxsiz-state.rxo;
		state.sending=1;
		result=USBCDC_sendData(state.rxbuf+state.rxo, len, CDC0_INTFNUM);
		switch (result) {
			// Possible results: generalError, busNotAvailable, intfBusyError, sendStarted
		case kUSBCDC_sendComplete:   // can't happen
			state.rxo+=len;
			if (state.rxo==state.rxsiz) {
				// last data transmitted, reset buffer
				state.rxo=0;
				state.rxsiz=0;
			}
			break;
		case kUSBCDC_sendStarted:
			state.rxo+=len;
			break;
		case kUSBCDC_intfBusyError:
		default:
			/* Data not submitted, no effect to care about */;
		}
	} else {
		if (state.rxsiz==state.rxo && !state.sending) {
			// last data transmitted, reset buffer
			state.rxo=0;
			state.rxsiz=0;
		}
		UCA1IE |= UCRXIE;
	}
	__bis_SR_register(bGIE);
}

void uart_inject(unsigned char ch) {
	// Used for diagnostic processing, sends things on USB.
	unsigned short bGIE  = (__get_SR_register() & GIE);
	__disable_interrupt();
	if (state.rxsiz < sizeof state.rxbuf)
		state.rxbuf[state.rxsiz++] = ch;
	__bis_SR_register(bGIE);
}
