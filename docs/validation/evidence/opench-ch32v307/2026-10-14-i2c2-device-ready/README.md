# openCH I2C2 device-ready HIL

Register-level diagnosis showed that an address probe must classify the address phase directly from STAR1 ADDR versus AF. The generic transmitter event check includes TXE and reads STAR2 as part of event evaluation, which made it unsuitable for a bounded ACK-only probe. The public device-ready path now waits for ADDR/AF explicitly, clears ADDR by reading STAR2, preserves NOT_FOUND across cleanup, and waits for STOP/BUSY completion.

WCH-Link program, verify and reset passed. Thirty-two consecutive iterations each verified that unused address `0x7F` returns `NOT_FOUND`, MPU6050 address `0x68` returns `OK`, and the immediately following WHO_AM_I transaction returns `0x68`.

This is bounded single-master address ACK/NACK and post-NACK recovery evidence. It does not prove multi-master arbitration, forced electrical stuck-bus recovery, clock stretching limits, injected line faults, DMA/IRQ operation or endurance.
