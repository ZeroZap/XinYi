# XinYi Framework — Makefile wrapper around CMake
#
# Usage:
#   make                              PC Release build
#   make BUILD_TYPE=debug             PC Debug build
#   make HAL_PLATFORM=STM32U5         STM32U5 build
#   make HAL_PLATFORM=STM32F4 BUILD_TESTS=ON
#   make HAL_PLATFORM=STM32L4
#   make test                         run PC unit tests + QEMU tests
#   make test-unit                    run PC unit tests only
#   make test-qemu                    run QEMU STM32F4 tests only
#   make test-qemu-ch32v              run QEMU CH32V tests only
#   make clean
#   make distclean                    remove all build directories
#   make help

# ---------- Defaults ----------
HAL_PLATFORM ?= PC
BUILD_TYPE    ?= Release
BUILD_TESTS   ?= OFF
FOTA          ?= OFF
JOBS          ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Normalise build type
ifeq ($(BUILD_TYPE),debug)
    _BUILD_TYPE := Debug
else ifeq ($(BUILD_TYPE),release)
    _BUILD_TYPE := Release
else
    _BUILD_TYPE := $(BUILD_TYPE)
endif

# Build directories are grouped under build/ to keep the repository root clean.
BUILD_ROOT ?= build
_PLATFORM := $(shell echo $(HAL_PLATFORM) | tr '[:upper:]' '[:lower:]')
BUILD_DIR ?= $(BUILD_ROOT)/$(_PLATFORM)
ROOT_BUILD_CLEAN := $(sort $(BUILD_ROOT) build build_*)

CMAKE_FLAGS := \
    -DHAL_PLATFORM=$(HAL_PLATFORM) \
    -DCMAKE_BUILD_TYPE=$(_BUILD_TYPE) \
    -DBUILD_TESTING=$(BUILD_TESTS) \
    -DXY_FOTA_ENABLED=$(FOTA)

# Test directories
UNIT_DIR := tests/unit
UNIT_BUILD_DIR := $(BUILD_ROOT)/tests/unit
QEMU_DIR := tests/qemu_stm32f4
QEMU_CH32V_DIR := tests/qemu_ch32v

# ---------- Targets ----------
.PHONY: all configure clean distclean help test test-unit test-qemu test-qemu-ch32v

all: $(BUILD_DIR)/CMakeCache.txt
	@cmake --build $(BUILD_DIR) -j$(JOBS)

$(BUILD_DIR)/CMakeCache.txt:
	@cmake -B $(BUILD_DIR) -S . $(CMAKE_FLAGS)

configure:
	cmake -B $(BUILD_DIR) -S . $(CMAKE_FLAGS)

# Run everything: PC unit tests + QEMU tests.
# Each phase prints its own results; reaching the end means everything passed.
test: test-unit test-qemu
	@echo ""
	@echo "=========================================="
	@echo "  ALL TEST SUITES PASSED"
	@echo "=========================================="

test-unit:
	@echo "=========================================="
	@echo "  PC unit tests ($(UNIT_DIR))"
	@echo "=========================================="
	@cmake -B $(UNIT_BUILD_DIR) -S $(UNIT_DIR) >/dev/null
	@cmake --build $(UNIT_BUILD_DIR) -j$(JOBS) >/dev/null
	@cd $(UNIT_BUILD_DIR) && ctest --output-on-failure

test-qemu:
	@echo ""
	@echo "=========================================="
	@echo "  QEMU STM32F4 tests ($(QEMU_DIR))"
	@echo "=========================================="
	@$(MAKE) -C $(QEMU_DIR) test

test-qemu-ch32v:
	@echo ""
	@echo "=========================================="
	@echo "  QEMU CH32V tests ($(QEMU_CH32V_DIR))"
	@echo "=========================================="
	@$(MAKE) -C $(QEMU_CH32V_DIR) test

clean:
	@cmake --build $(BUILD_DIR) --target clean 2>/dev/null \
	    || echo "(nothing to clean in $(BUILD_DIR))"
	@$(MAKE) -C $(QEMU_DIR) clean 2>/dev/null || true
	@$(MAKE) -C $(QEMU_CH32V_DIR) clean 2>/dev/null || true
	@rm -rf $(UNIT_BUILD_DIR)

distclean:
	rm -rf tmp $(ROOT_BUILD_CLEAN) \
	       $(UNIT_DIR)/build $(wildcard examples/*/build examples/*/*/build tests/*/build \
	       components/*/build components/*/*/build)
	@$(MAKE) -C $(QEMU_DIR) clean 2>/dev/null || true
	@$(MAKE) -C $(QEMU_CH32V_DIR) clean 2>/dev/null || true

help:
	@echo ""
	@echo "XinYi Framework Build"
	@echo ""
	@echo "  make [options]"
	@echo ""
	@echo "Options:"
	@echo "  HAL_PLATFORM=PC|STM32F4|STM32U5|STM32L4|WCH|HC32   (default: PC)"
	@echo "  BUILD_TYPE=Release|Debug                    (default: Release)"
	@echo "  BUILD_TESTS=ON|OFF                          (default: OFF)"
	@echo "  FOTA=ON|OFF                                 (default: OFF)"
	@echo "  BUILD_ROOT=DIR                              (default: build)"
	@echo "  BUILD_DIR=DIR                               (default: build/<platform>)"
	@echo "  JOBS=N                                      (default: nproc)"
	@echo ""
	@echo "Targets:"
	@echo "  all         build (default)"
	@echo "  configure   run cmake without building"
	@echo "  test        run all PC unit tests + QEMU tests"
	@echo "  test-unit   run PC unit tests only"
	@echo "  test-qemu   run QEMU STM32F4 tests only"
	@echo "  test-qemu-ch32v run QEMU CH32V tests only"
	@echo "  clean       clean build artifacts"
	@echo "  distclean   remove all build directories"
	@echo "  help        show this message"
	@echo ""
	@echo "Examples:"
	@echo "  make"
	@echo "  make test"
	@echo "  make BUILD_TYPE=debug"
	@echo "  make HAL_PLATFORM=STM32U5"
	@echo ""
