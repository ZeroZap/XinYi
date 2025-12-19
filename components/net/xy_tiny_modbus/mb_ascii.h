
#define MB_ASCII_START_CHAR ":"
#define MB_ASCII_END_CHARS "\r\n"
size_t mb_ascii_init(uint32_t baudrate);

size_t mb_ascii_start(void);

size_t mb_ascii_stop(void);

size_t mb_ascii_deinit(void);

size_t mb_ascii2hex(void);

uint16_t mb_ascii_lrc(const uint8_t *pdata);