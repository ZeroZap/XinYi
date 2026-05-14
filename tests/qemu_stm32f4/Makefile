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

QEMU         ?= qemu-system-arm
QEMU_MACHINE ?= olimex-stm32-h405
QEMU_TIMEOUT ?= 10

CPU_FLAGS := -mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16
CFLAGS    := $(CPU_FLAGS) -O0 -g3 -Wall -ffreestanding
LDFLAGS   := $(CPU_FLAGS) -nostdlib -T $(abspath stm32f405rg.ld)

# Discover tests: any directory holding src/main.c + src/startup.c.
TESTS := $(patsubst %/src/main.c,%,$(wildcard */src/main.c))

# Tests considered runnable as non-interactive PASS/FAIL checks.
RUNNABLE_TESTS := hal_test alg_test components_test

# Build artifact for each test: <dir>/<dir>.elf
ELFS := $(foreach t,$(TESTS),$(t)/$(t).elf)

.PHONY: all clean test list $(TESTS) $(addprefix run-,$(TESTS))

all: $(ELFS)

list:
	@echo "Discovered tests:"
	@for t in $(TESTS); do echo "  $$t"; done

# Per-test convenience target: `make hal_test` builds hal_test/hal_test.elf
define TEST_template
$(1): $(1)/$(1).elf
$(1)/$(1).elf: $(1)/src/main.c $(1)/src/startup.c stm32f405rg.ld
	@echo "  CC    $$@"
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $$@ $(1)/src/main.c $(1)/src/startup.c
	@$(SIZE) $$@ | tail -1
run-$(1): $(1)/$(1).elf
	@echo "  RUN   $$<"
	@timeout $(QEMU_TIMEOUT) $(QEMU) -M $(QEMU_MACHINE) -nographic \
	    -kernel $$< -semihosting 2>&1 | tee $(1)/test_output.log || true
endef
$(foreach t,$(TESTS),$(eval $(call TEST_template,$(t))))

# Run all non-interactive tests and report PASS/FAIL aggregate
test: $(foreach t,$(RUNNABLE_TESTS),$(t)/$(t).elf)
	@total_pass=0; total_fail=0; failed=""; \
	for t in $(RUNNABLE_TESTS); do \
	    echo "=== $$t ==="; \
	    timeout $(QEMU_TIMEOUT) $(QEMU) -M $(QEMU_MACHINE) -nographic \
	        -kernel $$t/$$t.elf -semihosting > $$t/test_output.log 2>&1 || true; \
	    line=$$(grep '\[RESULT\] PASS=' $$t/test_output.log || true); \
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
	@rm -f $(ELFS) $(foreach t,$(TESTS),$(t)/test_output.log)
