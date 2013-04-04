extern struct usbblaster_state {
	//uint8_t *bytes_in, *bytes_out;
	uint8_t bytes_to_shift, read;
} usb_jtag_state;

// This implements a lot of the usb blaster protocol (cpld side). 
// The return value should be send to host if .read unless .bytes_to_shift went from 0 to non-0
uint8_t usbblaster_byte(uint8_t fromhost);
void jtag_init(void);
int usbblaster_process_buffer(uint8_t *buf, int len);

// libxsvf JTAG interface
struct libxsvf_host;
int pulse_tck(struct libxsvf_host *h, int tms, int tdi, int tdo, int rmask, int sync);
