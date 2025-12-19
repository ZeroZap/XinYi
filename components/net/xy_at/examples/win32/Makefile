# 编译器设置
CC = gcc
CFLAGS = -Wall -g -I.
LDFLAGS = -lwinmm

# 目录设置
SRC_DIR = components/net/xy_at
DRIVER_DIR = components/drivers/uart
BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin

# 源文件
SRCS = $(SRC_DIR)/examples/win32/wiindows_at.c \
       $(SRC_DIR)/at.c \
       $(DRIVER_DIR)/win32_uart.c

# 目标文件
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)

# 头文件路径
INCLUDES = -I. \
           -I$(SRC_DIR) \
           -I$(DRIVER_DIR)

# 目标可执行文件
TARGET = $(BIN_DIR)/at_demo.exe

# 默认目标
all: $(TARGET)

# 创建构建目录
$(BUILD_DIR)/%.o: %.c
    @mkdir -p $(@D)
    $(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 链接目标文件
$(TARGET): $(OBJS)
    @mkdir -p $(@D)
    $(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# 清理目标
clean:
    rm -rf $(BUILD_DIR)

# 运行程序
run: $(TARGET)
    $(TARGET)

.PHONY: all clean run