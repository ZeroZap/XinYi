# FreeRTOS Cortex-M4F Port Provenance

XinYi carries the two files in this directory as project-owned integration
inputs rather than editing `third_party/` or the STM32CubeL4 vendor tree.

- Upstream: <https://github.com/FreeRTOS/FreeRTOS-Kernel>
- Tag: `V10.4.6`
- Commit: `a4b28e35103d699edf074dfff4835921b481b301`
- Upstream path: `portable/GCC/ARM_CM4F/`
- Imported: 2026-09-03
- License: MIT (upstream file headers)

Pinned SHA-256:

```text
8aa709759655b1711b28ea943b232a8c7f3ddbbb06f6c05f4328c681b223a734  port.c
a0cc3996ab10e9dce31b6665502e7a1328d134383d467b16e0aab205a7b07ccb  portmacro.h
```

The compile probe validates source compatibility only. Pandora interrupt-vector
ownership, clock accuracy, runtime scheduling, ISR-to-task behavior, and
hardware execution remain unverified.
