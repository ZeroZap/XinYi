# openCH CH32V307 software reset and reset cause

A backup-domain cookie gates a two-boot transaction: arm cookie, invoke `xy_hal_sys_software_reset()`, then prove the second boot observes both the cookie and the HAL software-reset reason. The reset flags are cleared and read back as none.

Boundary: this does not validate POR/brownout, external reset pin, watchdog reset attribution, power-loss backup retention, repeated reset endurance, bootloader interaction or reset latency.
