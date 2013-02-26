CC=msp430-gcc
CFLAGS=-mmcu=msp430f5507 -Os -Wall
LDFLAGS=-mmcu=msp430f5507 -Os

# TODO: autogenerate dependencies?
.PHONY: all clean program

all: main usbhidmain

clean:
	-rm main usbhidmain $(USBOBJS) libusb.a $(USBFWOBJS)

program: usbhidmain
	PYTHONPATH=~/msp430/python-msp430-tools python -m msp430.bsl5.hid -B -e -P $<

main: main.o tps65217.o swi2cmst.o

main.o: main.c cfg.h defs.h tps65217.h

swi2cmst.o: swi2cmst.c swi2cmst.h cfg.h

tps65217.o: tps65217.c tps65217.h cfg.h swi2cmst.h

CPPFLAGS=-I. -Imsp430-usb -Imsp430-usb/USB_config -Imsp430-usb/src -Imsp430-usb/src/F5xx_F6xx_Core_Lib \
	-D__REGISTER_MODEL__ -D__TI_COMPILER_VERSION__
USBOBJS=\
	msp430-usb/src/USB_API/USB_Common/dma.o \
	msp430-usb/src/USB_API/USB_Common/usb.o \
	msp430-usb/src/USB_API/USB_HID_API/UsbHid.o \
	msp430-usb/src/F5xx_F6xx_Core_Lib/HAL_UCS.o \
	msp430-usb/src/F5xx_F6xx_Core_Lib/HAL_PMM.o \
	msp430-usb/src/F5xx_F6xx_Core_Lib/HAL_TLV.o \
	usbConstructs.o usbEventHandling.o
USBFWOBJS=usbhidmain.o jtag.o msp430-usb/USB_config/descriptors.o msp430-usb/USB_config/UsbIsr.o
LDFLAGS += -Wl,--defsym=tSetupPacket=0x2380 -Wl,--defsym=tEndPoint0DescriptorBlock=0x0920 -Wl,--defsym=tInputEndPointDescriptorBlock=0x23C8 -Wl,--defsym=tOutputEndPointDescriptorBlock=0x2388 -Wl,--defsym=abIEP0Buffer=0x2378 -Wl,--defsym=abOEP0Buffer=0x2370

libusb.a: $(USBOBJS)
	ar rsc $@ $^

usbhidmain: $(USBFWOBJS) libusb.a 

