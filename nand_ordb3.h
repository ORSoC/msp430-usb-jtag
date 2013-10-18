/* When all three lengths are zero, we expect a whole nandreq.
   Then addr_bytes worth of address, writelen worth of write data,
   finally expect to produce readlen bytes of return data. */

/* Need to extend protocol with a result: timeout, finished, data.
   Might also want a signature word for synchronization. */

extern struct nandreq {
	uint8_t cmd;
	uint8_t addr_bytes;
	uint16_t writelen, readlen;
} nand_state;

/* Indicates that we're waiting for a new request. */
void nand_close(void);

static inline int expect_nandreq(void) {
	int ret = nand_state.addr_bytes==0 && nand_state.writelen==0 && nand_state.readlen==0;
	if (ret)
		nand_close();
	return ret;
}
/* Measure how much data we're waiting for from USB. */
static inline int expect_nanddata(void) {
	return nand_state.addr_bytes+nand_state.writelen;
}
/* R/Bn line; each step waits for it to be high. */
extern int nand_ready(void);
/* Tell the NAND block to process a fresh request. */
extern void process_nandreq(void);
/* Feed the NAND block data from USB, be it address or write */
extern void process_nanddata(char *data, int len);
/* Read data from NAND to send over USB */
extern int produce_nanddata(char *data, int maxlen);
/* Program FPGA from NAND data */
extern int program_fpga_from_nand(void);
extern void nand_enable_write(void);
extern void nand_disable_write(void);
