# Pandora STM32L475VE — Device open/close registry reference accounting

`xy_device_open()` used to bump only `dev->ref_count`, so the registry/PM
reference count stayed at zero. An open, in-use device could therefore be
auto-suspended by `xy_device_pm_check()`, `xy_device_sleep()` returned
`XY_DEVICE_OK` for it, and `xy_device_registry_unregister()` accepted removing
it from underneath its owner. `xy_device_close()` also decremented the count
before calling `api->close`, so a driver close failure left a half-retired
device that could not be retried, and the first close of a shared open tore the
driver down for the remaining holder.

Open now takes the registry reference first (waking a sleeping device and
rolling back on failure); close drops only its own reference until the last
holder, keeps the reference and `XY_DEV_STATE_OPENED` when the driver close
fails, and fails closed with `XY_ERROR_NOT_READY` when the caller holds none.

## Evidence boundary

This directory is a **bounded B1 boot/regression observation** on the reference
board, not runtime evidence for the new close semantics:

- The Pandora FreeRTOS image (`pandora_stm32l475_rtos`) compiles
  `components/device/src/xy_device.c` and `xy_device_core.c` directly, so the
  programmed image contains the changed code and its `xy_device_find()` /
  registry path runs (`OSAL_DEVICE_LOOKUP`).
- The image never calls `xy_device_open()`/`xy_device_close()`; the open/close
  contract itself is proven by the Host `device_framework` CTest
  (`xy_device_registry_ref_count`, `XY_DEVICE_BUSY` sleep/unregister, shared
  reference and retryable-close assertions).
- Only one boot in a 20 s window was captured. The later IPC ISR stress markers
  are outside the window, so no sustained-ISR, timing, concurrency, throughput
  or endurance claim follows.

## Result

| field | value |
|-------|-------|
| source commit | `5e0cd22242567d34858d5dcbe2f99e8d77c21bea` |
| image | `pandora_stm32l475_rtos.bin`, 42396 B, sha256 `41f1039554eb01d1a310b4ba264b9ec59fc75c68a31b90bea0465bada897043c` |
| probe | ST-Link V2.1 V2J24S11 serial `066AFF313933554D43244015`, chipid `0x415` |
| UART | CP2102 `/dev/ttyUSB0`, 115200 8N1, armed before `st-flash reset` |
| capture | 383 bytes, `PANDORA STM32L475VE XINYI OSAL FREERTOS READY` + `FIRMWARE_COMMIT 5e0cd222…` |
| error markers | 0 |
| read-back | 524288 B read back after programming; image prefix byte-identical |
| restore | pre-run 512 KiB image restored, read back and `cmp`-identical (`d3a4e156…`) |

Observed marker order in `uart.txt`:

```text
PANDORA STM32L475VE XINYI OSAL FREERTOS READY
FIRMWARE_COMMIT 5e0cd22242567d34858d5dcbe2f99e8d77c21bea
OSAL_STRESS_READY
OSAL_TIM6_IRQ_TAKE
OSAL_MUTEX_FAST
OSAL_TASK_FAST
OSAL_QUEUE_SEND
OSAL_EVENT_SET
OSAL_IPC_SEND
OSAL_PM_TICK
OSAL_SEM_TAKE
OSAL_EVENT_WAIT
OSAL_QUEUE_RECV
OSAL_DEVICE_LOOKUP
[I] OSAL_TRACE_DELIVER
OSAL_IPC_DELIVER
OSAL_MUTEX_SLOW
OSAL_TASK_SLOW
```

## File map

- `uart.txt` — raw capture for the single programmed boot.
- `result.json` — machine-readable record: identity, hashes, markers, restore
  verification and explicit limitations.
