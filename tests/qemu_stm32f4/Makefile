# Unified QEMU STM32F4 test build
#
# Each test is a directory with src/main.c + src/startup.c.
# A shared linker script (stm32f405rg.ld) is used for all tests.
#
# Usage:
#   make                     # build all tests
#   make <name>              # build one test (e.g. make hal_test)
#   make run-<name>          # build and run one test in QEMU
#   make test                # build and run the non-interactive tests
#   make clean               # remove all built ELFs

TOOLCHAIN_PATH ?= /home/eugene/Tools/arm-gnu-toolchain/bin
CC      := $(TOOLCHAIN_PATH)/arm-none-eabi-gcc
SIZE    := $(TOOLCHAIN_PATH)/arm-none-eabi-size

QEMU_UNIFIED_ARM ?= /home/eugene/Tools/qemu-unified-10.2.2/bin/qemu-system-arm
QEMU         ?= $(if $(wildcard $(QEMU_UNIFIED_ARM)),$(QEMU_UNIFIED_ARM),qemu-system-arm)
QEMU_MACHINE ?= olimex-stm32-h405
QEMU_TIMEOUT ?= 10
BUILD_DIR    ?= ../../build/qemu/stm32f4

CPU_FLAGS := -mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16
CFLAGS    := $(CPU_FLAGS) -O0 -g3 -Wall -ffreestanding
LDFLAGS   := $(CPU_FLAGS) -nostdlib -T $(abspath stm32f405rg.ld)

# Discover tests: any directory holding src/main.c + src/startup.c.
TESTS := $(patsubst %/src/main.c,%,$(wildcard */src/main.c))

# Tests considered runnable as non-interactive PASS/FAIL checks.
RUNNABLE_TESTS := hal_test alg_test components_test

# Build artifact for each test: ../../build/qemu/stm32f4/<dir>.elf
ELFS := $(foreach t,$(TESTS),$(BUILD_DIR)/$(t).elf)

.PHONY: all clean test list $(TESTS) $(addprefix run-,$(TESTS))

all: $(ELFS)

list:
	@echo "Discovered tests:"
	@for t in $(TESTS); do echo "  $$t"; done

# Per-test convenience target: `make hal_test` builds into $(BUILD_DIR)
define TEST_template
$(1): $(BUILD_DIR)/$(1).elf
$(BUILD_DIR)/$(1).elf: $(1)/src/main.c $(1)/src/startup.c stm32f405rg.ld
	@mkdir -p $$(dir $$@)
	@echo "  CC    $$@"
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $$@ $(1)/src/main.c $(1)/src/startup.c
	@$(SIZE) $$@ | tail -1
run-$(1): $(BUILD_DIR)/$(1).elf
	@echo "  RUN   $$<"
	@timeout $(QEMU_TIMEOUT) $(QEMU) -M $(QEMU_MACHINE) -nographic \
	    -kernel $$< -semihosting 2>&1 | tee $(BUILD_DIR)/$(1).log || true
endef
$(foreach t,$(TESTS),$(eval $(call TEST_template,$(t))))

# Run all non-interactive tests and report PASS/FAIL aggregate
test: $(foreach t,$(RUNNABLE_TESTS),$(BUILD_DIR)/$(t).elf)
	@total_pass=0; total_fail=0; failed=""; \
	for t in $(RUNNABLE_TESTS); do \
	    echo "=== $$t ==="; \
	    timeout $(QEMU_TIMEOUT) $(QEMU) -M $(QEMU_MACHINE) -nographic \
	        -kernel $(BUILD_DIR)/$$t.elf -semihosting > $(BUILD_DIR)/$$t.log 2>&1 || true; \
	    line=$$(grep '\[RESULT\] PASS=' $(BUILD_DIR)/$$t.log || true); \
	    if [ -z "$$line" ]; then \
	        echo "  no [RESULT] line"; failed="$$failed $$t"; continue; \
	    fi; \
	    pass=$$(echo "$$line" | sed 's/.*PASS=\([0-9]*\).*/\1/'); \
	    fail=$$(echo "$$line" | sed 's/.*FAIL=\([0-9]*\).*/\1/'); \
	    echo "  PASS=$$pass FAIL=$$fail"; \
	    total_pass=$$((total_pass + pass)); \
	    total_fail=$$((total_fail + fail)); \
	    [ "$$fail" -ne 0 ] && failed="$$failed $$t"; \
	done; \
	echo ""; \
	echo "Total: PASS=$$total_pass FAIL=$$total_fail"; \
	[ -z "$$failed" ] || { echo "Failed:$$failed"; exit 1; }

clean:
	@rm -rf $(BUILD_DIR)
