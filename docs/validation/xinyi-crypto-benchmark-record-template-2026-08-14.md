# XinYi Crypto Benchmark Record Template

**Date:** 2026-08-14  
**Status:** Template / no benchmark result recorded  
**Scope:** reproducible benchmark evidence for `components/crypto` optimization or backend-comparison work  
**Related design:** `docs/design/xinyi-crypto-benchmark-harness-proposal-2026-08-14.md`

## Purpose

This template records benchmark evidence without upgrading security, provenance, side-channel, production, or hardware-validation status. It must be filled with real command output and run metadata; do not paste host CTest success, estimated timing, or compile-only output into measurement fields.

Default `make test-unit` may validate this template and the benchmark manifest, but it must not execute timing loops or fail because a machine is slow.

## Result classification

Choose exactly one:

- `pending`: no benchmark run has been performed.
- `host-smoke-only`: manifest/policy/correctness smoke passed; no timing has been recorded.
- `host-timing-recorded`: opt-in PC timing was recorded with compiler, flags, iterations, warm-up count, commit hash, dirty state, and correctness-gate evidence.
- `target-compile-only`: benchmark harness compiled for the named target, but no target timing was recorded.
- `mcu-cycle-recorded`: real board cycle/us/throughput samples were recorded with clock, interrupt/cache state, toolchain flags, and raw log artifacts.
- `invalid`: the run used forbidden inputs, missing metadata, unstable correctness output, or unsupported evidence claims.

Current result: `pending`

## Run identity

| Field | Value |
| --- | --- |
| Operator | pending |
| Git commit under test | pending |
| Dirty state | pending |
| HAL_PLATFORM / board | pending |
| Compiler and version | pending |
| Compile flags | pending |
| Benchmark harness commit/path | pending |
| Correctness gate command/output | pending |

## Target compile-only section

Use only for an explicit target compile probe such as:

```bash
python3 tests/unit/crypto/crypto_benchmark_stm32u5_compile_probe.py \
  --run-compile --i-understand-target-compile-only
```

This may support `target-compile-only` classification when the command exits successfully. It still records no benchmark timing and does not prove hardware validation, MCU cycle measurements, security approval, provenance approval, side-channel safety, production readiness, or algorithm correctness beyond the separately listed focused CTest gates.

The default JSON plan path may be used to inspect intended commands and evidence boundaries without invoking the ARM toolchain:

```bash
python3 tests/unit/crypto/crypto_benchmark_stm32u5_compile_probe.py --plan-only --json
```

That JSON output remains plan-only/no-build metadata and cannot be pasted into result fields as target compile, timing, hardware, security, or provenance evidence.

| Field | Value |
| --- | --- |
| Target HAL_PLATFORM | pending |
| Build target | pending |
| Configure command/output | pending |
| Build command/output | pending |
| Compile-only evidence boundary | pending |

## Algorithm group under test

| Field | Value |
| --- | --- |
| Algorithm id | pending |
| Source ownership | pending |
| C fallback path | pending |
| Optimized/backend path, if any | pending / none |
| Contract CTest(s) | pending |
| Input sizes | pending |
| Iterations / warm-up iterations | pending |
| Test key/nonce/seed policy | public fixed test material only; no production/customer/identity secrets |

## Host timing section

Use only for an explicit opt-in host benchmark. Do not interpret this as MCU performance.
The default checked-in CTest smoke remains bounded to 1 iteration and must not contain throughput, latency, or pass/fail performance thresholds. Any real host timing record must state the requested iteration count, stay within the manifest bounds, and keep raw timing output as PC-only evidence. The checked-in refusal guards cover both zero iterations and requests above the current manifest maximum (`1000`) before any timing record can be accepted. Manifest input sizes must also remain non-negative and no larger than the current `4096`-byte host-smoke bound; out-of-range sizes are invalid rather than silently omitted from a record.

| Field | Value |
| --- | --- |
| Host CPU / OS | pending |
| Timer source | pending |
| Iteration count | pending |
| Warm-up count | pending |
| Samples | pending |
| Median / min / max | pending |
| Correctness digest/tag/ciphertext check | pending |
| Raw output artifact | pending |

## MCU cycle section

Use only after a real board run. Compile-only output belongs in `target-compile-only`, not here.

| Field | Value |
| --- | --- |
| Board / revision | pending |
| MCU / clock tree | pending |
| Cache / interrupt state | pending |
| Cycle counter or timer source | pending |
| UART/SWO/log artifact | pending |
| Samples | pending |
| bytes/s or cycles/byte | pending |
| Known measurement limitations | pending |

## Forbidden evidence substitutions

The following evidence may be attached for context but cannot upgrade this record by itself:

- focused `crypto_*` CTest pass output without benchmark metadata;
- `make test-unit` pass output without opt-in timing;
- PC host timing used as a target/MCU performance claim;
- STM32U5 or other target compile-only output used as timing or hardware validation;
- fake entropy, fake I/O, or deterministic test vectors used as production security evidence;
- any real production key, secure-boot signing key, customer secret, certificate private key, or product identity material.

## Evidence boundary statement

This benchmark record can only support the exact result classification selected above. It does not prove security approval, provenance approval, constant-time behavior, side-channel safety, production readiness, or hardware validation unless separate review and hardware-validation records explicitly provide that evidence.

## Verification commands for record/template changes

When changing this template, manifest, or checker, record real output from:

```bash
cd build/tests/unit && ctest --output-on-failure -R '^crypto_benchmark_manifest$'
make test-unit
git diff --check
```

## Rollback

Before commit:

```bash
git checkout -- docs/validation/xinyi-crypto-benchmark-record-template-2026-08-14.md
```

After commit:

```bash
git revert <commit>
```
