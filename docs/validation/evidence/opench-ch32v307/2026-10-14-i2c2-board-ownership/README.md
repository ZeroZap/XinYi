# openCH I2C2 board ownership HIL

The reusable WCH I2C HAL no longer enables GPIOB or writes PB10/PB11. The openCH board smoke now configures PB10/PB11 as AF open-drain, releases both output latches high, and then initializes I2C2.

WCH-Link program, verify and reset passed after this separation. The combined smoke verified MPU6050 WHO_AM_I, reversible AP3216C and ES8388 register transactions, and AHT10 address acknowledgement. PC, STM32L4, STM32U5 and CH32V307 builds plus 267 Host tests passed.

This proves the openCH board supplies the required pin setup without embedding board-specific GPIOs in the reusable peripheral HAL. Device-level protocol evidence remains in the dedicated AHT10, AP3216C, MPU6050 and ES8388 evidence directories.
