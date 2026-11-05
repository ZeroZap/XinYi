# PID on the Pandora board — first runtime evidence (2026-11-05)

## Why this slice

The PID component had Host tests but its Board column in the evidence matrix stayed
`pending`. The reason to run it on the target is specific, and it is not the
generic "run it on real hardware" argument:

**The two builds do not round the same way.** The controller is written entirely in
`float` — there is no `double` anywhere in `xy_pid.c`. The STM32L475VE has a
single-precision hardware FPU, so the target contracts the multiply-add in the
output sum into a single `vfma.f32` — **one** rounding. The x86-64 Host emits a
separate `mulss` and `addss` — **two** roundings. The same expression can differ in
its last bit between the two, and no Host test can show that.

**The elapsed interval has never been the real one.** The controller's `dt` comes
from `xy_os_tick_get()`, and the Host tests stub that with a plain variable. So the
number that drives the integral and the derivative has only ever been a
hard-coded one.

This image runs the controller against the **real FreeRTOS tick**, with a task that
steps the setpoint over time so `dt` is a genuine elapsed interval, and checks the
values the Host tests pin.

## What ran

Image `pandora_stm32l475_pid` (new, `boards/pandora_stm32l475/pid_main.c`), 20576 B,
SHA-256 `63cb0332c6eb031dcaccef4432f915a8278112491e196c18894043564996d064`, linked at
`0x08000000`.

Board: STM32L475VE, ST-Link V2.1 `V2J24S11` / `066AFF313933554D43244015`, chipid `0x415`.
UART: CP2102 `/dev/ttyUSB0`, 115200 8N1.

The write was snapshot-gated (`tools/pandora_hil_program.py run --address 0x08000000`):
`span_identical=true`, `unexpected_changes=0`.

### Marker sequence

```
PANDORA PID BOARD READY
FIRMWARE_COMMIT 1187e9a4bf407ab7738a45b6c986d3be218b1b25
PANDORA_PID_LOOP_OK_A
PANDORA_PID_WINDUP_OK_A
PANDORA_PID_CONTRACT_OK_A
PANDORA_PID_RESET_OK_A
B1_PID_BOARD_RUNTIME_PASS
```

Judged by `boards/pandora_stm32l475/validate_pid_capture.py`: `status=B1_PID_BOARD_RUNTIME_PASS`,
`checks_passed=4`, `failures=[]`, `error_steps=[]`.

| check | what it proves on the board |
|---|---|
| loop | the first compute establishes the baseline and does not count as an update; twenty stepped updates each count; the error is the setpoint minus the measurement; the output leaves zero and stays inside the configured limits; the proportional term alone is `kp * error`; the integral accumulates and is clamped; the derivative of a constant error stays at zero |
| windup | anti-windup clamps the integral at the output bound rather than letting it run away, and the output itself is clamped at the configured maximum |
| contract | a non-finite setpoint, a non-finite input and a non-finite measurement are each refused with the previous value kept; manual mode passes the previous output through |
| reset | reset returns the update count, the error, the integral and the derivative to their initial values |

## The image owns its tick source

This is the part worth stating plainly, because it is a trap the Host cannot show:

`xy_os_tick_get()` in the OSAL reads the **misc** tick counter, and nothing
increments it once the kernel owns SysTick. Left alone, it would read 0 forever, so
every elapsed interval would be zero, `dt` would never be positive, and the
controller would never update — a failure that looks exactly like a controller that
is simply not moving.

The image therefore overrides `xy_os_tick_get()` to return
`xy_os_kernel_get_tick_count()`, the same way the OSAL runtime image does.

The second trap is the delay: the image waits through `xy_os_delay()`, which yields
to the scheduler, and not through the bare-metal `xy_hal_delay_ms()`, which
reprograms SysTick's CMP and CTLR and would stop the kernel's tick for good.

## The capture is judged fail-closed and identity-bound

The validator requires the exact firmware commit that was flashed, the four check
markers in the order the image emits them, and the terminal marker last. Any error
marker — including the `*_ERROR_STEP_<n>` variant that names which check failed and
at which step — fails the capture and the step number is reported. A capture from
another build cannot be presented as this one.

The image's own fault handler prints nothing before it spins, so a crash is reported
by the *absence* of the terminal marker rather than by a marker. The board guard
requires the image to publish the same three post-mortem words the other board
images publish, and requires the stack-overflow hook to set them before it spins.

Eight cases cover the validator, registered as CTest `pandora_pid_capture_validator`.

## Boundary

This is runtime proof that the controller's arithmetic and its contract hold on the
target, with the elapsed interval coming from the real kernel tick. It is **not**:

- a performance measurement (the matrix's performance column stays pending);
- a plant simulation or HIL result — the controller is driven by a fixed setpoint,
  not by a plant;
- evidence about jitter or saturation recovery, which need a real plant and a
  disturbance;
- evidence about the auto-tuner (`xy_pid_auto`), which this image does not exercise.
