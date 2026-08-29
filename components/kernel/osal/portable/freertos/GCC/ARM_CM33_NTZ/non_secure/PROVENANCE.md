# FreeRTOS Cortex-M33 Port Provenance

XinYi carries the four files in this directory as project-owned integration
inputs rather than editing `third_party/` or an MCU vendor tree.

- Upstream: <https://github.com/FreeRTOS/FreeRTOS-Kernel>
- Tag: `V10.4.6`
- Commit: `a4b28e35103d699edf074dfff4835921b481b301`
- Upstream path: `portable/GCC/ARM_CM33_NTZ/non_secure/`
- Imported: 2026-08-29
- License: MIT (upstream file headers)

Pinned SHA-256:

```text
ee7f9b85b1eefa5d676f2001c6cb95d369aebeca7bf1f63402f2c86be45b8fcd  port.c
12e9ea69dd56e3ae9eb18d1c36b3acd92009cb1f791880863bc6503d247c7ed6  portasm.c
3fc5034e40bd99234389bf7a6f9dba9222e5b2d4bdca171d4b543ffa71230e20  portasm.h
f034b6fcd66b6958f9a99c04e2610c35bdbfb982ebd92d136d10c335bc8ad9a2  portmacro.h
```

The compile probe validates source compatibility only. Board interrupt-vector
ownership, clock accuracy, runtime scheduling, ISR-to-task behavior, and
hardware execution remain unverified.
