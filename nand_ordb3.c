/* Routines to access NAND Flash memory on ORDB3 from MSP430 */

#include <linux/mtd/nand.h>
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

#define P(n,func) P##n##_##func
#define SET_PORT(port, bits) P(port,OUT) |= (bits)
#define CLR_PORT(port, bits) P(port,OUT) &= ~(bits)
#define RD_PORT(port, bits) P(port,IN) & (bits)

static inline void nand_init(void) {
	SET_PORT(CEn_PORT,CEn_BIT);
	P(CEn_PORT,DIR) |= CEn_BIT;

	P(DATA_PORT,DIR) = 0;
	P(DATA_PORT,REN) = 0;
	SET_PORT(REn_PORT, REn_BIT);
	P(REn_PORT,DIR) |= REn_BIT;
	SET_PORT(WPn_PORT, WEn_BIT|R_Bn_BIT);
	CLR_PORT(WPn_PORT, WPn_BIT|ALE_BIT);
	P(WPn_PORT,REN) |= R_Bn_BIT;
	P(WPn_PORT,DIR) |= WEn_BIT|WPn_BIT|ALE_BIT;
	CLR_PORT(CLE_PORT,CLE_BIT|CEn_BIT);
	P(CLE_PORT,DIR) |= CLE_BIT|CEn_BIT;
}
static inline void nand_close(void) {
	// Release, but keep pullup for R/Bn and CEn and pulldown for WPn
	P(DATA_PORT,DIR) = 0;
	CLR_PORT(CEn_PORT,CEn_BIT);
	P(CEn_PORT,REN) |= CEn_BIT;
	P(CLE_PORT,DIR) &= ~(CLE_BIT|CEn_BIT);
	P(REn_PORT,DIR) &= ~REn_BIT;
	CLR_PORT(WPn_PORT,WPn_BIT);
	P(WPn_PORT,REN) |= WPn_BIT;
	P(WPn_PORT,DIR) &= ~(WEn_BIT|WPn_BIT|ALE_BIT);
}

static inline void nand_enable_write(void) {
	SET_PORT(WPn_PORT,WPn_BIT);
}
static inline void nand_disable_write(void) {
	CLR_PORT(WPn_PORT,WPn_BIT);
}

static inline nand_write_byte(uint8_t data) {
	P(DATA_PORT,PORT) = data;
	P(DATA_PORT,DIR) = 0xff;
	CLR_PORT(WEn_PORT,WEn_BIT);
	SET_PORT(WEn_PORT,WEn_BIT);
}

/* Functions ready for use from Linux/U-Boot MTD style code */

static void nand_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len) {
	P(DATA_PORT,DIR) = 0xff;
	while (len--) {
		P(DATA_PORT,PORT) = *buf++;
		CLR_PORT(WEn_PORT,WEn_BIT);
		SET_PORT(WEn_PORT,WEn_BIT);
	}
}

static void nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len) {
	P(DATA_PORT,DIR) = 0x00;
	while (len--) {
		CLR_PORT(REn_PORT,WEn_BIT);
		*buf++ = P(DATA_PORT,PIN);
		SET_PORT(REn_PORT,WEn_BIT);
	}
}

static void nand_verify_buf(struct mtd_info *mtd, const uint8_t *buf, int len) {
	P(DATA_PORT,DIR) = 0x00;
	while (len--) {
		CLR_PORT(REn_PORT,WEn_BIT);
		if (*buf++ != P(DATA_PORT,PIN))
			return -EFAULT;
		SET_PORT(REn_PORT,WEn_BIT);
	}
	return 0;
}

static uint8_t nand_read_byte(struct mtd_info *mtd) {
	char val;
	P(DATA_PORT,DIR) = 0x00;
	CLR_PORT(REn_PORT,REn_BIT);
	val = P(DATA_PORT,IN);
	SET_PORT(REn_PORT,REn_BIT);
	return val;
}

static int nand_ready(struct mtd_info *mtd) {
	return RD_PORT(R_Bn_PORT, R_Bn_BIT);
}

static void cmd_ctrl(struct mtd_info *mtd, int cmd, unsigned int ctrl) {
	if (cmd==NAND_CMD_NONE)
		return;
	if (ctrl & NAND_CLE) {
		SET_PORT(CLE_PORT,CLE_PIN);
		nand_write_byte(cmd);
		CLR_PORT(CLE_PORT,CLE_PIN);
	} else {
		SET_PORT(ALE_PORT,ALE_PIN);
		nand_write_byte(cmd);
		CLR_PORT(ALE_PORT,ALE_PIN);
	}
}

/* Default function uses a command instead */
static void select_chip(struct mtd_info *mtd, int chip) {
	if (chip==0) {
		CLR_PORT(CEn_PORT, CEn_BIT);
	} else {
		SET_PORT(CEn_PORT, CEn_BIT);
	}
}

int board_nand_init(struct nand_chip *nand) {
	nand.read_byte=nand_read_byte;
	nand.write_buf=nand_write_buf;
	nand.read_buf=nand_read_buf;
	nand.verify_buf=nand_verify_buf;
	nand.select_chip=select_chip;
	nand.cmd_ctrl=cmd_ctrl;
	return 0;
}
