# XinYi GUI Font Rendering Hardware Validation Record Template

**Date:** 2026-08-11  
**Status:** Template / no real display result recorded  
**Scope:** real-screen validation record for GUI font rendering and display-backend binding  
**Related design:** `docs/design/xinyi-gui-font-framebuffer-snapshot-proposal-2026-08-11.md`

## Purpose

This document is the required evidence format before GUI font rendering can be described as hardware-validated. It intentionally separates host framebuffer snapshots and fake display-backend CTests from real LCD/OLED/LED-matrix evidence so the project does not treat deterministic software checksums as visual or electrical validation.

Do not fill success fields with assumed values. If screen hardware is unavailable, leave the result as `pending` and record the blocker.

## Run identity

| Field | Value |
| --- | --- |
| Operator | pending |
| Git commit under test | pending |
| Board / revision | pending |
| MCU / build target | pending |
| Display device / panel | pending |
| Display interface | pending |
| GUI sample application / smoke | pending |
| Validation location | pending |

## Firmware/software under test

| Item | Required value |
| --- | --- |
| GUI font assets | current manifest-declared `ascii_8x16`, `ascii_16x24`, `chinese_16x16_ui_legacy` unless explicitly documented otherwise |
| Font engine guard status | `gui_fonts`, `gui_font_manifest`, `gui_font_engine`, `gui_font_snapshot`, `gui_font_generator_*`, `make test-unit` |
| GUI/display bridge guard status | `gui_display_backend` and the concrete adapter CTest, for example `gui_ssd1306_adapter` |
| Target compile status | STM32U5 or board-specific build/probe result and log pointer |
| Generated glyph tables | pending / not used / used with manifest and license/provenance record |

## Board wiring and display configuration

| Field | Value |
| --- | --- |
| Display driver path | pending |
| Bus instance | pending |
| Data pins / mux | pending |
| Control pins (CS/DC/RST/BL/etc.) | pending |
| Bus speed / mode | pending |
| Display resolution | pending |
| Color format / mono mapping | pending |
| Backlight / contrast setting | pending |

## Required visual transcript

Record exact firmware logs and attach screenshots/photos or framebuffer dumps for each step. Do not record only a final boolean.

### 1. Display initialization and clear

| Field | Value |
| --- | --- |
| Setup | pending |
| API sequence | GUI init -> display adapter bind -> clear -> flush |
| Expected | screen clears to known background with no bus/display error |
| Actual | pending |
| Raw log / trace | pending |
| Photo or capture artifact | pending |

### 2. ASCII font sample

| Field | Value |
| --- | --- |
| Sample text | pending, recommended: ` !~OK` plus numeric/status text |
| Font asset | pending |
| Expected | glyphs visible, aligned, clipped only at intended bounds |
| Actual | pending |
| Host snapshot checksum reference | pending |
| Photo or capture artifact | pending |

### 3. Chinese UI glyph sample

| Field | Value |
| --- | --- |
| Sample text | pending, restricted to manifest-required UI codepoints unless documented |
| Font asset | `chinese_16x16_ui_legacy` or documented generated replacement |
| Expected | required UI glyphs render in stable positions; placeholder/duplicate legacy limitations are explicitly noted |
| Actual | pending |
| Photo or capture artifact | pending |

### 4. Clipping and unknown-glyph behavior

| Field | Value |
| --- | --- |
| Setup | pending |
| API sequence | render near display edge and render unsupported glyph |
| Expected | no memory corruption, no wraparound artifacts, fallback/unsupported behavior matches host contract |
| Actual | pending |
| Raw log / trace | pending |
| Photo or capture artifact | pending |

### 5. Long-run refresh smoke

| Field | Value |
| --- | --- |
| Duration / refresh count | pending |
| Sample loop | pending |
| Expected | no visible tearing beyond known interface limits, no crash, no accumulating bus errors |
| Actual | pending |
| Error count / retry count | pending |

## Result classification

Choose exactly one:

- `pending`: no real display run has been performed.
- `compile-only`: target build/probe passed, but no real display evidence exists.
- `host-snapshot-only`: host framebuffer snapshot passed, but no screen hardware evidence exists.
- `hardware-failed`: real display run failed; blocker and raw logs/artifacts are captured.
- `hardware-passed-basic`: clear/flush, ASCII sample, Chinese UI sample, and clipping/unknown-glyph behavior passed on a real display.
- `hardware-passed-stress`: basic validation plus long-run refresh smoke passed.

Current result: `pending`

## Blockers / notes

- pending

## Enablement decision

GUI font rendering remains host-guarded but not hardware-qualified until this record reaches at least `hardware-passed-basic`. `gui_font_snapshot` checksums and fake display-backend CTests must not be used as substitutes for real screen photos/logs tied to a board/display configuration.
