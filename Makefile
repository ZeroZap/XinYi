# Top-level Makefile for XY Framework
# Version: 2.0
# Date: 2026-02-28

# ==================== Configuration ====================

CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c99 -O2
LDFLAGS ?=

# Windows system needs additional libraries
ifeq ($(OS),Windows_NT)
    LDFLAGS += -ladvapi32
endif

# Build options
BUILD_TYPE ?= release
BUILD_TESTS ?= 0
VERBOSE ?= 0

ifeq ($(BUILD_TYPE),debug)
    CFLAGS += -g -O0 -DDEBUG
else ifeq ($(BUILD_TYPE),release)
    CFLAGS += -O2 -DNDEBUG
endif

ifeq ($(BUILD_TESTS),1)
    CFLAGS += -DBUILD_TESTING=1
endif

ifeq ($(VERBOSE),1)
    Q :=
    CFLAGS += -v
else
    Q := @
endif

# ==================== Component Detection ====================

# Auto-detect component directories
COMPONENTS := $(notdir $(wildcard components/*))

# Filter to only include components with CMakeLists.txt or Makefile
VALID_COMPONENTS := $(foreach comp,$(COMPONENTS),\
    $(if $(or \
        $(wildcard components/$(comp)/CMakeLists.txt),\
        $(wildcard components/$(comp)/Makefile)),\
        $(comp)))

# ==================== Targets ====================

.PHONY: all clean test install help configure $(VALID_COMPONENTS)

# Default target
all: $(VALID_COMPONENTS)
	@echo ""
	@echo "Build complete!"
	@echo "  Build Type: $(BUILD_TYPE)"
	@echo "  Components: $(words $(VALID_COMPONENTS))"
	@echo ""

# Build each component
$(VALID_COMPONENTS):
	$(Q)if [ -f components/$@/Makefile ]; then \
		echo "Building component: $@ (Makefile)"; \
		$(MAKE) -C components/$@ all; \
	elif [ -f components/$@/CMakeLists.txt ]; then \
		echo "Building component: $@ (CMake)"; \
		mkdir -p components/$@/build; \
		cd components/$@/build && cmake .. -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) && $(MAKE); \
	else \
		echo "Skipping $@: No build system found"; \
	fi

# Clean all components
clean:
	@echo "Cleaning all components..."
	$(Q)for component in $(VALID_COMPONENTS); do \
		echo "Cleaning component: $$component"; \
		if [ -f components/$$component/Makefile ]; then \
			$(MAKE) -C components/$$component clean; \
		elif [ -d components/$$component/build ]; then \
			rm -rf components/$$component/build; \
		fi; \
	done
	@echo "Clean complete!"

# Test all components
test:
	@echo "Running tests..."
	$(Q)for component in $(VALID_COMPONENTS); do \
		if [ -d components/$$component/tests ] || [ -d components/$$component/test ]; then \
			echo "Testing component: $$component"; \
			if [ -f components/$$component/tests/CMakeLists.txt ]; then \
				cd components/$$component/tests/build && ctest --output-on-failure || true; \
			elif [ -f components/$$component/Makefile ]; then \
				$(MAKE) -C components/$$component test || true; \
			fi; \
		fi; \
	done
	@echo "Tests complete!"

# Configure (generate build files without building)
configure:
	@echo "Configuring components..."
	$(Q)for component in $(VALID_COMPONENTS); do \
		if [ -f components/$$component/CMakeLists.txt ]; then \
			echo "Configuring $$component..."; \
			mkdir -p components/$$component/build; \
			cd components/$$component/build && cmake .. -DCMAKE_BUILD_TYPE=$(BUILD_TYPE); \
		fi; \
	done
	@echo "Configuration complete!"

# Install all components
install:
	@echo "Installing components..."
	$(Q)for component in $(VALID_COMPONENTS); do \
		echo "Installing component: $$component"; \
		if [ -f components/$$component/Makefile ]; then \
			$(MAKE) -C components/$$component install || true; \
		elif [ -d components/$$component/build ]; then \
			$(MAKE) -C components/$$component/build install || true; \
		fi; \
	done
	@echo "Installation complete!"

# Help
help:
	@echo "XinYi Framework Makefile"
	@echo ""
	@echo "Usage:"
	@echo "  make [target] [options]"
	@echo ""
	@echo "Targets:"
	@echo "  all         - Build all components (default)"
	@echo "  clean       - Clean all build artifacts"
	@echo "  test        - Run all component tests"
	@echo "  configure   - Generate build files without building"
	@echo "  install     - Install components to system"
	@echo "  help        - Show this help message"
	@echo ""
	@echo "Options:"
	@echo "  BUILD_TYPE=release|debug  - Build type (default: release)"
	@echo "  BUILD_TESTS=0|1           - Build tests (default: 0)"
	@echo "  VERBOSE=0|1               - Verbose output (default: 0)"
	@echo ""
	@echo "Examples:"
	@echo "  make                              # Build all components"
	@echo "  make BUILD_TYPE=debug             # Debug build"
	@echo "  make BUILD_TESTS=1                # Build with tests"
	@echo "  make crypto                       # Build specific component"
	@echo "  make clean BUILD_TYPE=release     # Clean release build"
	@echo ""
	@echo "Available Components:"
	@$(foreach comp,$(VALID_COMPONENTS),echo "  - $(comp)";)
