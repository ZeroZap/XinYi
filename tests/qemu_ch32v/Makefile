# Unified QEMU CH32V test build
#
# Usage:
#   make                     # build all tests
#   make smoke_test          # build one test
#   make run-smoke_test      # build and run one test in QEMU
#   make test                # build and run non-interactive tests
#   make clean

WCH_TOOLCHAIN_BIN ?= /usr/share/MRS2/MRS-linux-x64/resources/app/resources/linux/components/WCH/Toolchain/RISC-V Embedded GCC15/bin
CC      := $(WCH_TOOLCHAIN_BIN)/riscv32-wch-elf-gcc
SIZE    := $(WCH_TOOLCHAIN_BIN)/riscv32-wch-elf-size

QEMU_UNIFIED_RISCV32 ?= /home/eugene/Tools/qemu-unified-10.2.2/bin/qemu-system-riscv32
QEMU         ?= $(if $(wildcard $(QEMU_UNIFIED_RISCV32)),$(QEMU_UNIFIED_RISCV32),qemu-system-riscv32)
QEMU_MACHINE ?= ch32v307
QEMU_TIMEOUT ?= 10
BUILD_DIR    ?= ../../build/qemu/ch32v

CPU_FLAGS := -march=rv32imafc_zicsr -mabi=ilp32f
CFLAGS    := $(CPU_FLAGS) -O0 -g3 -Wall -ffreestanding -fno-builtin
LDFLAGS   := $(CPU_FLAGS) -nostdlib -T $(abspath ch32v307.ld)

TESTS := $(patsubst %/src/main.c,%,$(wildcard */src/main.c))
RUNNABLE_TESTS := smoke_test
ELFS := $(foreach t,$(TESTS),$(BUILD_DIR)/$(t).elf)

.PHONY: all clean test list $(TESTS) $(addprefix run-,$(TESTS))

all: $(ELFS)

list:
	@echo "Discovered tests:"
	@for t in $(TESTS); do echo "  $$t"; done

define TEST_template
$(1): $(BUILD_DIR)/$(1).elf
$(BUILD_DIR)/$(1).elf: $(1)/src/main.c $(1)/src/startup.S ch32v307.ld
	@mkdir -p $$(dir $$@)
	@echo "  CC    $$@"
	@"$(CC)" $(CFLAGS) $(LDFLAGS) -o $$@ $(1)/src/main.c $(1)/src/startup.S
	@"$(SIZE)" $$@ | tail -1
run-$(1): $(BUILD_DIR)/$(1).elf
	@echo "  RUN   $$<"
	@timeout $(QEMU_TIMEOUT) "$(QEMU)" -M $(QEMU_MACHINE) -display none \
	    -serial stdio -kernel $$< 2>&1 | tee $(BUILD_DIR)/$(1).log || true
endef
$(foreach t,$(TESTS),$(eval $(call TEST_template,$(t))))

test: $(foreach t,$(RUNNABLE_TESTS),$(BUILD_DIR)/$(t).elf)
	@total_pass=0; total_fail=0; failed=""; \
	for t in $(RUNNABLE_TESTS); do \
	    echo "=== $$t ==="; \
	    timeout $(QEMU_TIMEOUT) "$(QEMU)" -M $(QEMU_MACHINE) -display none \
	        -serial stdio -kernel $(BUILD_DIR)/$$t.elf > $(BUILD_DIR)/$$t.log 2>&1 || true; \
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
