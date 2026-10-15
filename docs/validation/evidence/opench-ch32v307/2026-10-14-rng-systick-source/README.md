# openCH RNG SysTick-source HIL

The WCH RNG blocking APIs now enforce timeout in elapsed milliseconds instead of a CPU-dependent polling-loop count. The conversion follows the active SysTick source: HCLK when STCLK is selected and HCLK/8 otherwise. A zero timeout performs the immediate availability check and returns timeout if data is still busy.

WCH-Link program, verify and reset passed. The same image completed 32 rounds under HCLK and another 32 under HCLK/8. Each round requested 16 hardware RNG words with a 100 ms per-word timeout and required at least one word to differ from the first, for 1,024 words total without error.

This proves bounded RNG data-ready polling and timeout-clock integration. The variation check is not a statistical randomness, entropy, health-test, cryptographic-strength, bias or certification assessment.
