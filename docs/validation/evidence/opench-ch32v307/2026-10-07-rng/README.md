# openCH CH32V307 hardware RNG runtime evidence

The image was independently programmed, verified and reset through the WCH-Link/MRS OpenOCD path. A five-second CH549 UART capture contained repeated `OPENCH_RNG_VARIATION_OK` markers and no error marker. This is bounded evidence that the CH32V307 hardware RNG becomes ready and consecutive samples vary; it is not a statistical, entropy-quality, cryptographic or endurance qualification.
