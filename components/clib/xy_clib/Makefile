# XY Component Makefile for xy_clib

# Compiler and flags
CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c99 -O2 -Iinc -I.
LDFLAGS ?=

# Windows system needs additional libraries
ifeq ($(OS),Windows_NT)
    LDFLAGS += -ladvapi32
endif

# Source and object directories
SRCDIR = .
OBJDIR = build
INCDIR = inc

# Find all source files (examples/tests are not part of the library)
SOURCES = $(filter-out %_example.c test_%.c,$(wildcard *.c))
SUBDIRS = $(filter-out $(OBJDIR) $(INCDIR), $(wildcard */))
SUBDIR_SOURCES = $(foreach dir,$(SUBDIRS),$(filter-out %_example.c test_%.c,$(wildcard $(dir)*.c)))

# Combine all sources
ALL_SOURCES = $(SOURCES) $(SUBDIR_SOURCES)
OBJECTS = $(ALL_SOURCES:%.c=$(OBJDIR)/%.o)

# Library name
LIBRARY = libxy_xy_clib.a

.PHONY: all clean library help install

all: library

# Create build directory
$(OBJDIR):
	mkdir -p $(OBJDIR)
	@for dir in $(SUBDIRS); do \
		mkdir -p $(OBJDIR)/$$dir; \
	done

# Compile source files
$(OBJDIR)/%.o: %.c | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Create static library
library: $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	ar rcs $@ $^
	@echo "Library $(LIBRARY) created successfully"

# Tests are provided by the repository-level tests/unit suite.
# Run from the repository root:
#   make test-unit
# Or focus this component with:
#   cmake --build build/tests/unit --target test_clib -j"$(nproc)"
#   ctest --test-dir build/tests/unit -R '^clib_component$' --output-on-failure

# Clean
clean:
	rm -rf $(OBJDIR) $(LIBRARY)

# Install headers and library (optional)
install: library
	@echo "Installing library to system directories"
	# cp $(LIBRARY) /usr/local/lib/
	# cp $(INCDIR)/*.h /usr/local/include/

# Show help
help:
	@echo "Available targets:"
	@echo "  all       - Compile library"
	@echo "  library   - Compile library file"
	@echo "  clean     - Clean generated files"
	@echo "  install   - Install library and headers"
	@echo "  help      - Show this help information"