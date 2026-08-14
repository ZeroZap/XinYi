# XinYi Crypto compatibility Makefile
#
# The canonical component build is the repository CMake target `xy_tiny_crypto`.
# Historical versions of this file compiled every src/*.c file directly, but most
# active algorithms now live in module directories (xy_crc/, xy_base/, xy_hmac/,
# etc.) and some remaining src/*.c files are compatibility wrappers or excluded
# stale API copies.  Keep this wrapper thin so ad-hoc `make -C components/crypto`
# users exercise the same source ownership as root builds.

REPO_ROOT := $(abspath ../..)
BUILD_DIR ?= $(REPO_ROOT)/build/pc
BUILD_TYPE ?= Release
HAL_PLATFORM ?= PC
CMAKE ?= cmake

.PHONY: all library test run_test clean help

all: library

library:
	$(CMAKE) -B "$(BUILD_DIR)" -S "$(REPO_ROOT)" -DHAL_PLATFORM=$(HAL_PLATFORM) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)
	$(CMAKE) --build "$(BUILD_DIR)" --target xy_tiny_crypto -j$$(nproc)

# Component-local legacy tests are no longer authoritative.  Run the repository
# Unity/CTest gate so crypto targets and policy smokes stay in sync.
test run_test:
	$(MAKE) -C "$(REPO_ROOT)" test-unit

clean:
	$(CMAKE) --build "$(BUILD_DIR)" --target clean || true

help:
	@echo "XinYi Crypto targets:"
	@echo "  all/library - configure/build canonical xy_tiny_crypto via root CMake"
	@echo "  test        - run repository make test-unit"
	@echo "  run_test    - alias for test"
	@echo "  clean       - clean the configured CMake build directory"
	@echo "Variables: BUILD_DIR=$(BUILD_DIR) BUILD_TYPE=$(BUILD_TYPE) HAL_PLATFORM=$(HAL_PLATFORM)"
