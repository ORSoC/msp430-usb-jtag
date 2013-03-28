#! /usr/bin/env python

# Host side interface to ORDB3 NAND Flash, through MSP430 microcontroller
import struct, usb

class UsbConn:
    def __init__(self, vid=0x09fb, pid=0x6001): #vid=0x0403, pid=0x6010):
        for bus in usb.busses():
            for dev in bus.devices:
                if dev.idVendor==vid and dev.idProduct==pid:
                    self.dev=dev
        self.h=self.dev.open()
        # Since I don't know how to do this, either leave IDs as Altera or unload ftdi_sio!
        #self.h.detach_kernel_driver()
        self.h.claimInterface(1)
    def read(self,size):
        return self.h.bulkRead(3, size)
    def write(self,data):
        return self.h.bulkWrite(4, data)

def main():
    #ser=open('/dev/ttyUSB1','rb+')
    try:
        ser=UsbConn()
    except:
        ser=UsbConn()
    #ser.write('\0'*(256+2**16+6))   # Enough blank data to clear NAND state
    # TODO: Add a sync method to protocol.
    readparampage=struct.pack('<BBHHB', 0xec, 1, 0, 256, 0)
    ser.write(readparampage)
    parampage=ser.read(256+5*2)
    print "Parameter page:", repr(''.join(map(chr,parampage)))
    # TODO: filter out 31 60 headers as libftdi does

if __name__=='__main__':
    main()
