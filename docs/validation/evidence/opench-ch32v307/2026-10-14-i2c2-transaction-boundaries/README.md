# openCH I2C2 transaction-boundary HIL

The WCH I2C HAL now waits for BUSY to clear before every blocking public transaction, restores ACK before START, and waits for STOP/BUSY completion after transmit and memory-write paths. Device-ready success uses the same STOP/BUSY completion boundary.

WCH-Link program, verify and reset passed. The smoke completed 32 consecutive mixed iterations across MPU6050, AP3216C, ES8388 and AHT10, followed by a separate MPU6050 register-pointer master transmit and master receive pair. This covers 128 mixed transactions plus the explicit TX/RX boundary without an intervening reset or reinitialization.

This is bounded controller transaction-state evidence. It does not prove multi-master arbitration, electrically forced stuck-bus recovery, clock stretching limits, injected bus errors, DMA/IRQ operation, performance or endurance.
