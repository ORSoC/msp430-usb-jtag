#BOARD ?= OLIMEXINO_5510
#MCU ?= msp430f5510

BOARD ?= ORDB3A
MCU ?= msp430f5507

CC=msp430-gcc
CFLAGS=-mmcu=$(MCU) -Os -Wall -g
LDFLAGS=-mmcu=$(MCU) -Os -g

# TODO: autogenerate dependencies?
.PHONY: all clean program

all: bootstrapper ordb3a_firmware

clean:
	-rm -f bootstrapper ordb3a_firmware $(USBOBJS) libusb.a $(USBFWOBJS) $(LIBXSVFOBJS) libxsvf.a

prog-hid: ordb3a_firmware
	#-sudo usb_modeswitch -v 09fb -p 6001 -H -V 2047 -P 0200
	PYTHONPATH=~/msp430/python-msp430-tools python -m msp430.bsl5.hid -B -e -P $<

prog-debug: ordb3a_firmware.golden
	LD_LIBRARY_PATH=/home/yann/msp430/MSP430.DLLv3_OS_Package	\
			mspdebug tilib -d /dev/ttyACM0 prog $<


swi2cmst.o: swi2cmst.c swi2cmst.h cfg.h

tps65217.o: tps65217.c tps65217.h cfg.h swi2cmst.h


CPPFLAGS=-I. -Ilibxsvf -Imsp430-usb -Imsp430-usb/USB_config -Imsp430-usb/src -Imsp430-usb/src/F5xx_F6xx_Core_Lib \
	-D__REGISTER_MODEL__ -D__TI_COMPILER_VERSION__ -D$(BOARD) -DLIBXSVF_WITHOUT_SVF -DLIBXSVF_WITHOUT_SCAN

USBOBJS=\
	msp430-usb/src/USB_API/USB_Common/dma.o \
	msp430-usb/src/USB_API/USB_Common/usb.o \
	msp430-usb/src/USB_API/USB_HID_API/UsbHid.o \
	msp430-usb/src/USB_API/USB_CDC_API/UsbCdc.o \
	msp430-usb/src/F5xx_F6xx_Core_Lib/HAL_UCS.o \
	msp430-usb/src/F5xx_F6xx_Core_Lib/HAL_PMM.o \
	msp430-usb/src/F5xx_F6xx_Core_Lib/HAL_TLV.o \
	usbConstructs.o usbEventHandling.o
LIBXSVFOBJS=libxsvf/xsvf.o libxsvf/play.o libxsvf/tap.o
USBFWOBJS=ordb3a_main.o jtag.o msp430-usb/USB_config/descriptors.o \
	boardinit.o tps65217.o swi2cmst.o uart.o \
	msp430-usb/src/F5xx_F6xx_Core_Lib/HAL_PMAP.o \
	msp430-usb/USB_config/UsbIsr.o nand_ordb3.o
LDFLAGS += -Wl,--defsym=tSetupPacket=0x2380 -Wl,--defsym=tEndPoint0DescriptorBlock=0x0920 -Wl,--defsym=tInputEndPointDescriptorBlock=0x23C8 -Wl,--defsym=tOutputEndPointDescriptorBlock=0x2388 -Wl,--defsym=abIEP0Buffer=0x2378 -Wl,--defsym=abOEP0Buffer=0x2370

libusb.a: $(USBOBJS)
	ar rsc $@ $^

libxsvf.a: $(LIBXSVFOBJS)
	ar rsc $@ $^

ordb3a_firmware: $(USBFWOBJS) libusb.a libxsvf.a
	$(CC) -o $@ $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS)

## Don't! Cy5 module does expect cy5 board define #bootstrapper: BOARD=OLIMEXINO5510_ORSOC8695EP4GX
bootstrapper: MCU=msp430f5510
bootstrapper: main.c tps65217.c swi2cmst.c
	$(CC) -o $@ -DLED_AVAIL -DBOOTSTRAP $(LDFLAGS) $(LOADLIBES) $(LDLIBS) $(CPPFLAGS) $(CFLAGS) $^

#main.o: main.c cfg.h defs.h tps65217.h
