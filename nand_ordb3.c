/* Routines to access NAND Flash memory on ORDB3 from MSP430 */

#include <USB_API/USB_Common/types.h>
#include <USB_API/USB_HID_API/UsbHid.h>

#include <stdint.h>
#include <msp430.h>

#include <safesleep.h>

#include <nand_ordb3.h>

#define LIBXSVF
#ifdef LIBXSVF
#include <stdlib.h>  /* for realloc() */
#include <string.h>  /* for memcpy() */
#include <libxsvf.h>
#include <jtag.h>    /* For libxsvf JTAG operations */
#endif

// Port J is mapped as port 19 if you extrapolate from 1/2, 3/4 etc
#define J 19
#define P19DIR PJDIR
#define P19REN PJREN
#define P19OUT PJOUT
#define P19IN PJIN

#if ORDB3A
#define DATA_PORT 1

#define CLE_PORT 5
#define CEn_PORT 5
#define CLE_BIT BIT1
#define CEn_BIT BIT0
#if CLE_PORT!=CEn_PORT
# error //Need to fix same port optimizations below
#endif

#define REn_PORT 6
#define REn_BIT BIT3

#define R_Bn_PORT J
#define WPn_PORT J
#define ALE_PORT J
#define WEn_PORT J
#if (R_Bn_PORT!=WPn_PORT) || (ALE_PORT!=WPn_PORT) || (WEn_PORT!=WPn_PORT)
# error //Need to fix same port optimizations below
#endif
#define R_Bn_BIT BIT3
#define WPn_BIT BIT2
#define ALE_BIT BIT1
#define WEn_BIT BIT0
#else
# error //Need to correct pin mappings
#endif

#define ROWBYTES (geom.addresscycles&0x0f)
typedef long row_t;
struct nandreq nand_state;

static struct nandgeom {
	uint32_t bytesperpage;
	uint32_t sparebytesperpage;
	uint32_t bytesperpartialpage;
	uint32_t sparebytesperpartialpage;
	uint32_t pagesperblock;
	uint32_t blocksperlun;
	uint8_t luns, addresscycles;
} geom;
static int colbits, rowbits;

/*static inline*/ void nand_open(void) {
	P5OUT |= CEn_BIT;
	P5DIR |= CEn_BIT;

	// Disable FPGA->MSP command path
	P1IE = 0;
	P1IFG = 0;

	P1DIR = 0;
	P1REN = 0;
	P6OUT |= REn_BIT;
	P6DIR |= REn_BIT;
	PJOUT |= WEn_BIT|R_Bn_BIT;
	PJOUT &= ~(WPn_BIT|ALE_BIT);
	PJREN |= R_Bn_BIT;
	PJDIR |= WEn_BIT|WPn_BIT|ALE_BIT;
	P5OUT &= ~(CLE_BIT|CEn_BIT);
	P5DIR |= CLE_BIT|CEn_BIT;
}
/*static inline*/ void nand_close(void) {
	// Release, but keep pullup for R/Bn and CEn and pulldown for WPn
	P1DIR = 0;
	P5OUT |= CEn_BIT;
	P5REN |= CEn_BIT;
	P5DIR &= ~(CLE_BIT|CEn_BIT);
	P6DIR &= ~REn_BIT;
	PJOUT &= ~WPn_BIT;
	PJREN |= WPn_BIT;
	PJDIR &= ~(WEn_BIT|WPn_BIT|ALE_BIT);

	// Enable FPGA->MSP command path
	P1IES = 0;
	P1IFG = 0;
	P1IE = BIT7;
}

static inline void nand_enable_write(void) {
	PJOUT |= WPn_BIT;
}
static inline void nand_disable_write(void) {
	PJOUT &= ~WPn_BIT;
}

static inline void nand_write_byte(char data) {
	P1OUT = data;
	P1DIR = 0xff;
	PJOUT &= ~WEn_BIT;
	PJOUT |= WEn_BIT;
}

static inline void nand_ALE(int ale) {
	if (ale)
		PJOUT |= ALE_BIT;
	else
		PJOUT &= ~ALE_BIT;
}

static inline void nand_CLE(int cle) {
	if (cle)
		P5OUT |= CLE_BIT;
	else
		P5OUT &= ~CLE_BIT;
}

static void ordb3_nand_write_buf(const char *buf, int len) {
	P1DIR = 0xff;
	while (len--) {
		P1OUT = *buf++;
		PJOUT &= ~WEn_BIT;
		PJOUT |= WEn_BIT;
	}
}

static void ordb3_nand_read_buf(char *buf, int len) {
	P1DIR = 0x00;
	while (len--) {
		P6OUT &= ~REn_BIT;
		*buf++ = P1IN;
		P6OUT |= REn_BIT;
	}
}

#if 0
#ifndef EFAULT
#define EFAULT 14
#endif
static int nand_verify_buf(const char *buf, int len) {
	P1DIR = 0x00;
	while (len--) {
		uint8_t val;
		P6OUT &= ~REn_BIT;
		val=P1IN;
		P6OUT |= REn_BIT;
		if (*buf++ != val)
			return -EFAULT;
	}
	return 0;
}
#endif

static uint8_t ordb3_nand_read_byte(void) {
	char val;
	P1DIR = 0x00;
	P6OUT &= ~REn_BIT;
	val=P1IN;
	P6OUT |= REn_BIT;
	return val;
}

int nand_ready(void) {
	return PJIN & R_Bn_BIT;
}

/* Default function uses a command instead */
static void select_chip(int chip) {
	if (chip==0) {
		P5OUT &= ~CEn_BIT;
	} else {
		P5OUT |= CEn_BIT;
	}
}

char nand_read_status(void) {
	nand_CLE(1);
	nand_write_byte(0x70);
	nand_CLE(0);
	return ordb3_nand_read_byte();
}

#if 0 /* Better to leave this to host? Firmware is not planned to write on its own. */
int nand_erase_row(row_t row) {
	long timeout;
	int i;

	for (timeout=-1; timeout && !nand_ready(); --timeout)
		;  // Wait until flash chip ready
	if (!nand_ready())
		return 0;
	nand_CLE(1);
	nand_write_byte(0x60);
	nand_CLE(0);
	nand_ALE(1);
	for (i=0; i<ROWBYTES; i++) {
		nand_write_byte(row&0xff);
		row>>=8;
	}
	nand_ALE(0);
	nand_CLE(1);
	nand_write_byte(0xd0);
	nand_CLE(0);
	for (timeout=-1; timeout && !nand_ready(); --timeout)
		;  // Wait until flash chip ready
	if (!nand_ready())
		return 0;  // Timeout
	return !(nand_read_status()&1);  // Fail not set
}
#endif

int nand_probe(char *buf, int size) {
	int tries=1+5, i, timeout;

	if (size<4+1+32)
		return 0;  /* Caller error */

	nand_open();

	nand_state.addr_bytes=0;
	nand_state.writelen=0;
	nand_state.readlen=0;


	select_chip(0);
	
	/* Wait for initial ready */
	for (timeout=-1; timeout && !nand_ready(); --timeout)
		;  // Wait until flash chip ready
	if (!nand_ready())
		return 0;

	/* Perform reset */
	nand_CLE(1);
	nand_write_byte(0xff);
	nand_CLE(0);

	/* Wait for reset to finish */
	for (timeout=-1; timeout && !nand_ready(); --timeout)
		;  // Wait until flash chip ready
	if (!nand_ready())
		return 0;

	/* Read ID */
	nand_CLE(1);
	nand_write_byte(0x90);
	nand_CLE(0);
	nand_ALE(1);
	nand_write_byte(0x20);
	nand_ALE(0);
	ordb3_nand_read_buf(buf, 4);
	
	if (!(buf[0]=='O' &&
	      buf[1]=='N' &&
	      buf[2]=='F' &&
	      buf[3]=='I')) {
		buf[0]='F';
		buf[1]='a';
		buf[2]='i';
		buf[3]='l';
		return 4;
	}

	do {
		/* Read ID vendor - to identify chip */
		nand_CLE(1);
		nand_write_byte(0x90);  // Read ID
		nand_CLE(0);
		nand_ALE(1);
		nand_write_byte(0x00);  // manufacturer ID region
		nand_ALE(0);
		ordb3_nand_read_buf(buf+4, 5);

		if (!(buf[4]==0x2c && buf[5]==0xda)) {
			// MT29F1G08ABADA is f1, MT29F2G08ABAEAH4 is da
			buf[4]='!';
			break;  // Not the known chip, don't poke at vendor specific feature 
		}

		if (buf[4+4]&0x80)
			break;  // ECC is enabled

		/* ECC is not enabled, try to enable it */
		nand_CLE(1);
		nand_write_byte(0xef);  // set features
		nand_CLE(0);
		nand_ALE(1);
		nand_write_byte(0x90);  // Micron array operation mode
		nand_ALE(0);
		nand_write_byte(0x08);  // Enable internal ECC
		nand_write_byte(0x00);
		nand_write_byte(0x00);
		nand_write_byte(0x00);
		for (timeout=-1; timeout && !nand_ready(); --timeout)
			;  // Wait until flash chip ready
		if (!nand_ready())
			return 5;
	} while (--tries);

	/* We have now initialized and hopefully enabled ECC. Read the description */
	nand_CLE(1);
	nand_write_byte(0xec);  // Read parameter page
	nand_CLE(0);
	nand_ALE(1);
	nand_write_byte(0x00);
	nand_ALE(0);
	for (timeout=-1; timeout && !nand_ready(); --timeout)
		;  // Wait until flash chip ready
	if (!nand_ready())
		return 5;
	// Skip bytes until human readable manufacturer and model
	for (i=0; i<32; i++) {
		ordb3_nand_read_byte();
	}
	// Read ID strings
	ordb3_nand_read_buf(buf+4+1, 32);
	for (i=64; i<80; i++) {
		ordb3_nand_read_byte();
	}
	ordb3_nand_read_buf((void*)&geom, sizeof geom);
	colbits=0;
	for (i=1; i<geom.pagesperblock; i<<=1)
		colbits++;
	rowbits=0;
	for (i=1; i<geom.blocksperlun; i<<=1)
		rowbits++;
	for (i=1; i<geom.luns; i<<=1)
		rowbits++;
	return 4+1+32;
}

static int nandreport_size;
static char nandreport[61];
void Do_NAND_Probe(void) {
	nandreport_size = nand_probe(nandreport, sizeof nandreport);
}


#ifdef LIBXSVF
/* XSVF player connection */

#define MAXBLOCKS 32
struct xsvfnand_state {
	//enum { disconnect, idle, reading, erasing, writing } state;
	long blocks[MAXBLOCKS];	// List of blocks holding XSVF data
	int bytesleftinpage, pageinblock, blockinlist;
} xsvf_nand_state;

// TODO: Find out if libxsvf might be improved to support async reading.
// (in short: not easily. it does read in command chunks though, so 
// perhaps we can break the loop.)
enum cache_mode { Uncached=0x30, Cached=0x31, Last=0x3f };
static void nand_loadpage(uint32_t page, enum cache_mode mode) {
	long timeout;
	/* Column address (byte in page) first, row address (page) second */
	int i;

	for (timeout=-1; timeout && !nand_ready(); --timeout)
		;  // Wait until flash chip ready
	nand_CLE(1);
	nand_write_byte(0x00);  // Read mode
	nand_CLE(0);
	nand_ALE(1);
	for (i=geom.addresscycles>>4; i--; )
		nand_write_byte(0);
	for (i=geom.addresscycles&0x0f; i--; page>>=8)
		nand_write_byte(page);
	nand_ALE(0);
	nand_CLE(1);
	nand_write_byte(mode);
	nand_CLE(0);
	/* At this point, the page is being fetched from flash. 
	   Wait for R/Bn before reading data, and check status register
	   for ECC failures. If cached, the previously fetched page is 
	   readable currently. */
	for (timeout=-1; timeout && !nand_ready(); --timeout)
		;  // Wait until flash chip ready
}

static int xsvf_setup(struct libxsvf_host *h) {
	nand_open();

	xsvf_nand_state.bytesleftinpage=geom.bytesperpage;
	xsvf_nand_state.pageinblock=2;
	xsvf_nand_state.blockinlist=0;
	
	// Load page 0 for block list
	nand_loadpage(0, Uncached);
	ordb3_nand_read_buf((void*)&xsvf_nand_state.blocks,
			    sizeof(xsvf_nand_state.blocks));

	/* Sanity check: first block is valid and not 0 */
	if (xsvf_nand_state.blocks[0]<=0 ||
	    xsvf_nand_state.blocks[0]>=geom.blocksperlun*geom.luns)
		return -1;

	/* Start loading first page */
	nand_loadpage(xsvf_nand_state.blocks[0]*geom.pagesperblock,Cached);
	/* Start loading second page */
	nand_loadpage(xsvf_nand_state.blocks[0]*geom.pagesperblock+1,Cached);
	// At this point, the first page should be ready to read. 
	// The second page is loaded (or will be).
	return 0;
}

static int xsvf_shutdown(struct libxsvf_host *h) {
	nand_close();
	return 0;
}

static void xsvf_udelay(struct libxsvf_host *h, long usecs, int tms, long num_tck) {
	while (num_tck) {
		pulse_tck(h,tms,-1,-1,0,0);
	}
}

static int xsvf_getbyte(struct libxsvf_host *h) {
	if (!--xsvf_nand_state.bytesleftinpage) {
		/* Need to start on a new page */
		int bil=xsvf_nand_state.blockinlist;
		nand_loadpage(xsvf_nand_state.blocks[bil]*geom.pagesperblock+
			      xsvf_nand_state.pageinblock, Cached);
		xsvf_nand_state.bytesleftinpage=geom.bytesperpage;
		if (++xsvf_nand_state.pageinblock>=geom.pagesperblock) {
			/* New block */
			if (bil<MAXBLOCKS) {
				xsvf_nand_state.blockinlist=bil+1;
				xsvf_nand_state.pageinblock=0;
			} else {
				/* End of block list, but we still have a page on its way. */
				xsvf_nand_state.pageinblock--;  // Repeat last page
			}
		}
	}
	return ordb3_nand_read_byte();
}

void xsvf_report_error(struct libxsvf_host *h, const char *file, int line, const char *message) {
	/* Nothing to be done as yet */
}

void *xsvf_realloc(struct libxsvf_host *h, void *ptr, int size, enum libxsvf_mem which) {
	/* There are 36 different possible buffers, we may well make them static */
	/* Actually in use are only 5 or so? */
	static int sizes[LIBXSVF_MEM_NUM];
	if (size>sizes[which]) {
		void *newptr=malloc(size);
		if (ptr) {
			if (newptr)
				memcpy(newptr, ptr, sizes[which]);
			free(ptr);
		}
		sizes[which]=newptr?size:0;
		return newptr;
	} else
		return ptr;
}

const struct libxsvf_host xsvf_host={
	.setup=xsvf_setup,
	.shutdown=xsvf_shutdown,
	.udelay=xsvf_udelay,
	.getbyte=xsvf_getbyte,
	.pulse_tck=pulse_tck,
	.report_error=xsvf_report_error,
	.realloc=xsvf_realloc,
};

#endif

void process_nandreq(void) {
	nand_open();
	nand_CLE(1);
	nand_write_byte(nand_state.cmd);
	nand_CLE(0);
}
void process_nanddata(char *data, int len) {
	/* Note that trailing commands are not allowed! */
	if (len && nand_state.addr_bytes) {
		int alen = nand_state.addr_bytes;
		if (len<alen) alen=len;

		nand_ALE(1);
		ordb3_nand_write_buf(data, alen);
		nand_ALE(0);

		len-=alen;
		nand_state.addr_bytes-=alen;
		data+=alen;
	}
	if (len && nand_state.writelen) {
		int wrlen = nand_state.writelen;
		if (len<wrlen) wrlen=len;
		ordb3_nand_write_buf(data, wrlen);
		nand_state.writelen-=wrlen;
	}
}
int produce_nanddata(char *data, int len) {
	if (len>nand_state.readlen)
		len=nand_state.readlen;
	if (len) {
		ordb3_nand_read_buf(data, len);
		nand_state.readlen-=len;
	}
	return len;
}

/*
 * Port 1 interrupt vector
 *
 * Triggered by NAND D7 going high (edge sensitive).
 * If CEn is also high, that indicates a command for the MSP430.
 */
extern volatile uint8_t bCommand;
#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR (void)
{
	uint16_t iv = P1IV;  // Read interrupt vector register to clear IFG
	if (!(P5DIR&CEn_BIT) && (P5IN&CEn_BIT)) {
		// Could add ALE and CLE both high, which is invalid for NAND.
		// That is not for the flash, read it
		bCommand = P1IN | BIT7;  // Use bit 7 to indicate new command
		// Wake main thread up
		WAKEUP_IRQ(LPM3_bits);
	}
}

