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
 * H1 Example
 * LED Control Demo:
 *
 * This USB demo example is to be used with a PC application (e.g. HID App.exe)
 * This demo application is used to control the operation of the LED at P1.0
 *
 * Typing the following pharses in the HyperTerminal Window does the following
 * 1. "LED ON!" Turns on the LED and returns "LED is ON" phrase to PC
 * 2. "LED OFF!" Turns off the LED and returns "LED is OFF" back to HyperTerminal
 * 3. "LED TOGGLE - SLOW!" Turns on the timer used to toggle LED with a large
 *   period and returns "LED is toggling slowly" phrase back to HyperTerminal
 * 4. "LED TOGGLE - FAST!" Turns on the timer used to toggle LED with a smaller
 *   period and returns "LED is toggling fast" phrase back to HyperTerminal
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

#include "usbConstructs.h"

#include "defs.h"
#include "jtag.h"
#include "safesleep.h"

extern unsigned int boardInit(void);

VOID Init_Ports (VOID);
VOID Init_Clock (VOID);
VOID Init_TimerA1 (VOID);
static void inline Reset_TimerA1(void) {
	TA1CTL = TASSEL__ACLK + ID__2 + TACLR + MC__UP;                              //ACLK/1, clear TAR
}
BYTE retInString (char* string);

volatile BYTE bHIDDataReceived_event = FALSE;   //Indicates data has been received without an open rcv operation
volatile BYTE bTimerTripped_event = FALSE;

#define MAX_STR_LENGTH 64

unsigned int SlowToggle_Period = 20000 - 1;
unsigned int FastToggle_Period = 1000 - 1;

/*  
 * ======== main ========
 */
VOID main (VOID)
{
    WDTCTL = WDTPW + WDTHOLD;                                   //Stop watchdog timer

    Init_Ports();                                               //Init ports (do first ports because clocks do change ports)
    SetVCore(3);
    Init_Clock();                                               //Init clocks

    USB_init();                                 //init USB
    Init_TimerA1();

    //Enable various USB event handling routines
    USB_setEnabledEvents(
        kUSB_VbusOnEvent + kUSB_VbusOffEvent + kUSB_receiveCompletedEvent
        + kUSB_dataReceivedEvent + kUSB_UsbSuspendEvent + kUSB_UsbResumeEvent +
        kUSB_UsbResetEvent);

    //See if we're already attached physically to USB, and if so, connect to it
    //Normally applications don't invoke the event handlers, but this is an exception.
    if (USB_connectionInfo() & kUSB_vbusPresent){
        USB_handleVbusOnEvent();
    }

    init_sleep(LPM0_bits);
    __enable_interrupt();                           //Enable interrupts globally
    while (1)
    {
        //Check the USB state and directly main loop accordingly
        switch (USB_connectionState())
        {
            case ST_USB_DISCONNECTED:
		    /* FIXME SR_sleep_next decision */
		    set_sleep_mode(/*LPM3_bits*/LPM0_bits);
		    enter_sleep();
                _NOP();
                break;

            case ST_USB_CONNECTED_NO_ENUM:
                break;

            case ST_ENUM_ACTIVE:
		    set_sleep_mode(LPM0_bits);
		    enter_sleep();
                _NOP();                                                             //For Debugger

                                                                                    //Exit LPM on USB receive and perform a receive
                                                                                    //operation
                if (bHIDDataReceived_event){                                        //Some data is in the buffer; begin receiving a
                                                                                    //command
		    int i, o, len;
                    char pieceOfString[MAX_STR_LENGTH] = "";                        //Holds the new addition to the string
                    bHIDDataReceived_event = FALSE;  // Must be before receive

		    do {

                    len=hidReceiveDataInBuffer((BYTE*)pieceOfString,
                        MAX_STR_LENGTH,
                        HID0_INTFNUM);                                              //Get the next piece of the string

		    o = usbblaster_process_buffer(pieceOfString, len);

		    if (o) {
		    Reset_TimerA1();
#if 0 /* InBackground expects the buffer to remain unchanged */
                    hidSendDataInBackground((BYTE*)pieceOfString,
                        o,HID0_INTFNUM,0);                      //Echoes back the characters received (needed
                                                                                    //for Hyperterm)
#else
		    hidSendDataWaitTilDone((BYTE*)pieceOfString,o,HID0_INTFNUM,0);
#endif
		    }
		    } while (len);
                }
#if 1
		else if (bTimerTripped_event) {  // Be sure to send status bytes now and then (16ms)
			bTimerTripped_event = FALSE;
			hidSendDataWaitTilDone(0,0,HID0_INTFNUM,0);
		}
#endif
                break;

            case ST_ENUM_SUSPENDED:
                LED_OFF;                                                     //When suspended, turn off LED
		set_sleep_mode(LPM0_bits);
		enter_sleep();
                break;

            case ST_ENUM_IN_PROGRESS:
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
#elif ORDB3A
			P5SEL |= BIT2;	// External oscillator, we do not need XT2OUT
#else
# error unknown board
#endif

        //use REFO for FLL and ACLK
        UCSCTL3 = (UCSCTL3 & ~(SELREF_7)) | (SELREF__REFOCLK);
        UCSCTL4 = (UCSCTL4 & ~(SELA_7)) | (SELA__REFOCLK);

        //MCLK will be driven by the FLL (not by XT2), referenced to the REFO
        Init_FLL_Settle(USB_MCLK_FREQ / 1000, USB_MCLK_FREQ / 32768);   //Start the FLL, at the freq indicated by the config
                                                                        //constant USB_MCLK_FREQ
#ifdef OLIMEXINO_5510
        XT2_Start(XT2DRIVE_0);                                          //Start the "USB crystal"
#elif ORDB3A
	XT2_Bypass();
#else
#error What sort of clock?
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
    PJOUT = 0b0101;
    PJDIR = 0b0000;
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
	TA1CCR0 = 16*(32768/1024/2);  // default timeout 16ms
	TA1CCTL0 = CCIE;                                        //CCR0 interrupt enabled
	Reset_TimerA1();
}

/*  
 * ======== retInString ========
 */
//This function returns true if there's an 0x0D character in the string; and if so,
//it trims the 0x0D and anything that had followed it.
BYTE retInString (char* string)
{
    BYTE retPos = 0,i,len;
    char tempStr[MAX_STR_LENGTH] = "";

    strncpy(tempStr,string,strlen(string));                 //Make a copy of the string
    len = strlen(tempStr);
    while ((tempStr[retPos] != 0x21) && (retPos++ < len)) ; //Find 0x21 "!"; if not found, retPos ends up at len

    if (retPos < len){                                      //If 0x21 was actually found...
        for (i = 0; i < MAX_STR_LENGTH; i++){               //Empty the buffer
            string[i] = 0x00;
        }
        strncpy(string,tempStr,retPos);                     //...trim the input string to just before 0x0D
        return ( TRUE) ;                                    //...and tell the calling function that we did so
    }

    return ( FALSE) ;                                       //Otherwise, it wasn't found
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
