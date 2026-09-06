#ifndef PANDORA_SOFT_I2C_H
#define PANDORA_SOFT_I2C_H

#include <stdint.h>

void *pandora_soft_i2c3_init(void);
void *pandora_soft_i2c4_init(void);
int pandora_soft_i2c_probe(void *i2c, uint8_t dev_addr);

/* Compatibility name for the existing board AHT10 path (software-I2C4). */
void *pandora_soft_i2c_init(void);

#endif
