# openCH I2C: an address nobody answers on, and whether the bus survives it

Board: openCH CH32V307, WCH-Link CH549F. There is no UART evidence channel on this board that
can decide anything (the virtual COM port's ring drops and reorders bytes), so every reading here
comes from the debugger: `nm` resolves the published counters in the ELF, OpenOCD halts and reads
them. Image programmed with `program <elf> verify` → `Verified OK`, then `reset`.

## What was asked

`xy_hal_i2c.c` looks correct by reading: `i2c_wait_event()` maps an address NACK (AF) to
`XY_HAL_ERROR_NOT_FOUND`, maps BERR/ARLO/OVR to `XY_HAL_ERROR_IO`, and on failure calls
`i2c_abort()` to issue a STOP and clear the flags. But none of it had ever been observed on the
board, and this is the exact shape of a fail-open defect: a missing device reported as success, or
a latched AF that leaves every later transfer broken while the caller sees only the first error.

Stage 14 of the stall probe performs the transfer a real bus performs when a part is absent:
`xy_hal_i2c_mem_read(I2C2, 0x7F, 0x75, ...)` — nothing answers at 0x7F — immediately followed by
the same read against the MPU6050 at 0x68, which must still answer `0x68`. It publishes
`xy_probe_i2c_nak_code`, `xy_probe_i2c_recover_code` and `xy_probe_i2c_recover_value`.

| reading | value | meaning |
|---|---|---|
| `xy_probe_i2c_nak_code` | `0xfffffffa` = −6 | `XY_HAL_ERROR_NOT_FOUND`, not success, not a bare timeout |
| `xy_probe_i2c_recover_code` | `0x0` | the very next transfer succeeded — AF did not stay latched |
| `xy_probe_i2c_recover_value` | `0x68` | and it read the right part |
| `xy_probe_stage` | `15` | every measurement published |
| `xy_probe_i2c_ops` / `_fails` | `4` / `0` | the pre-existing I2C reads, unaffected |
| `xTickCount` | `0x11ce → 0x1bda` | scheduler still advancing across the run |

```
osal_stall_probe_ok stage=15 loops=3 wait=0x80000000 get=0xfffffffe uart=7/7 i2c=4/4 \
  nak=0xfffffffa recover=0x68 recv_ms=10 ticks=0x11ce->0x1bda
```

## Making the judgement fail first

A check that has never been seen to fail is decoration, so the same validator was run against an
image that *should* be rejected. One temporary local edit (never committed) pointed the "unanswered"
read at `0x68`, the address that does answer, leaving everything else byte-identical. Rebuilt,
`Verified OK`, same validator:

```
osal_stall_probe failed:
- an unanswered I2C address returned 0, expected 0xfffffffa (XY_HAL_ERROR_NOT_FOUND)
```

exit code 1. The source was then restored from a byte-exact backup (`sha256 92a0d47f…` re-checked
against the pre-RED file), the rebuild reproduced the green ELF **bit-for-bit**
(`5aa034f9…`), it was reflashed, and the validator returned green again. The RED is a manual
temporary change and will not re-run automatically; only the contract tests
(`tests/unit/hal_component/test_opench_osal_probe_judge.py`) and the probe guard are permanent.

## Boundaries

One board, one image, one run per state. The NAK is an address-NACK from an absent address — this
is **not** clock stretching, a busy slave, a bus held low, or arbitration loss; the I2C timeout
path, repeated-start failures and recovery from a bus that is already broken at power-on are not
covered. `uart 7/7` and the advancing tick are liveness readings from the same run, not a stability
claim. The image is a diagnostic probe, not a product image.
