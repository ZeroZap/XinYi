# Makefile for EFLASH Component
# Author: XinYi Components Team
# Date: 2025-10-22

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -g
LDFLAGS =

# Source files
SOURCES = eflash.c eflash_test.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = eflash_test

# Header files
HEADERS = eflash.h

# Build rules
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "Build complete: $(TARGET)"

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Run tests
test: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Clean complete"

# Rebuild everything
rebuild: clean all

# Help
help:
	@echo "EFLASH Component Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  all      - Build the test executable (default)"
	@echo "  test     - Build and run tests"
	@echo "  clean    - Remove build artifacts"
	@echo "  rebuild  - Clean and rebuild"
	@echo "  help     - Show this help message"

.PHONY: all test clean rebuild help
