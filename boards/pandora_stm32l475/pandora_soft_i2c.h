#ifndef PANDORA_SOFT_I2C_H
#define PANDORA_SOFT_I2C_H

void *pandora_soft_i2c3_init(void);
void *pandora_soft_i2c4_init(void);

/* Compatibility name for the existing board AHT10 path (software-I2C4). */
void *pandora_soft_i2c_init(void);

#endif
