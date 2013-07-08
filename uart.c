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
    //TO DO: You can place your code here
	state.sending=0;
    return (TRUE);                             //return FALSE to go asleep after interrupt (in the case the CPU slept before
                                                //interrupt)
}


void init_uart(void) {
	unsigned char p4_6_map = PM_UCA1TXD;
	// Set USCI_A1 to 115200 8N1
	UCA1CTL1 |= UCSWRST;
	UCA1CTL0 = UCMODE_0;  // 8N1 uart mode
	UCA1CTL1 = BRCLK_SEL | UCSWRST;  // clock source per uart.h

	// for our case of 24MHz input, 115200 baud, we have an error of +0.16%(?)
	// when not using any modulation. start with that for now. 
	// Comparable baud rate setting from SLAU208:
	// 12MHz 115200baud UCOS16=1 UCBR=6 UCBRS=0 UCBRF=8  (???)
	UCA1BRW = ((BRCLK_FREQ+UART_BAUD/2)/UART_BAUD)/16;   // oversampling mode
	UCA1MCTL= 0;  // TODO: figure out how modulation really affects things and choose a modulated baud rate
	UCA1STAT= UCLISTEN;   // Loopback mode for initial testing
	// ignore IR registers for now
	UCA1IFG = /*UCTXIFG | UCRXIFG*/0; // TODO enable tx interrupt when data available
	UCA1CTL1 &=~UCSWRST;

	// Enable USCI_A1 UART pin functions
	// P4.4 = PM_UCA1TXD, P4.5 = PM_UCA1RXD
	P4SEL |= BIT4 | BIT5;

	/* Debug: Make P4.6 another TX pin for oscilloscope using port mapper pmap */
	configure_ports(&p4_6_map, &P4MAP6, 1, 1);
	P4SEL |= BIT6;

	state.rxsiz=0;
	state.txsiz=0;
	state.txo=0;
	state.rxo=0;
	state.sending=0;
}

void handle_uart(void) {
	stay_awake();   // Temporary: until UART wakes main thread up. 
	// receive tx chars from usb
	if (state.txsiz<sizeof state.txbuf) {
		state.txsiz+=cdcReceiveDataInBuffer(state.txbuf+state.txsiz,
						    sizeof state.txbuf-state.txsiz,
						    CDC0_INTFNUM);
		// Debug: USBCDC_sendData(state.txbuf, state.txsiz, CDC0_INTFNUM);
	}
	// transmit a tx char to uart
	if (state.txo<state.txsiz) {
#if 1
		if (UCA1IFG&UCTXIFG) {   // uart ready
			UCA1TXBUF = state.txbuf[state.txo++];
			if (state.txo==state.txsiz) {
				state.txo=0;
				state.txsiz=0;
			}
		} else {  // want to send, but uart busy (TODO: interrupt)
			stay_awake();
		}
#else
		if (state.rxsiz<sizeof state.rxbuf) {
			// Software loopback (strictly for testing)
			state.rxbuf[state.rxsiz++] = state.txbuf[state.txo++];;
			if (state.txo==state.txsiz) {
				state.txo=0;
				state.txsiz=0;
			}
		} else {  // want to send, but uart busy (TODO: interrupt)
			stay_awake();
		}
#endif
	}
	// Receive chars from uart - TODO: wakeup
	if (UCA1IFG&UCRXIFG) {
		if (state.rxsiz<sizeof state.rxbuf) {
			state.rxbuf[state.rxsiz++] = UCA1RXBUF;
		}
	}
	// Pass chars to USB
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
	} else if (state.rxsiz) {
		// All data has been submitted to USB, check if done
		//WORD bytesSent, bytesReceived;
		//BYTE ret = USBCDC_intfStatus(CDC0_INTFNUM,&bytesSent,&bytesReceived);
		//if (!(ret & kUSBCDC_waitingForSend)) {
		if (!state.sending) {
			// last data transmitted, reset buffer
			state.rxo=0;
			state.rxsiz=0;
		}
	}
}
