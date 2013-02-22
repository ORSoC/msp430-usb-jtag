extern struct usbblaster_state {
	//uint8_t *bytes_in, *bytes_out;
	uint8_t bytes_to_shift, read;
} usb_jtag_state;

// This implements a lot of the usb blaster protocol (cpld side). 
// The return value should be send to host if .read unless .bytes_to_shift went from 0 to non-0
uint8_t usbblaster_byte(uint8_t fromhost);
void jtag_init(void);
