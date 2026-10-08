# openCH CH32V307 I2S2 master-TX runtime evidence

UART validation starts only at the unique boot identity emitted by the newly programmed image, then requires INIT, callback registration, START, READY and repeated TX completion markers. The smoke configures I2S2 at 48 kHz, Philips, 16-bit master TX and writes four samples per cycle through the canonical HAL. This proves controller configuration, TXE-driven data-register writes and callback dispatch. It does not prove physical BCLK/WS/SD waveforms, exact audio sample rate, external codec reception, DMA/IRQ receive, audio quality or endurance.
