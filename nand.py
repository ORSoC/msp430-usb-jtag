#! /usr/bin/env python

# Host side interface to ORDB3 NAND Flash, through MSP430 microcontroller
import struct, usb, traceback, sys

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
    def __init__(self, vids=(0x09fb, 0x0403), pids=(0x6001, 0x6010), iface=1):
        for bus in usb.busses():
            for dev in bus.devices:
                if dev.idVendor in vids and dev.idProduct in pids:
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
        assert self.inep and self.outep, "Interface %d endpoints not found"%(iface)
        try:
            self.h.detachKernelDriver(iface)
        except:
            pass
        self.h.claimInterface(iface)
    def read(self,size):
        # filter out 31 60 headers as libftdi does
        packs=[]
        #statuswords=size//62+1
        while size>0:
            # TODO: read in a large URB then filter
            pack=bytearray(self.h.bulkRead(self.inep, min(size+2,64), 5000))
            if not pack.startswith("\x31\x60"):
                raise ValueError("FTDI header mismatch: got "+repr(pack))
            del pack[:2]  # Remove FTDI header
            size-=len(pack)
            packs.append(str(pack))
        return ''.join(packs)
    def write(self,data):
        return self.h.bulkWrite(self.outep, data, 5000)

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
        self.rowaddressbytes=addresscycles &0x0f
        self.columnaddressbytes=addresscycles  >>4
        self.totalpages=self.pagesperblock*self.blocksperunit*self.units
        #self.pageaddressbits=bitsforaddress(self.pagesperblock)
    def loadpage(self, pageno):
        print "Loading page", pageno
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
    def readpage(self, page):
        self.loadpage(page)
        return self.datafrompage(size=self.bytesperpage)
    def datafrompage(self, start=0, size=None):
        if size is None:
            # Default to reading the rest of the page
            size=self.bytesperpage+self.spareperpage-start
        self.command(0x00)   # switch to read mode
        if start:
            self.command(0x05, struct.pack('<H',start))   # set start address
            return self.command(0xe0, readlen=size)
        else:
            return self.command(0x00, readlen=size)
    def badblock(self, block):
        # check first and last page of the block
        for page in (0,self.pagesperblock-1):
            self.loadpage(block*self.pagesperblock+page)
            marker=self.datafrompage(self.bytesperpage,1)
            if countones(ord(marker))<=4:
                return True
        return False
    def eraseblock(self, block):
        print "Erasing block", block,
        address=self.pagetorowaddress(block*self.pagesperblock)
        print "address", repr(address)
        self.command(0x60, address)
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
        return '\x00'*self.columnaddressbytes+self.pagetorowaddress(page)
    def pagetorowaddress(self, page):
        return struct.pack('<Q',page)[:self.rowaddressbytes]
    def command(self, cmd, address="", outdata="", readlen=0):
        cmdbytes=struct.pack('<BBHH', cmd, len(address), len(outdata), readlen)
        towrite=cmdbytes+address
        #print "Writing command %r(%d) address %r(%d)"%(cmdbytes,len(cmdbytes),
        #                                               address,len(address))
        self.conn.write(towrite+outdata)
        if False:
          written=0
          outlen=len(outdata)
          while written<len(outdata):
            towrite=outdata[written:written+64]
            print "Writing %d bytes: %r"%(len(towrite),towrite)
            print "Written %d bytes, %d left"%(written,len(outdata)-written)
            wrote=self.conn.write(outdata[written:written+64])
            written+=wrote
            (outlen,)=struct.unpack('<H',self.conn.read(2))
            print "Wrote %d more, FW expects %d more"%(wrote,outlen)
          while outlen:
            (outlen,)=struct.unpack('<H',self.conn.read(2))
            print "Waited for FW to sync up (%d left)"%(outlen)
        return self.conn.read(readlen)
    def readparampage(self):
        return self.command(0xec, address='\0', readlen=256)
    def readstatus(self):
        "Status register. 80=write enabled, 40=ready, 08=rewrite recommended, 02=cached error, 01=error"
        return ord(self.command(0x70, readlen=1))
    def programpage(self, page, data):
        status=self.readstatus()
        if not status & 0x80:
            raise IOError("Tried to program read-only nand, status %#02x"%status)
        self.command(0x80,self.pagetoaddress(page),data)  # Load address and data
        self.command(0x10)  # program to flash
        return self.readstatus()
    def programpagewithverify(self, page, data):
        print "programming page", page,
        sys.stdout.flush()
        status=self.programpage(page,data)
        print "reading back page", page,
        sys.stdout.flush()
        status=self.loadpage(page)
        if status&0x08:  # rewrite recommended
            raise IOError("Rewrite recommended for page %d (just written)"%(page))
        readback=self.datafrompage(0,len(data))
        if readback!=data:
            print "readback mismatch, wrote:", repr(data)
            print "readback mismatch, read:", repr(readback)
            raise IOError("Readback did not match for page %d"%(page))
        print "data matched"
        sys.stdout.flush()
        return status

def writeimage(onfi, image, indexpage=0):
    """Write an image to early blocks of the flash.
    Each page is written from page 1 onwards (blocks are preerased),
    with page 0 holding the list of pages used, which is also returned.
    Pages are verified by read back as they are written, and pages that
    are deemed unusable will be skipped over."""
    indexblock=indexpage//onfi.pagesperblock
    onfi.loadpage(indexpage)
    indexpagedata=onfi.datafrompage()
    blocks=[]
    offset=0
    # May want to start at a later block to not write block 0 too often
    pageno=(indexblock+1)*onfi.pagesperblock
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
            blocks.append(block)
        pagedata=image[offset:offset+onfi.bytesperpage]
        try:
            status=onfi.programpagewithverify(pageno,pagedata)
        except IOError, e:
            raise
            traceback.print_exc()
            # skip entire block, because firmware reads entire blocks
            pageinblock=pageno%onfi.pagesperblock
            pageno=(pageno|(onfi.pagesperblock-1))+1
            offset-=onfi.bytesperpage*pageinblock
            tries+=1
            if tries<5:
                continue
            else:
                raise IOError("Too many failed writes")
        # Page write succeeded
        offset+=len(pagedata)
        pageno+=1
    # Write block list to page 0, terminated with all 1s
    blocklist=struct.pack('<%dI'%(len(blocks)), *blocks)+'\xff\xff\xff\xff'
    indexpagedata=blocklist+indexpagedata[len(blocklist):]
    if indexblock not in blocks:
        print "Erasing index block (%d). Warning: not keeping contents!"%indexblock
        print "Block list stored in page %d (page %d in block %d)"%(indexpage,indexpage%onfi.pagesperblock,
                                                                    indexblock)
        onfi.eraseblock(indexblock)
    onfi.programpagewithverify(indexpage,indexpagedata)
    return blocks

def readimage(onfi, indexpage=0):
    maxblocks=32
    onfi.loadpage(indexpage)
    blocklist=struct.unpack('<%dI'%(maxblocks),
                            onfi.datafrompage(size=maxblocks*4))
    print "Block list:", blocklist
    data=[]
    for block in blocklist:
        if 0<=block<onfi.totalpages/onfi.pagesperblock:
            print "Reading block", block
            for page in range(onfi.pagesperblock):
                onfi.loadpage(block*onfi.pagesperblock+page)
                data.append(onfi.datafrompage(size=onfi.bytesperpage))
        else:
            break
    return ''.join(data)
    
def main(argv):
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
    if argv[1]=='badblocks':
        print "Bad blocks:", onfi.scanforbadblocks()
    if argv[1]=='read':
        if len(argv)>3:  # read from specific page address
            page=int(argv[3],0)
            open(argv[2],'wb').write(onfi.readpage(page))
        else:
            open(argv[2],'wb').write(readimage(onfi))
    if argv[1]=='write':
        data=open(argv[2],'rb').read()
        if len(argv)>3:  # Write to specific block address
            # 32 for ordb3a boot code
            block=int(argv[3],0)
            if len(argv)>4 and argv[4]=='len':   # Patch in length for boot ROM
                data=struct.pack('>I', len(data))+data[4:]
            while data:
                onfi.eraseblock(block)
                for page in range(onfi.pagesperblock*block,
                                  onfi.pagesperblock*(block+1)):
                    onfi.programpagewithverify(page,data[:onfi.bytesperpage])
                    data=data[onfi.bytesperpage:]
                    if not data:
                        break
                block+=1
        else:  # Write XSVF image with block list at 0, for MSP430 to configure FPGA
            blocks=writeimage(onfi,data)
    if argv[1]=='rawread':
        for page in range(int(argv[2])):
            onfi.loadpage(page)
            print repr(onfi.datafrompage())
    if argv[1]=='erase':
        onfi.eraseblock(0)
    if argv[1]=='info':
        print "%d bytes per page, %d pages per block, %d total blocks"%(
            onfi.bytesperpage, onfi.pagesperblock, onfi.totalpages/onfi.pagesperblock)
        print "Address bytes: %d column %d row"%(
            onfi.columnaddressbytes, onfi.rowaddressbytes)

if __name__=='__main__':
    from sys import argv
    main(argv)
