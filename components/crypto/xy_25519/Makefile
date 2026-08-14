# Makefile for XY 25519 Library (X25519 + Ed25519)

# Toolchain selection based on target CPU
ifeq ($(TARGET_CPU),cortex-m0)
    # Cortex-M0: ARMv6-M, no UMULL, use assembly optimizations
    CC = arm-none-eabi-gcc
    AS = arm-none-eabi-as
    AR = arm-none-eabi-ar
    CFLAGS = -Wall -Wextra -std=c99 -O3 \
             -mcpu=cortex-m0 -mthumb \
             -ffunction-sections -fdata-sections \
             -fshort-enums -ffreestanding \
             -I. -I.. -I../inc -I../../xy_clib -I../../trace/xy_log/inc
    ASFLAGS = -mcpu=cortex-m0 -mthumb

    SOURCES = fe25519_m0.c xy_25519_m0.c
    # Focused-test-only/upstream-material assembly path.  Host CTests and this
    # ad-hoc Makefile do not prove production key exchange, signature security,
    # target timing, or side-channel behavior.
    ASM_SOURCES = asm/cortex_m0_mul256_reference.s \
                  asm/cortex_m0_reduce_reference.s \
                  asm/cortex_m0_mpy121666_reference.s \
                  asm/cortex_m0_square_reference.s
    CFLAGS += -DXY_25519_USE_M0_ASSEMBLY=1
    BUILD_TYPE = "Cortex-M0 optimized"

else ifeq ($(TARGET_CPU),cortex-m0plus)
    # Cortex-M0+: Same ARMv6-M ISA as M0
    CC = arm-none-eabi-gcc
    AS = arm-none-eabi-as
    AR = arm-none-eabi-ar
    CFLAGS = -Wall -Wextra -std=c99 -O3 \
             -mcpu=cortex-m0plus -mthumb \
             -ffunction-sections -fdata-sections \
             -fshort-enums -ffreestanding \
             -I. -I.. -I../inc -I../../xy_clib -I../../trace/xy_log/inc
    ASFLAGS = -mcpu=cortex-m0plus -mthumb

    SOURCES = fe25519_m0.c xy_25519_m0.c
    # Focused-test-only/upstream-material assembly path.  Host CTests and this
    # ad-hoc Makefile do not prove production key exchange, signature security,
    # target timing, or side-channel behavior.
    ASM_SOURCES = asm/cortex_m0_mul256_reference.s \
                  asm/cortex_m0_reduce_reference.s \
                  asm/cortex_m0_mpy121666_reference.s \
                  asm/cortex_m0_square_reference.s
    CFLAGS += -DXY_25519_USE_M0_ASSEMBLY=1
    BUILD_TYPE = "Cortex-M0+ optimized (same as M0)"

else ifeq ($(TARGET_CPU),cortex-m23)
    # Cortex-M23: ARMv8-M Baseline (compatible with M0 ISA)
    CC = arm-none-eabi-gcc
    AS = arm-none-eabi-as
    AR = arm-none-eabi-ar
    CFLAGS = -Wall -Wextra -std=c99 -O3 \
             -mcpu=cortex-m23 -mthumb \
             -ffunction-sections -fdata-sections \
             -fshort-enums -ffreestanding \
             -I. -I.. -I../inc -I../../xy_clib -I../../trace/xy_log/inc
    ASFLAGS = -mcpu=cortex-m23 -mthumb

    SOURCES = fe25519_m0.c xy_25519_m0.c
    # Focused-test-only/upstream-material assembly path.  Host CTests and this
    # ad-hoc Makefile do not prove production key exchange, signature security,
    # target timing, or side-channel behavior.
    ASM_SOURCES = asm/cortex_m0_mul256_reference.s \
                  asm/cortex_m0_reduce_reference.s \
                  asm/cortex_m0_mpy121666_reference.s \
                  asm/cortex_m0_square_reference.s
    CFLAGS += -DXY_25519_USE_M0_ASSEMBLY=1
    BUILD_TYPE = "Cortex-M23 (ARMv8-M Baseline)"

else ifeq ($(TARGET_CPU),cortex-m3)
    # Cortex-M3: Has UMULL, should use M3-specific optimizations
    CC = arm-none-eabi-gcc
    AS = arm-none-eabi-as
    AR = arm-none-eabi-ar
    CFLAGS = -Wall -Wextra -std=c99 -O2 \
             -mcpu=cortex-m3 -mthumb \
             -I. -I.. -I../inc -I../../xy_clib -I../../trace/xy_log/inc
    ASFLAGS = -mcpu=cortex-m3 -mthumb

    SOURCES = xy_25519.c
    ASM_SOURCES =
    BUILD_TYPE = "Cortex-M3 generic"
    $(warning "M3 has UMULL - consider M3-specific optimizations for 30% speedup")

else ifeq ($(TARGET_CPU),cortex-m33)
    # Cortex-M33: Has UMULL + DSP
    CC = arm-none-eabi-gcc
    AS = arm-none-eabi-as
    AR = arm-none-eabi-ar
    CFLAGS = -Wall -Wextra -std=c99 -O2 \
             -mcpu=cortex-m33 -mthumb \
             -I. -I.. -I../inc -I../../xy_clib -I../../trace/xy_log/inc
    ASFLAGS = -mcpu=cortex-m33 -mthumb

    SOURCES = xy_25519.c
    ASM_SOURCES =
    BUILD_TYPE = "Cortex-M33 generic"
    $(warning "M33 has UMULL+DSP - consider M33-specific optimizations for 45% speedup")

else ifeq ($(TARGET_CPU),cortex-m4)
    # Cortex-M4: Has UMULL + DSP + FPU
    CC = arm-none-eabi-gcc
    AS = arm-none-eabi-as
    AR = arm-none-eabi-ar
    CFLAGS = -Wall -Wextra -std=c99 -O2 \
             -mcpu=cortex-m4 -mthumb \
             -I. -I.. -I../inc -I../../xy_clib -I../../trace/xy_log/inc
    ASFLAGS = -mcpu=cortex-m4 -mthumb

    SOURCES = xy_25519.c
    ASM_SOURCES =
    BUILD_TYPE = "Cortex-M4 generic"
    $(warning "M4 has UMULL+DSP - consider M4-specific optimizations for 50% speedup")

else
    # Generic build (PC/unknown)
    CC = gcc
    AS = as
    AR = ar
    CFLAGS = -Wall -Wextra -std=c99 -O2 \
             -I. -I.. -I../inc -I../../xy_clib -I../../trace/xy_log/inc
    ASFLAGS =
    SOURCES = xy_25519.c
    ASM_SOURCES =
    BUILD_TYPE = "Generic"
endif

LDFLAGS =

# Object files
OBJECTS = $(SOURCES:.c=.o) $(ASM_SOURCES:.s=.o)

# Library name
LIBRARY = libxy_25519.a

.PHONY: all clean library help m0

all: library

# Cortex-M0 specific target
m0:
	@$(MAKE) TARGET_CPU=cortex-m0

# Cortex-M0+ target
m0plus:
	@$(MAKE) TARGET_CPU=cortex-m0plus

# Cortex-M23 target
m23:
	@$(MAKE) TARGET_CPU=cortex-m23

# Compile C source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble assembly source files
%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

# Create static library
library: $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	$(AR) rcs $@ $^
	@echo "Library $(LIBRARY) created successfully"
	@echo "Configuration: $(BUILD_TYPE)"

# Clean
clean:
	rm -f $(OBJECTS) $(LIBRARY)
	rm -f asm/*.o

# Help
help:
	@echo "Available targets:"
	@echo "  all       - Compile library (generic or optimized based on TARGET_CPU)"
	@echo "  m0        - Compile Cortex-M0 optimized library"
	@echo "  m0plus    - Compile Cortex-M0+ optimized library"
	@echo "  m23       - Compile Cortex-M23 optimized library"
	@echo "  library   - Compile library file only"
	@echo "  clean     - Clean generated files"
	@echo "  help      - Show this help information"
	@echo ""
	@echo "Platform-specific builds:"
	@echo "  Cortex-M0:   make TARGET_CPU=cortex-m0  (or: make m0)"
	@echo "  Cortex-M0+:  make TARGET_CPU=cortex-m0plus  (or: make m0plus)"
	@echo "  Cortex-M23:  make TARGET_CPU=cortex-m23  (or: make m23)"
	@echo "  Cortex-M3:   make TARGET_CPU=cortex-m3  (uses generic)"
	@echo "  Cortex-M4:   make TARGET_CPU=cortex-m4  (uses generic)"
	@echo "  Generic:     make  (no TARGET_CPU)"
	@echo ""
	@echo "Evidence boundary: focused-test-only / upstream-material"
	@echo "  This ad-hoc Makefile is not the root xy_tiny_crypto ownership path."
	@echo "  It does not prove production security, provenance, target timing, or side-channel behavior."
