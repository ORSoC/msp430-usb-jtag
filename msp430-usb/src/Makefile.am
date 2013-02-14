CFLAGS += -D__MSP430F5522__ -D__TI_COMPILER_VERSION__ -D__REGISTER_MODEL__ -DUSB_TAG
CFLAGS += -I$(top_srcdir)/src/USB_API/USB_CDC_API -I$(top_srcdir)/src/USB_config \
	 -I$(top_srcdir)/src/F5xx_F6xx_Core_Lib \
	 -ggdb -g3 -O0

LDFLAGS += -Wl,--defsym=tSetupPacket=0x2380 -Wl,--defsym=tEndPoint0DescriptorBlock=0x0920 -Wl,--defsym=tInputEndPointDescriptorBlock=0x23C8 -Wl,--defsym=tOutputEndPointDescriptorBlock=0x2388 -Wl,--defsym=abIEP0Buffer=0x2378 -Wl,--defsym=abOEP0Buffer=0x2370

bin_PROGRAMS = demo

demo_SOURCES =  USB_API/USB_CDC_API/UsbCdc.c \
		USB_API/USB_Common/dma.c USB_API/USB_Common/usb.c \
		USB_API/USB_HID_API/UsbHid.c USB_API/USB_HID_API/UsbHidReq.c \
		USB_API/USB_MSC_API/UsbMscReq.c USB_API/USB_MSC_API/UsbMscScsi.c USB_API/USB_MSC_API/UsbMscStateMachine.c \
		USB_API/USB_PHDC_API/UsbPHDC.c \
		USB_config/UsbIsr.c USB_config/descriptors.c \
		F5xx_F6xx_Core_Lib/HAL_FLASH.c F5xx_F6xx_Core_Lib/HAL_PMAP.c F5xx_F6xx_Core_Lib/HAL_PMM.c \
		F5xx_F6xx_Core_Lib/HAL_TLV.c F5xx_F6xx_Core_Lib/HAL_UCS.c \
		usbConstructs.c usbEventHandling.c main.c

%.ihex: %
	msp430-objcopy -O ihex $< $@

%.lst: %
	msp430-objdump -dst $< > $@
