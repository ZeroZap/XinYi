# Pandora AP3216C bounded stimulus record — 2026-09-06

## Result

- Status: `AP3216C_BOUNDED_IR_RESPONSE_B1`
- Evidence boundary: static AP3216C B1 plus bounded IR stimulus-response B1.
- Board: Pandora STM32L475VE V2.4, U8 AP3216C.
- Firmware: `f320087cbcbe5d0ae107b27b96b42528438219e2`, continuous ALS+PS mode `0x03`.
- Bus: board-owned software I2C3, SCL PC0 / SDA PC1, 7-bit address `0x1E`.

## Retained evidence

- Raw capture: `evidence/pandora-stm32l475/2026-09-06/ap3216c-user-dynamic-f320087c.raw`
- Raw size: 16,812 bytes
- Raw SHA-256: `7fc69e9fab215a547c3b08f251fdce0379d33c75ebbea312b5f4ff0c9c43c835`
- Machine metadata: `evidence/pandora-stm32l475/2026-09-06/ap3216c-user-dynamic-f320087c.json`
- Samples: 244; unique complete sample lines: 139; unique raw register tuples: 120.
- Error/malformed lines: 0/0.
- Observed ranges: ALS 10–11 lux, PS 0–15 raw, IR 0–514 raw.
- First 50-sample IR median: 2.5.
- Maximum later 50-sample rolling IR median: 257.0; delta 254.5.

The capture starts in a low-IR window and later contains a sustained high-IR window. The validator
requires at least 100 samples, 50 unique complete lines, no error or malformed lines, and an IR median
increase of at least 128 across independent 50-sample windows. Negative Host cases reject malformed,
error-bearing, short, and static captures.

## Evidence boundary

This closes only a bounded qualitative IR response on the existing AP3216C path. It does **not** prove
quantitative ALS response, quantitative proximity response, interrupt-pin behavior, optical accuracy,
calibration, distance thresholds, or NACK/recovery B2. ALS remained 10–11 and PS 0–15, so neither is
promoted as a controlled response result.
