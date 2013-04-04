#! /usr/bin/env python

# Host side interface to ORDB3 NAND Flash, through MSP430 microcontroller
import struct, usb, array

# NAND command serialization:
#  <B  command byte
#   B  number of address bytes
#   H  number of data bytes out
#   H  number of data bytes in
# Commands are always aligned to the beginning of a USB transfer.
# Address is column (page) then row (block). 

def bitsforaddress(n):
    bits=0
    while (n>1):
        bits+=1
        n-=1
    return bits

def countones(n):
    count=0
    while n:
        count+=n&1
        n=n>>1
    return count

class UsbConn:
    def __init__(self, vid=0x09fb, pid=0x6001, iface=1): #vid=0x0403, pid=0x6010):
        for bus in usb.busses():
            for dev in bus.devices:
                if dev.idVendor==vid and dev.idProduct==pid:
                    self.dev=dev
        if not hasattr(self,"dev"):
            raise ValueError("Device not found")
        self.h=self.dev.open()
        # Since I don't know how to do this, either leave IDs as Altera or unload ftdi_sio!
        #self.h.detach_kernel_driver()
        #self.h.claimInterface(iface)
        cfg=self.dev.configurations[0]
        # No clue why we get tuples
        for intf in cfg.interfaces:
            for alt in intf:
                #print alt, alt.interfaceNumber, dir(alt)
                if alt.interfaceNumber==iface:
                    for ep in alt.endpoints:
                        inp=ep.address&0x80
                        epno=ep.address&0x7f
                        if inp:
                            self.inep=epno
                        else:
                            self.outep=epno
    def read(self,size):
        # filter out 31 60 headers as libftdi does
        packs=[]
        statuswords=size//62+1
        while size>0:
            # TODO: read in a large URB then filter
            pack=bytearray(self.h.bulkRead(self.inep, min(size+2,64))[2:])
            size-=len(pack)
            packs.append(pack)
            #print len(pack), size, repr(pack)
        return ''.join(map(str,packs))
    def write(self,data):
        return self.h.bulkWrite(self.outep, data)

class ONFI:
    def __init__(self, conn):
        self.conn=conn
        self.parampage=self.readparampage()
        self.vendor=self.parampage[32:44]
        self.model=self.parampage[44:64]
        (self.bytesperpage,self.spareperpage,
         bytesperpartialpage, spareperpartialpage,  # obsolete
         self.pagesperblock, self.blocksperunit, self.units,
         addresscycles)=struct.unpack('<IHIHIIBB',self.parampage[80:102])
        self.blockaddressbytes=addresscycles &0x0f
        self.pageaddressbytes=addresscycles  >>4
        self.totalpages=self.pagesperblock*self.blocksperunit*self.units
        #self.pageaddressbits=bitsforaddress(self.pagesperblock)
    def loadpage(self, pageno):
        self.command(0x00, self.pagetoaddress(pageno))  # load address
        self.command(0x30)  # Request read
        while True:
            status=self.readstatus()
            if status&0x40:
                if status&0x01:
                    raise IOError("Failed to read block")
                else:
                    # Ready, so return
                    return status
            else:
                print "status not ready"
    def datafrompage(self, start=0, size=None):
        if size is None:
            # Default to reading the rest of the page
            size=self.bytesperpage+self.spareperpage-start
        self.command(0x00)   # switch to read mode
        self.command(0x05, struct.pack('<H',start))   # set start address
        return self.command(0xe0, readlen=size)
    def badblock(self, block):
        # check first and last page of the block
        for page in (0,self.pagesperblock-1):
            self.loadpage(block*self.pagesperblock+page)
            marker=self.datafrompage(self.bytesperpage,1)
            if countones(ord(marker))<=4:
                return True
        return False
    def eraseblock(self, block):
        self.command(0x60, self.pagetoblockaddress(block*self.pagesperblock))
        self.command(0xd0)
        return self.readstatus()
    def scanforbadblocks(self):
        # Read the bad block marker for every block and return a list of
        # which block are marked bad
        badblocks=[]
        for block in range(0, self.blocksperunit*self.units):
            if self.badblock(block):
                badblocks.append(block)
        self.badblocks=badblocks
        return badblocks
    def pagetoaddress(self, page):
        #block=page>>self.pageaddressbits
        #pageinblock=page&(self.pagesperblock-1)
        block,pageinblock=divmod(page,self.pagesperblock)
        return (struct.pack('<Q',pageinblock)[:self.pageaddressbytes] +
                struct.pack('<Q',block)[:self.blockaddressbytes])
    def pagetoblockaddress(self, page):
        return self.pagetoaddress(page)[-self.blockaddressbytes:]
    def command(self, cmd, address="", outdata="", readlen=0):
        cmdbytes=struct.pack('<BBHH', cmd, len(address), len(outdata), readlen)
        self.conn.write(cmdbytes+address+outdata)
        return self.conn.read(readlen)
    def readparampage(self):
        return self.command(0xec, address='\0', readlen=256)
    def readstatus(self):
        "Status register. 80=write enabled, 40=ready, 08=rewrite recommended, 01=error"
        return ord(self.command(0x70, readlen=1))
    def programpage(self, page, data):
        self.command(0x80,self.pagetoaddress(page),data)  # Load address and data
        self.command(0x10)  # program to flash
        return self.readstatus()
    def programpagewithverify(self, page, data):
        status=self.programpage(page,data)
        status=onfi.loadpage(page)
        if status&0x08:  # rewrite recommended
            raise IOError("Rewrite recommended for page %d (just written)"%(page))
        readback=onfi.datafrompage(0,len(data))
        if readback!=data:
            raise IOError("Readback did not match for page %d"%(page))
        return status

def writeimage(onfi, image):
    """Write an image to early blocks of the flash.
    Each page is written from page 1 onwards (blocks are preerased),
    with page 0 holding the list of pages used, which is also returned.
    Pages are verified by read back as they are written, and pages that
    are deemed unusable will be skipped over."""
    onfi.loadpage(0)
    page0data=onfi.datafrompage()
    pages=[]
    offset=0
    pageno=1  # May want to start at a later block to not write block 0 too often
    block=None   # Always erase first block
    tries=0
    while offset<len(image):
        if block!=pageno//onfi.pagesperblock:  # New block
            block=pageno//onfi.pagesperblock
            while onfi.badblock(block):
                # Skip bad blocks
                block+=1
                pageno=onfi.pagesperblock*block
            onfi.eraseblock(block)
        pagedata=image[offset:offset+onfi.bytesperpage]
        try:
            status=onfi.programpagewithverify(pageno,pagedata)
        except IOError, e:
            print e
            pageno+=1
            tries+=1
            if tries<5:
                continue
            else:
                raise IOError("Too many failed writes")
        # Page write succeeded
        pages.append(pageno)
        offset+=len(pagedata)
        pageno+=1
    # Write block list to page 0, terminated with all 1s
    blocklist=struct.pack('<%dI'%(len(pages)), *pages)+'\xff\xff\xff\xff'
    page0data=blocklist+page0data[len(blocklist):]
    onfi.programpagewithverify(0,page0data)
    return pages
    
def main():
    #ser=open('/dev/ttyUSB1','rb+')
    ser=UsbConn()
    #ser.write('\0'*(256+2**16+6))   # Enough blank data to clear NAND state
    # Sync protocol might not be needed. We use start of a USB packet to parse
    # commands. If we receive a command that does not look valid, we flush that
    # buffer. So sending a lot of 0 followed by two short frames should bring
    # us in sync. On the other hand, we've never been out of sync yet. 
    onfi=ONFI(ser)
    #print "Parameter page:", repr(onfi.parampage)
    print "Found %s %s capacity %g mebibytes"%(onfi.vendor,onfi.model,
                                           onfi.totalpages*onfi.bytesperpage/1024**2)
    print "Bad blocks:", onfi.scanforbadblocks()

if __name__=='__main__':
    main()
