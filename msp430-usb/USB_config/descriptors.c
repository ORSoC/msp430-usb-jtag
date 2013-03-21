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

/*-----------------------------------------------------------------------------+
| Include files                                                                |
|-----------------------------------------------------------------------------*/
#include <USB_API/USB_Common/device.h>
#include <USB_API/USB_Common/types.h>                // Basic Type declarations
#include <USB_API/USB_Common/defMSP430USB.h>
#include <USB_API/USB_Common/usb.h>              // USB-specific Data Structures
#include "descriptors.h"
#include <USB_API/USB_CDC_API/UsbCdc.h>
#include <USB_API/USB_HID_API/UsbHidReq.h>

WORD const report_desc_size[HID_NUM_INTERFACES] =
{
36
};
WORD const report_len_input[HID_NUM_INTERFACES] =
{
64	/* With our modifications, specifies maximum size of input packets */
};
/*-----------------------------------------------------------------------------+
| Device Descriptor                                                            |
|-----------------------------------------------------------------------------*/
BYTE const abromDeviceDescriptor[SIZEOF_DEVICE_DESCRIPTOR] = {
    SIZEOF_DEVICE_DESCRIPTOR,               // Length of this descriptor
    DESC_TYPE_DEVICE,                       // Type code of this descriptor
    0x00, 0x02,                             // Release of USB spec
    0x00,                                   // Device's base class code
    0x00,                                   // Device's sub class code
    0x00,                                   // Device's protocol type code
    EP0_PACKET_SIZE,                        // End point 0's packet size
    USB_VID&0xFF, USB_VID>>8,               // Vendor ID for device, TI=0x0451
                                            // You can order your own VID at www.usb.org
    USB_PID&0xFF, USB_PID>>8,               // Product ID for device,
                                            // this ID is to only with this example
    VER_FW_L, VER_FW_H,                     // Revision level of device
    1,                                      // Index of manufacturer name string desc
    2,                                      // Index of product name string desc
    USB_STR_INDEX_SERNUM,                   // Index of serial number string desc
    1                                       //  Number of configurations supported
};

/*-----------------------------------------------------------------------------+
| Configuration Descriptor                                                     |
|-----------------------------------------------------------------------------*/
const struct abromConfigurationDescriptorGroup abromConfigurationDescriptorGroup=
{
    /* Generic part */
    {
        // CONFIGURATION DESCRIPTOR (9 bytes)
        SIZEOF_CONFIG_DESCRIPTOR,                          // bLength
        DESC_TYPE_CONFIG,                                  // bDescriptorType
        DESCRIPTOR_TOTAL_LENGTH, 0x00,                     // wTotalLength
        USB_NUM_INTERFACES,                 	           // bNumInterfaces
        USB_CONFIG_VALUE,                                  // bConfigurationvalue
        CONFIG_STRING_INDEX,                               // iConfiguration Description offset
        USB_SUPPORT_SELF_POWERED | USB_SUPPORT_REM_WAKE,   // bmAttributes, bus power, remote wakeup
        USB_MAX_POWER                                      // Max. Power Consumption
    },

    /******************************************************* start of HID*************************************/
    {
	/*start HID[0] Here - actually not HID but USB-Blaster bulk (emulating FT245) */
        {
            //-------- Descriptor for HID class device -------------------------------------
            // INTERFACE DESCRIPTOR (9 bytes) 
            SIZEOF_INTERFACE_DESCRIPTOR,        // bLength 
            DESC_TYPE_INTERFACE,                // bDescriptorType: 4 
            HID0_REPORT_INTERFACE,              // bInterfaceNumber
            0x00,                               // bAlternateSetting
            2,                                  // bNumEndpoints
            0xFF,                               // bInterfaceClass: 3 = HID Device, FF=vendor specific
            0xFF,                               // bInterfaceSubClass:
            0xFF,                               // bInterfaceProtocol:
            INTF_STRING_INDEX + 0,              // iInterface:1

#if 0
            // HID DESCRIPTOR (9 bytes)
            0x09,     			                // bLength of HID descriptor
            0x21,             		            // HID Descriptor Type: 0x21
            0x01,0x01,			                // HID Revision number 1.01
            0x00,			                    // Target country, nothing specified (00h)
            0x01,			                    // Number of HID classes to follow
            0x22,			                    // Report descriptor type
			 (report_desc_size_HID0 & 0x0ff),  // Total length of report descriptor
 			 (report_desc_size_HID0  >> 8),
#endif
            SIZEOF_ENDPOINT_DESCRIPTOR,         // bLength
            DESC_TYPE_ENDPOINT,                 // bDescriptorType
            HID0_INEP_ADDR,                     // bEndpointAddress; bit7=1 for IN, bits 3-0=1 for ep1
            EP_DESC_ATTR_TYPE_BULK,              // bmAttributes, interrupt transfers
            0x40, 0x00,                         // wMaxPacketSize, 64 bytes
            0,                                  // bInterval, ms

            SIZEOF_ENDPOINT_DESCRIPTOR,         // bLength
            DESC_TYPE_ENDPOINT,                 // bDescriptorType
            HID0_OUTEP_ADDR,                    // bEndpointAddress; bit7=1 for IN, bits 3-0=1 for ep1
            EP_DESC_ATTR_TYPE_BULK,              // bmAttributes, interrupt transfers
            0x40, 0x00,                         // wMaxPacketSize, 64 bytes
            0,                                  // bInterval, ms

	         /* end of HID[0]*/
        }

    }
    /******************************************************* end of HID**************************************/

};
/*-----------------------------------------------------------------------------+
| String Descriptor                                                            |
|-----------------------------------------------------------------------------*/
BYTE const abromStringDescriptor[] = {

	// String index0, language support
	4,		// Length of language descriptor ID
	3,		// LANGID tag
	0x09, 0x04,	// 0x0409 for English

#if 1
	2, 3,	// Empty string
#else
	// String index1, Manufacturer
	36,		// Length of this string descriptor
	3,		// bDescriptorType
	'O',0x00,'R',0x00,'S',0x00,'o',0x00,'C',0x00,' ',0x00,
	'A',0x00,'B',0x00,' ',0x00,' ',0x00,' ',0x00,' ',0x00,
	' ',0x00,' ',0x00,' ',0x00,' ',0x00,' ',0x00,
#endif

#if 1
	2+2*10,	// length of descriptor
	3,	// bDescriptorType=string
	'U',0x00,'s',0x00,'b',0x00,'B',0x00,'l',0x00,'a',0x00,
	's',0x00,'t',0x00,'e',0x00,'r',0x00,
#else
	// String index2, Product
	38,		// Length of this string descriptor
	3,		// bDescriptorType
	'U',0x00,'S',0x00,'B',0x00,' ',0x00,'B',0x00,'l',0x00,
	'a',0x00,'s',0x00,'t',0x00,'e',0x00,'r',0x00,' ',0x00,
	'x',0x00,'a',0x00,'m',0x00,'p',0x00,'l',0x00,'e',0x00,
#endif

	// String index3, Serial Number
	4,		// Length of this string descriptor
	3,		// bDescriptorType
	'0',0x00,

	// String index4, Configuration String
	22,		// Length of this string descriptor
	3,		// bDescriptorType
	'M',0x00,'S',0x00,'P',0x00,'4',0x00,'3',0x00,'0',0x00,
	' ',0x00,'U',0x00,'S',0x00,'B',0x00,

	// String index5, Interface String
	28,		// Length of this string descriptor
	3,		// bDescriptorType
	'F',0x00,'T',0x00,'D',0x00,'I',0x00,'I',0x00,'n',0x00,
	't',0x00,'e',0x00,'r',0x00,'f',0x00,'a',0x00,'c',0x00,
	'e',0x00
};

BYTE const report_desc_HID0[]=
{
    0x06, 0x00, 0xff,	// Usage Page (Vendor Defined)
    0x09, 0x01,	// Usage Page (Vendor Defined)
    0xa1, 0x01,	// COLLECTION (Application)
    0x85, 0x3f,	// Report ID (Vendor Defined)
    0x95, MAX_PACKET_SIZE-1,	// Report Count
    0x75, 0x08,	// Report Size
    0x25, 0x01,	// Usage Maximum
    0x15, 0x01,	// Usage Minimum
    0x09, 0x01,	// Vendor Usage
    0x81, 0x02,	// Input (Data,Var,Abs)
    0x85, 0x3f,	// Report ID (Vendor Defined)
    0x95, MAX_PACKET_SIZE-1,	// Report Count
    0x75, 0x08,	// Report Size
    0x25, 0x01,	// Usage Maximum
    0x15, 0x01,	// Usage Minimum
    0x09, 0x01,	// Vendor Usage
    0x91 ,0x02,	// Ouput (Data,Var,Abs)
    0xc0	// end Application Collection
};
const PBYTE report_desc[HID_NUM_INTERFACES] =
{
(PBYTE)&report_desc_HID0
};
/**** Populating the endpoint information handle here ****/

const struct tUsbHandle stUsbHandle[]=
{
	{
        HID0_INEP_ADDR,
        HID0_OUTEP_ADDR,
        0, 
        HID_CLASS,
        0,
        0,
        OEP2_X_BUFFER_ADDRESS,
        OEP2_Y_BUFFER_ADDRESS,
        IEP1_X_BUFFER_ADDRESS,
        IEP1_Y_BUFFER_ADDRESS
    }
};
//-------------DEVICE REQUEST LIST---------------------------------------------
/* Ignore FTDI specific device set requests (such as modes and baud rates) */
BYTE usbSetVendor(VOID) {
	/* Just acknowledge the data without using it */
        usbSendZeroLengthPacketOnIEP0();
	return (FALSE);
}

extern __no_init tDEVICE_REQUEST __data16 tSetupPacket;

/* FTDI latency timer setting */
BYTE usbSetLatencyTimer(VOID) {
	TA1CCR0 = tSetupPacket.wValue*(32768/1024/2);
        usbSendZeroLengthPacketOnIEP0();
	return (FALSE);
}
BYTE usbGetLatencyTimer(VOID) {
	BYTE ftdi_latency = TA1CCR0/(32768/1024/2);
	usbClearOEP0ByteCount();            //for status stage
	wBytesRemainingOnIEP0 = 1;
	usbSendDataPacketOnEP0(&ftdi_latency);
	return (FALSE);
}

extern __no_init tEDB0 tEndPoint0DescriptorBlock;
BYTE usbDisconnectThenBSL(VOID) {
	volatile long i;
	/* Just acknowledge the data without using it */
        usbSendZeroLengthPacketOnIEP0();
//	while (!(tEndPoint0DescriptorBlock.bIEPBCNT & EPBCNT_NAK));  /* wait until sent */
	/* Shut down USB contact - TODO: de-enumerate? wait for response to be sent? */
	USB_disable();
	/* Stop DMA units so they can't interfere with BSL */
	DMA0CTL = DMA1CTL = DMA2CTL = 0;
	/* Wait to make sure host sees we're gone */
	for (i=0; i<USB_MCLK_FREQ/40; i++);
	/* Start BSL */
	void (*BSL)(void) = (void*)0x1000;
	BSL();	// Does not return
	return (FALSE);
}

const tDEVICE_REQUEST_COMPARE tUsbRequestList[] = 
{
	/* FTDI latency timer set/get */
	USB_REQ_TYPE_OUTPUT | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE, 9,
	0xff,0xff, 0xff,0xff, 0xff,0xff,
	0xc0, &usbSetLatencyTimer,
	USB_REQ_TYPE_INPUT | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE, 10,
	0xff,0xff, 0xff,0xff, 0xff,0xff,
	0xc0, &usbGetLatencyTimer,
	/* Vendor specific requests - sent for FTDI chip */
	USB_REQ_TYPE_OUTPUT | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE, 0,
	0,0, 0,0, 0,0,
	0x80, &usbSetVendor,
	// Huawei style mode switch - jump to bootloader
	USB_REQ_TYPE_OUTPUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE,
	USB_REQ_SET_FEATURE, PUTWORD(1), PUTWORD(0), PUTWORD(0),
	0xff, &usbDisconnectThenBSL,
	
#if 0
    //---- HID 0 Class Requests -----//
    USB_REQ_TYPE_INPUT | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
 	USB_REQ_GET_REPORT,
	0xff,0xff,
	HID0_REPORT_INTERFACE,0x00,
	0xff,0xff,
	0xcc,&usbGetReport,
	// SET REPORT
	USB_REQ_TYPE_OUTPUT | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
	USB_REQ_SET_REPORT,
	0xff,0xFF,                          // bValueL is index and bValueH is type
	HID0_REPORT_INTERFACE,0x00,
	0xff,0xff,
	0xcc,&usbSetReport,
	// GET REPORT DESCRIPTOR
	USB_REQ_TYPE_INPUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
    USB_REQ_GET_DESCRIPTOR,
    0xff,DESC_TYPE_REPORT,              // bValueL is index and bValueH is type
    HID0_REPORT_INTERFACE,0x00,
    0xff,0xff,
    0xdc,&usbGetReportDescriptor,

    // GET HID DESCRIPTOR
    USB_REQ_TYPE_INPUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
    USB_REQ_GET_DESCRIPTOR,
    0xff,DESC_TYPE_HID,                 // bValueL is index and bValueH is type
    HID0_REPORT_INTERFACE,0x00,
    0xff,0xff,
    0xdc,&usbGetHidDescriptor,
#endif
    //---- USB Standard Requests -----//
    // clear device feature
    USB_REQ_TYPE_OUTPUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE,
    USB_REQ_CLEAR_FEATURE,
    FEATURE_REMOTE_WAKEUP,0x00,         // feature selector
    0x00,0x00,
    0x00,0x00,
    0xff,&usbClearDeviceFeature,

    // clear endpoint feature
    USB_REQ_TYPE_OUTPUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_ENDPOINT,
    USB_REQ_CLEAR_FEATURE,
    FEATURE_ENDPOINT_STALL,0x00,
    0xff,0x00,
    0x00,0x00,
    0xf7,&usbClearEndpointFeature,

    // get configuration
    USB_REQ_TYPE_INPUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE,
    USB_REQ_GET_CONFIGURATION,
    0x00,0x00, 
    0x00,0x00, 
    0x01,0x00,
    0xff,&usbGetConfiguration,

    // get device descriptor
    USB_REQ_TYPE_INPUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE,
    USB_REQ_GET_DESCRIPTOR,
    0xff,DESC_TYPE_DEVICE,              // bValueL is index and bValueH is type
    0xff,0xff,
    0xff,0xff,
    0xd0,&usbGetDeviceDescriptor,

    // get configuration descriptor
    USB_REQ_TYPE_INPUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE,
    USB_REQ_GET_DESCRIPTOR,
    0xff,DESC_TYPE_CONFIG,              // bValueL is index and bValueH is type
    0xff,0xff,
    0xff,0xff,
    0xd0,&usbGetConfigurationDescriptor,

    // get string descriptor
    USB_REQ_TYPE_INPUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE,
    USB_REQ_GET_DESCRIPTOR,
    0xff,DESC_TYPE_STRING,              // bValueL is index and bValueH is type
    0xff,0xff,
    0xff,0xff,
    0xd0,&usbGetStringDescriptor,

    // get interface
    USB_REQ_TYPE_INPUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
    USB_REQ_GET_INTERFACE,
    0x00,0x00,
    0xff,0xff,
    0x01,0x00,
    0xf3,&usbGetInterface,

    // get device status
    USB_REQ_TYPE_INPUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE,
    USB_REQ_GET_STATUS,
    0x00,0x00,
    0x00,0x00,
    0x02,0x00,
    0xff,&usbGetDeviceStatus, 
    // get interface status
    USB_REQ_TYPE_INPUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
    USB_REQ_GET_STATUS,
    0x00,0x00,
    0xff,0x00,
    0x02,0x00,
    0xf7,&usbGetInterfaceStatus,
    // 	get endpoint status
    USB_REQ_TYPE_INPUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_ENDPOINT,
    USB_REQ_GET_STATUS,
    0x00,0x00,
    0xff,0x00,
    0x02,0x00,
    0xf7,&usbGetEndpointStatus,

    // set address
    USB_REQ_TYPE_OUTPUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE,
    USB_REQ_SET_ADDRESS,
    0xff,0x00,
    0x00,0x00,
    0x00,0x00,
    0xdf,&usbSetAddress,

    // set configuration
    USB_REQ_TYPE_OUTPUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE,
    USB_REQ_SET_CONFIGURATION,
    0xff,0x00,
    0x00,0x00,
    0x00,0x00,
    0xdf,&usbSetConfiguration,

    // set device feature
    USB_REQ_TYPE_OUTPUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE,
    USB_REQ_SET_FEATURE,
    0xff,0x00,                      // feature selector
    0x00,0x00,
    0x00,0x00,
    0xdf,&usbSetDeviceFeature,

    // set endpoint feature
    USB_REQ_TYPE_OUTPUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_ENDPOINT,
    USB_REQ_SET_FEATURE,
    0xff,0x00,                      // feature selector
    0xff,0x00,                      // endpoint number <= 127
    0x00,0x00,
    0xd7,&usbSetEndpointFeature,

    // set interface
    USB_REQ_TYPE_OUTPUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
    USB_REQ_SET_INTERFACE,
    0xff,0x00,                      // feature selector
    0xff,0x00,                      // interface number
    0x00,0x00,
    0xd7,&usbSetInterface,

    // end of usb descriptor -- this one will be matched to any USB request
    // since bCompareMask is 0x00.
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, 
    0x00,&usbInvalidRequest     // end of list
};

/*-----------------------------------------------------------------------------+
| END OF Descriptor.c FILE                                                     |
|-----------------------------------------------------------------------------*/
