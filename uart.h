#define BRCLK_FREQ 24000000
#define BRCLK_SEL UCSSEL__ACLK
#define UART_BAUD 115200

void init_uart(void);
void handle_uart(void);
