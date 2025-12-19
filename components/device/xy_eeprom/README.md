# EFLASH Component

Flash接口模拟EEPROM读写组件

## 概述

EFLASH是一个用Flash接口模拟EEPROM读写的软件组件。该组件通过PC Mock的方式模拟Flash的读、写、擦除操作，支持可配置的最小写入单元和页大小。

## 特性

- ✅ **可配置写入单元**: 支持32bit、64bit、128bit等不同的最小写入单元
- ✅ **可配置页大小**: 支持自定义页大小，如512字节、1KB、2KB等
- ✅ **多页支持**: 可配置多个Flash页
- ✅ **自动擦除**: 支持写入前自动擦除功能
- ✅ **地址对齐检查**: 确保读写地址和大小符合对齐要求
- ✅ **Flash特性模拟**: 模拟真实Flash只能将位从1变为0的特性
- ✅ **完整的错误处理**: 提供详细的错误码和状态反馈

## 文件结构

```
xy_eeprom/
├── eflash.h          # 头文件，包含API声明和数据结构定义
├── eflash.c          # 实现文件，包含所有功能的实现
├── eflash_test.c     # 测试文件，包含完整的单元测试
└── README.md         # 本文档
```

## API 接口

### 初始化和配置

#### eflash_init
```c
eflash_result_t eflash_init(eflash_handle_t *handle, const eflash_config_t *config);
```
初始化Flash设备，分配内存并设置配置参数。

**参数**:
- `handle`: Flash设备句柄指针
- `config`: 配置结构体指针

**返回值**: 操作结果状态码

#### eflash_deinit
```c
eflash_result_t eflash_deinit(eflash_handle_t *handle);
```
释放Flash设备资源。

### 读写操作

#### eflash_read
```c
eflash_result_t eflash_read(eflash_handle_t *handle, uint32_t address, uint8_t *data, size_t size);
```
从Flash读取数据。

**参数**:
- `handle`: Flash设备句柄
- `address`: 起始地址
- `data`: 读取数据的缓冲区
- `size`: 读取字节数

#### eflash_write
```c
eflash_result_t eflash_write(eflash_handle_t *handle, uint32_t address, const uint8_t *data, size_t size);
```
向Flash写入数据。地址和大小必须按照配置的写入单元对齐。

**参数**:
- `handle`: Flash设备句柄
- `address`: 起始地址（必须对齐）
- `data`: 要写入的数据
- `size`: 写入字节数（必须对齐）

### 擦除操作

#### eflash_erase_page
```c
eflash_result_t eflash_erase_page(eflash_handle_t *handle, uint32_t page_index);
```
擦除指定页。

#### eflash_erase_sector
```c
eflash_result_t eflash_erase_sector(eflash_handle_t *handle, uint32_t address);
```
擦除指定地址所在的扇区（页）。

#### eflash_erase_all
```c
eflash_result_t eflash_erase_all(eflash_handle_t *handle);
```
擦除整个Flash。

### 辅助函数

#### eflash_get_info
```c
eflash_result_t eflash_get_info(eflash_handle_t *handle, eflash_config_t *config);
```
获取Flash配置信息。

#### eflash_is_address_valid
```c
bool eflash_is_address_valid(eflash_handle_t *handle, uint32_t address, size_t size);
```
检查地址范围是否有效。

#### eflash_get_page_index
```c
uint32_t eflash_get_page_index(eflash_handle_t *handle, uint32_t address);
```
获取地址对应的页索引。

#### eflash_is_page_erased
```c
bool eflash_is_page_erased(eflash_handle_t *handle, uint32_t page_index);
```
检查指定页是否已擦除。

## 使用示例

### 基本使用

```c
#include "eflash.h"

int main(void)
{
    eflash_handle_t flash;
    eflash_config_t config = {
        .page_size = 512,                    // 页大小512字节
        .page_count = 16,                    // 16个页
        .write_unit = EFLASH_WRITE_UNIT_32BIT, // 32位写入单元
        .auto_erase = true                   // 自动擦除
    };

    // 初始化
    if (eflash_init(&flash, &config) != EFLASH_OK) {
        printf("初始化失败\n");
        return -1;
    }

    // 写入数据（必须4字节对齐）
    uint8_t write_data[32] = {0x11, 0x22, 0x33, 0x44, /* ... */};
    eflash_write(&flash, 0, write_data, 32);

    // 读取数据
    uint8_t read_data[32];
    eflash_read(&flash, 0, read_data, 32);

    // 清理
    eflash_deinit(&flash);

    return 0;
}
```

### 不同写入单元配置

```c
// 64位写入单元
eflash_config_t config_64bit = {
    .page_size = 1024,
    .page_count = 32,
    .write_unit = EFLASH_WRITE_UNIT_64BIT,  // 64位 = 8字节
    .auto_erase = true
};

// 128位写入单元
eflash_config_t config_128bit = {
    .page_size = 2048,
    .page_count = 64,
    .write_unit = EFLASH_WRITE_UNIT_128BIT, // 128位 = 16字节
    .auto_erase = true
};
```

### 手动擦除模式

```c
eflash_config_t config = {
    .page_size = 512,
    .page_count = 16,
    .write_unit = EFLASH_WRITE_UNIT_32BIT,
    .auto_erase = false  // 关闭自动擦除
};

eflash_init(&flash, &config);

// 手动擦除第一页
eflash_erase_page(&flash, 0);

// 写入数据
uint8_t data[4] = {0x12, 0x34, 0x56, 0x78};
eflash_write(&flash, 0, data, 4);
```

## 配置参数说明

### eflash_config_t

| 字段 | 类型 | 说明 |
|------|------|------|
| `total_size` | uint32_t | Flash总大小（字节），可设为0自动计算 |
| `page_size` | uint32_t | 页大小（字节），如512、1024、2048等 |
| `page_count` | uint32_t | 页数量 |
| `write_unit` | eflash_write_unit_t | 最小写入单元 |
| `auto_erase` | bool | 是否自动擦除 |

### 写入单元选项

- `EFLASH_WRITE_UNIT_32BIT`: 32位（4字节）
- `EFLASH_WRITE_UNIT_64BIT`: 64位（8字节）
- `EFLASH_WRITE_UNIT_128BIT`: 128位（16字节）

## 错误码

| 错误码 | 说明 |
|--------|------|
| `EFLASH_OK` | 操作成功 |
| `EFLASH_ERROR_INVALID_PARAM` | 无效参数 |
| `EFLASH_ERROR_OUT_OF_RANGE` | 地址超出范围 |
| `EFLASH_ERROR_ALIGNMENT` | 地址或大小未对齐 |
| `EFLASH_ERROR_WRITE_FAIL` | 写入失败 |
| `EFLASH_ERROR_ERASE_FAIL` | 擦除失败 |
| `EFLASH_ERROR_NOT_INIT` | 设备未初始化 |
| `EFLASH_ERROR_BUSY` | 设备忙 |

## 编译和测试

### 编译

```bash
# Windows (使用MSVC或MinGW)
gcc -o eflash_test eflash.c eflash_test.c

# Linux/Mac
gcc -o eflash_test eflash.c eflash_test.c
```

### 运行测试

```bash
./eflash_test
```

测试程序包含10个测试用例，涵盖：
1. 基本初始化
2. 32位写入单元读写
3. 64位写入单元
4. 128位写入单元
5. 页擦除
6. 多页操作
7. 对齐检查
8. 全擦除
9. 越界访问
10. 获取信息

## 注意事项

1. **对齐要求**: 写入操作的地址和大小必须按照配置的写入单元对齐
2. **Flash特性**: 模拟真实Flash特性，只能将位从1变为0，不能从0变为1（需要先擦除）
3. **内存管理**: 使用动态内存分配，注意在使用完毕后调用`eflash_deinit`释放资源
4. **页大小限制**: 页大小不能超过`EFLASH_MAX_PAGE_SIZE`（默认4096字节）
5. **页数量限制**: 页数量不能超过`EFLASH_MAX_PAGES`（默认64页）

## 版本历史

- **v1.0** (2025-10-22): 初始版本
  - 支持可配置的写入单元（32/64/128位）
  - 支持可配置的页大小和页数量
  - 完整的读写擦除功能
  - 完善的错误处理
  - 包含完整的单元测试

## 许可证

本组件遵循项目的整体许可证。

## 作者

XinYi Components Team

## 联系方式

如有问题或建议，请联系项目维护者。
