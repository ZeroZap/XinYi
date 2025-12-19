# Top-level Makefile for XY Framework

# Compiler and flags
CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c99 -O2
LDFLAGS ?=

# Windows system needs additional libraries
ifeq ($(OS),Windows_NT)
    LDFLAGS += -ladvapi32
endif

# Component directories
COMPONENTS = crypto xy_clib dm net device trace osal Bank sensor ipc time_tick xy_key xy_state_machine fota kernel misc pm xfer xy_code_style

# Default target
all: $(COMPONENTS)

# Build each component
$(COMPONENTS):
	@echo "Building component: $@"
	$(MAKE) -C components/$@ all

# Clean all components
clean:
	@for component in $(COMPONENTS); do \
		echo "Cleaning component: $$component"; \
		$(MAKE) -C components/$$component clean; \
	done

# Test all components
test:
	@for component in $(COMPONENTS); do \
		if [ -f components/$$component/test/Makefile ] || [ -f components/$$component/Makefile ]; then \
			echo "Testing component: $$component"; \
			$(MAKE) -C components/$$component test || true; \
		fi; \
	done

# Install all components
install:
	@for component in $(COMPONENTS); do \
		echo "Installing component: $$component"; \
		$(MAKE) -C components/$$component install || true; \
	done

# Help
help:
	@echo "Available targets:"
	@echo "  all       - Build all components"
	@echo "  clean     - Clean all components"
	@echo "  test      - Run tests for all components"
	@echo "  install   - Install all components"
	@echo "  help      - Show this help information"
	@echo ""
	@echo "Component-specific targets:"
	@for component in $(COMPONENTS); do \
		echo "  $$component - Build $$component component"; \
	done

.PHONY: all clean test install help $(COMPONENTS)