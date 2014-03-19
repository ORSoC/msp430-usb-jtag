/* --COPYRIGHT--,BSD
 * Copyright (c) 2012, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
/*  
 * ======== main.c ========
 * Originally based on demo H1 from TI MSP430 USB API package
 * Heavily modified by ORSoC, to emulate FTDI chip, Altera USB Blaster and
 * support bootloader function (that's in descriptors.c)
 *
 +----------------------------------------------------------------------------+
 * Please refer to the MSP430 USB API Stack Programmer's Guide,located
 * in the root directory of this installation for more details.
 *----------------------------------------------------------------------------*/
#include <intrinsics.h>
#include <string.h>

#include "USB_config/descriptors.h"

#include "USB_API/USB_Common/device.h"
#include "USB_API/USB_Common/types.h"               //Basic Type declarations
#include "USB_API/USB_Common/usb.h"                 //USB-specific functions

#include "F5xx_F6xx_Core_Lib/HAL_UCS.h"
#include "F5xx_F6xx_Core_Lib/HAL_PMM.h"

#include "USB_API/USB_HID_API/UsbHid.h"
#include "USB_API/USB_CDC_API/UsbCdc.h"

#include "usbConstructs.h"

#include "defs.h"
#include "jtag.h"
#include "safesleep.h"
#include "nand_ordb3.h"
#include "uart.h"

extern unsigned int boardInit(void);
extern void fpga_powerdown(void);
extern void fpga_powerup(void);
extern int check_pushbutton(void);

VOID Init_Ports (VOID);
VOID Init_Clock (VOID);
VOID Init_TimerA1 (VOID);
static void inline Reset_TimerA1(void) {
	TA1CTL = TASSEL__ACLK + ID__2 + TACLR + MC__UP;                              //ACLK/1, clear TAR
}
BYTE retInString (char* string);

volatile BYTE bHIDDataReceived_event = FALSE;   //Indicates data has been received without an open rcv operation
volatile BYTE bTimerTripped_event = FALSE, bCommand = 0;

#define MAX_STR_LENGTH 64

unsigned int SlowToggle_Period = 20000 - 1;
unsigned int FastToggle_Period = 1000 - 1;

/*  
 * ======== main ========
 */
extern void Do_NAND_Probe(void);
int main (VOID)
{
	char fpga_powered=0;
    WDTCTL = WDTPW + WDTHOLD;                                   //Stop watchdog timer

    Init_Ports();                                               //Init ports (do first ports because clocks do change ports)
    fpga_powered=1;
    SetVCore(3);	// Do this before accessing NAND but after talking to PM chip on I²C
    Init_Clock();                                               //Init clocks

    Do_NAND_Probe();	// Initializes NAND and enables ECC

    // Configure FPGA
    program_fpga_from_nand();

    USB_init();                                 //init USB
    Init_TimerA1();
    init_uart();

    //Enable various USB event handling routines
    USB_setEnabledEvents(
        kUSB_VbusOnEvent + kUSB_VbusOffEvent + kUSB_receiveCompletedEvent
        + kUSB_dataReceivedEvent + kUSB_UsbSuspendEvent + kUSB_UsbResumeEvent +
        kUSB_UsbResetEvent + kUSB_sendCompletedEvent);

    //See if we're already attached physically to USB, and if so, connect to it
    //Normally applications don't invoke the event handlers, but this is an exception.
    if (USB_connectionInfo() & kUSB_vbusPresent){
        USB_handleVbusOnEvent();
    }

    init_sleep(LPM0_bits);
    __enable_interrupt();                           //Enable interrupts globally
    while (1)
    {
        // Check for commands from FPGA
        uint8_t gotcommand = bCommand;
	if (gotcommand & 0x80) {
		bCommand = gotcommand & 0x7f;
		// Handle command byte (7bit)
		// Example thing to do: send I2C command to run power down sequence
		// (Should reprogram power down sequence not to kill msp430 for wakeup)
		uart_inject(bCommand);
	}
        //Check the USB state and directly main loop accordingly
        switch (USB_connectionState())
        {
            case ST_USB_DISCONNECTED:
		    set_sleep_mode(LPM3_bits);
		    enter_sleep();
                break;

            case ST_USB_CONNECTED_NO_ENUM:
                break;

            case ST_ENUM_ACTIVE:
		    if (!fpga_powered) {
			    fpga_powerup();
			    fpga_powered=1;
		    }
		    set_sleep_mode(0);
		    enter_sleep();

                                                                                    //Exit LPM on USB receive and perform a receive
                                                                                    //operation
                if (bHIDDataReceived_event){                                        //Some data is in the buffer; begin receiving a
                                                                                    //command
		    int i, o, len;
                    unsigned char pieceOfString[MAX_STR_LENGTH] = "";                        //Holds the new addition to the string
                    bHIDDataReceived_event = FALSE;  // Must be before receive

		    do {

			    len=hidReceiveDataInBuffer(pieceOfString,
						       MAX_STR_LENGTH,
						       HID0_INTFNUM);               //Get the next piece of the string

			    o = usbblaster_process_buffer(pieceOfString, len);

			    if (o) {
				    Reset_TimerA1();   // No need, the new version will only send if USB has buffer free
				    // However, either of these will do partial sends happily so we're
				    // not sending at optimal bitrate. 
				    hidSendDataWaitTilDone(pieceOfString,o,HID0_INTFNUM,0);
			    }
		    } while (len);
                }

		/* Flash interface handling */
		if (nand_ready()) {
			int len;
			char buf[MAX_STR_LENGTH];
			if (expect_nandreq()) {
				len=USBHID_bytesInUSBBuffer(FLASH_INTFNUM);
				if (len>=sizeof(struct nandreq)) {
					/* Unlock flash */
					nand_enable_write();  // raise WPn
					hidReceiveDataInBuffer((BYTE*)&nand_state, sizeof(struct nandreq),
							       FLASH_INTFNUM);
					// Simple validation for now
					if (nand_state.addr_bytes<8 &&
					    !(nand_state.writelen&&nand_state.readlen)) {
						process_nandreq();
					} else {
						// Invalid command, flush the buffer
						hidReceiveDataInBuffer((BYTE*)buf, len, FLASH_INTFNUM);
						nand_state.addr_bytes=0;
						nand_state.writelen=0;
						nand_state.readlen=0;
					}
					stay_awake();  // So we may process further data
				} else if (len) {
					// Too small a packet, discard the data
					hidReceiveDataInBuffer((BYTE*)buf, len, FLASH_INTFNUM);
					stay_awake();
				}
			} else if ((len=expect_nanddata())) {  // Yes, this is an assignment
				len=hidReceiveDataInBuffer((BYTE*)buf,
							   sizeof(buf)<len?sizeof(buf):len,
							   FLASH_INTFNUM);
				if (len) {
					int origwrlen=nand_state.writelen;
					process_nanddata(buf, len);
#if 0
					if (origwrlen!=nand_state.writelen) {
						if (nand_state.writelen)
							USBHID_sendData((BYTE*)&nand_state.writelen,
									2,FLASH_INTFNUM);
						else {
							BYTE sendstate;
							do {
								// Retry until the 0 word gets through
								sendstate=hidSendDataWaitTilDone((BYTE*)&nand_state.writelen,
												 2,FLASH_INTFNUM,0);
							} while (sendstate==3);
						}
					}
#endif
					stay_awake();
				}
			} else if (nand_state.readlen) {
				/* FIXME now that we have multiple interfaces, we must not 
				   spin on one of them like this... */
				len=produce_nanddata(buf, sizeof(buf)<62?sizeof(buf):62);
				hidSendDataWaitTilDone((BYTE*)buf,len,FLASH_INTFNUM,0);
				stay_awake();
			}
		}
		if (nand_state.readlen || USBHID_bytesInUSBBuffer(FLASH_INTFNUM)) {
			stay_awake();  /* Waiting for NAND, don't sleep */
		}

		handle_uart();

		if (bTimerTripped_event) {  // Be sure to send status bytes now and then (16ms)
#if 0
			static unsigned char cdc_data_buf[4]="00\r\n";
			unsigned char fill=USBCDC_bytesInUSBBuffer(CDC0_INTFNUM);
			cdc_data_buf[1]=(fill&0x0f) + '0';
			cdc_data_buf[0]=(fill>>4) + '0';
			if (cdc_data_buf[1] > '9') cdc_data_buf[1] += 'a'-'0';
			if (cdc_data_buf[0] > '9') cdc_data_buf[0] += 'a'-'0';
			USBCDC_sendData(cdc_data_buf, sizeof cdc_data_buf, CDC0_INTFNUM); // Worked!
#endif

			bTimerTripped_event = FALSE;
			/* We don't care if this send fails, because that can only mean it wasn't needed
			   (data already on its way, or USB disconnected) */
			USBHID_sendData(0,0,HID0_INTFNUM);
			USBHID_sendData(0,0,FLASH_INTFNUM);
		}

                break;

            case ST_ENUM_SUSPENDED:
                LED_OFF;                                                     //When suspended, turn off LED
		if (fpga_powered) {
			fpga_powerdown();
			fpga_powered=0;
		}
		set_sleep_mode(LPM0_bits);
		enter_sleep();
                break;

            case ST_ENUM_IN_PROGRESS:
		    //fpga_powerup();
		    //fpga_powered=1;
                break;

            case ST_NOENUM_SUSPENDED:
		set_sleep_mode(LPM0_bits);
		enter_sleep();
                break;

            case ST_ERROR:
                _NOP();
                break;

            default:;
        }
    }  //while(1)
    return 0;
} //main()

/*  
 * ======== Init_Clock ========
 */
VOID Init_Clock (VOID)
{
    //Initialization of clock module
    if (USB_PLL_XT == 2){
#if OLIMEXINO_5510
#if defined (__MSP430F552x) || defined (__MSP430F550x)
	P5SEL |= 0x0C;                                      //enable XT2 pins for F5529
#elif defined (__MSP430F563x_F663x)
	P7SEL |= 0x0C;
#endif

	//use REFO for FLL and ACLK
	UCSCTL3 = (UCSCTL3 & ~(SELREF_7)) | (SELREF__REFOCLK);
	UCSCTL4 = (UCSCTL4 & ~(SELA_7)) | (SELA__REFOCLK);
	
	//MCLK will be driven by the FLL (not by XT2), referenced to the REFO
	Init_FLL_Settle(USB_MCLK_FREQ / 1000, USB_MCLK_FREQ / 32768);   //Start the FLL, at the freq indicated by the config
                                                                        //constant USB_MCLK_FREQ
        XT2_Start(XT2DRIVE_0);                                          //Start the "USB crystal"

#elif ORDB3A

	P5SEL |= BIT2 | BIT4;	// External oscillators, we do not need outputs

	// Default state is clock pins disabled, SM from DCOCLKDIV, REF from XT1
	// That will fail as it runs the FLL but doesn't feed it a clock
	//XT1_Bypass();   // Set up XT1, so it won't keep failing
	// Alternative to not require XT1: reconfigure clock sources to internal
	UCSCTL4 = SELA__REFOCLK | SELS__REFOCLK | SELM__REFOCLK;  // very slow
	XT2_Bypass();   // Enable XT2 oscillator bypass
	// All clocks based on XT2
	UCSCTL4 = SELA__XT2CLK | SELS__XT2CLK | SELM__XT2CLK;
#else
# error unknown board
#endif
    } 
	else {
		#if defined (__MSP430F552x) || defined (__MSP430F550x)
			P5SEL |= 0x10;                                      //enable XT1 pins
		#endif
        //Use the REFO oscillator to source the FLL and ACLK
        UCSCTL3 = SELREF__REFOCLK;
        UCSCTL4 = (UCSCTL4 & ~(SELA_7)) | (SELA__REFOCLK);

        //MCLK will be driven by the FLL (not by XT2), referenced to the REFO
        Init_FLL_Settle(USB_MCLK_FREQ / 1000, USB_MCLK_FREQ / 32768);   //set FLL (DCOCLK)

        XT1_Start(XT1DRIVE_0);                                          //Start the "USB crystal"
    }
}

/*  
 * ======== Init_Ports ========
 */
VOID Init_Ports (VOID)
{
    // Set up voltages using external PM chip
    boardInit();

    //Initialization of ports
#if OLIMEXINO_5510
    // P1 goes to Arduino D2..D9. Unused pins are set output low.
    P1OUT = 0x00;	
    P1DIR = 0xFF;
    // P2 has only a button; enable it
    P2OUT = 1;
    P2DIR = 0;
    P2REN = 1;
    // P3 is absent
    //P3OUT = 0x00;
    //P3DIR = 0xFF;
    // P4 has JTAG (1..3), UART (4,5) and I²C (6,7)
    // Also connects to arduino D0,D1,D10..D13
    P4OUT = 0b00110010;
    P4DIR = 0b00011011;
    // P5 has 6 pins, 4 used for crystals and 2 arduino A5,A4
    P5OUT = 0x00;
    P5DIR = 0xFF;
    // P6 has 4 pins, A3..A0, also connected for battery measure on A3
    P6OUT = 0x00;
    P6DIR = 0xFF;
    // PJ has LED1, BAT_SENSE_E, UEXT_PWR_E and #UEXT_CS (used for TMS)
    PJOUT = 0b0001;
    PJDIR = 0b1111;
    jtag_init();
    // Make sure our UEXT pins don't burn things, hopefully
    P4DS = 0;
    PJDS = 0;
#elif ORDB3A
    // P1 is NAND data
    P1OUT = 0x00;	
    P1DIR = 0x00;
    P1REN = 0xff;
    // P2 has wifi interrupt
    P2OUT = 1;
    P2DIR = 0;
    P2REN = 1;
    // P3 is absent
    //P3OUT = 0x00;
    //P3DIR = 0xFF;
    // P4 has GPS_RESET_N LNA_EN RXD TXD TCK TDO TDI TMS
    P4OUT = 0b11110110;
    P4DIR = 0b00011011;
    P4REN = 0b11100100;
    // P5 has 6 pins, (xout) xin (xt2out) xt2in CLE CEn
    P5OUT = 0xff;
    P5DIR = 0x00;
    P5REN = 0b101011;
    // P6 has 4 pins, REn PWR_EN SCL SDA
    P6OUT = 0b1111;
    P6REN = 0b1111;
    P6DIR = 0b0100;
    // PJ has R/BYn WP ALE WEn
    PJOUT = 0b1001;
    PJDIR = 0b0100;
    PJREN = 0b1111;
    jtag_init();
#else
#error Unknown board!
#endif
}

/*  
 * ======== UNMI_ISR ========
 */
#pragma vector = UNMI_VECTOR
__interrupt VOID UNMI_ISR (VOID)
{
    switch (__even_in_range(SYSUNIV, SYSUNIV_BUSIFG))
    {
        case SYSUNIV_NONE:
            __no_operation();
            break;
        case SYSUNIV_NMIIFG:
            __no_operation();
            break;
        case SYSUNIV_OFIFG:
            UCSCTL7 &= ~(DCOFFG + XT1LFOFFG + XT2OFFG); //Clear OSC flaut Flags fault flags
            SFRIFG1 &= ~OFIFG;                          //Clear OFIFG fault flag
            break;
        case SYSUNIV_ACCVIFG:
            __no_operation();
            break;
        case SYSUNIV_BUSIFG:
                                                    //If bus error occured - the cleaning of flag and re-initializing of USB is
                                                    //required.
            SYSBERRIV = 0;                          //clear bus error flag
            USB_disable();                          //Disable
    }
}

/*  
 * ======== Init_TimerA1 ========
 */
VOID Init_TimerA1 (VOID)
{
	// Count at ACLK/2 = 32768Hz/2 = 16384Hz
	// Chosen such that we can use one RLAM to do *16 for latency timer
	TA1CCR0 = 16*(32768/1024/2);  // default timeout 16ms (roughly - 16384)
	TA1CCTL0 = CCIE;                                        //CCR0 interrupt enabled
	Reset_TimerA1();
}


/*  
 * ======== TIMER1_A0_ISR ========
 */
#pragma vector=TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR (void)
{
	//PJOUT ^= BIT3;                                          //Toggle LED P1.0
	bTimerTripped_event = TRUE;
	// Wake main thread up
	WAKEUP_IRQ(LPM3_bits);
    	 __no_operation();                       // Required for debugger
}

