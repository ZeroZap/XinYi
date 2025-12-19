#ifndef _SD_PORT_H_
#define _SD_PORT_H_


int32_t sd_spi_init(uint8_t speed_mode);
int32_t sd_spi_deinit(void);
int32_t sd_spi_read_bytes(uint8_t *data, uint16_t len);
int32_t sd_spi_write_bytes(uint8_t *data, uint16_t len);
uint8_t sd_spi_rw_byte(uint8_t data);
#endif