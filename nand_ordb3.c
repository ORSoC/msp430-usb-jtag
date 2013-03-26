/* Routines to access NAND Flash memory on ORDB3 from MSP430 */

#include <USB_API/USB_Common/types.h>
#include <USB_API/USB_HID_API/UsbHid.h>

#include <stdint.h>
#include <msp430.h>

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

#define ROWBYTES 3
typedef long row_t;

static inline void nand_init(void) {
	P5OUT |= CEn_BIT;
	P5DIR |= CEn_BIT;

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
static inline void nand_close(void) {
	// Release, but keep pullup for R/Bn and CEn and pulldown for WPn
	P1DIR = 0;
	P5OUT |= CEn_BIT;
	P5REN |= CEn_BIT;
	P5DIR &= ~(CLE_BIT|CEn_BIT);
	P6DIR &= ~REn_BIT;
	PJOUT &= ~WPn_BIT;
	PJREN |= WPn_BIT;
	PJDIR &= ~(WEn_BIT|WPn_BIT|ALE_BIT);
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

static uint8_t ordb3_nand_read_byte(void) {
	char val;
	P1DIR = 0x00;
	P6OUT &= ~REn_BIT;
	val=P1IN;
	P6OUT |= REn_BIT;
	return val;
}

static int nand_ready(void) {
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

int nand_probe(char *buf, int size) {
	int tries=1+5, i, timeout;

	if (size<4+1+32)
		return 0;  /* Caller error */

	nand_init();

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
	return 4+1+32;
}

static int nandreport_size;
static char nandreport[61];
void Do_NAND_Probe(void) {
	nandreport_size = nand_probe(nandreport, sizeof nandreport);
}
void Report_NAND(int ifnum) {
	USBHID_sendData(nandreport,nandreport_size,ifnum);
}
