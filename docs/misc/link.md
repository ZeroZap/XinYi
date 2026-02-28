# 嵌入式编译平台链接脚本详解

链接脚本（Linker Script）是嵌入式开发中控制代码和数据在内存中布局的关键文件。下面详细介绍三大主流平台的链接脚本。

------

## 一、三大平台链接脚本概览

|   平台   | 链接脚本扩展名 | 链接器名称 |
| :------: | :------------: | :--------: |
| **IAR**  |     `.icf`     |   ILINK    |
| **KEIL** |     `.sct`     |  armlink   |
| **GCC**  |     `.ld`      |   GNU ld   |

------

## 二、GCC 链接脚本 (.ld)

### 基本结构

```ld
/* 内存区域定义 */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM (rwx)   : ORIGIN = 0x20000000, LENGTH = 128K
    CCMRAM (rw) : ORIGIN = 0x10000000, LENGTH = 64K
}

/* 入口点 */
ENTRY(Reset_Handler)

/* 段定义 */
SECTIONS
{
    /* 代码段 */
    .text :
    {
        . = ALIGN(4);
        _stext = .;
        KEEP(*(.isr_vector))    /* 中断向量表 */
        *(.text)                 /* 代码 */
        *(.text*)
        *(.rodata)               /* 只读数据 */
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* 已初始化数据段 */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH
    
    _sidata = LOADADDR(.data);  /* 数据在Flash中的加载地址 */

    /* 未初始化数据段 */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* 堆栈 */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > RAM
}
```

### 关键语法

|     语法     |            说明            |
| :----------: | :------------------------: |
|  `MEMORY{}`  |      定义物理内存区域      |
| `SECTIONS{}` |         定义输出段         |
|     `.`      |   位置计数器（当前地址）   |
|  `ALIGN(n)`  |          地址对齐          |
|   `KEEP()`   |       防止被垃圾回收       |
|    `AT>`     | 指定加载地址（VMA vs LMA） |
| `PROVIDE()`  |     定义可被覆盖的符号     |

------

## 三、KEIL 链接脚本 (.sct)

### 基本结构

```text
; 分散加载文件
LR_IROM1 0x08000000 0x00080000  {    ; 加载区域

    ER_IROM1 0x08000000 0x00080000  {  ; 执行区域1 (Flash)
        *.o (RESET, +First)             ; 中断向量表放最前
        *(InRoot$$Sections)             ; 库初始化代码
        .ANY (+RO)                      ; 所有只读段
        .ANY (+XO)                      ; 只执行段
    }
    
    RW_IRAM1 0x20000000 0x00020000  {  ; 执行区域2 (RAM)
        .ANY (+RW +ZI)                  ; 读写和零初始化段
    }
    
    ; CCM RAM 区域
    RW_CCMRAM 0x10000000 0x00010000 {
        *(.ccmram)                      ; 自定义段
    }
}
```

### 关键语法

|     语法      |             说明             |
| :-----------: | :--------------------------: |
|   `LR_xxx`    |   加载区域（Load Region）    |
|   `ER_xxx`    | 执行区域（Execution Region） |
|   `+First`    |       放置在区域最前面       |
| `+RO/+RW/+ZI` |    只读/读写/零初始化属性    |
|    `.ANY`     |       匹配任意目标文件       |
| `*(section)`  |          匹配特定段          |
|    `FIXED`    |           固定地址           |
|   `UNINIT`    |         不初始化区域         |

### 区域属性

```text
ER_REGION addr size [属性列表]
```

常用属性：

- `ABSOLUTE` - 绝对地址
- `FIXED` - 固定地址和大小
- `UNINIT` - 不进行零初始化
- `NOCOMPRESS` - 不压缩
- `PADVALUE` - 填充值

------

## 四、IAR 链接脚本 (.icf)

### 基本结构

```icf
/*###ICF### Section handled by ICF editor */

/* 定义内存区域 */
define symbol __ICFEDIT_region_ROM_start__ = 0x08000000;
define symbol __ICFEDIT_region_ROM_end__   = 0x0807FFFF;
define symbol __ICFEDIT_region_RAM_start__ = 0x20000000;
define symbol __ICFEDIT_region_RAM_end__   = 0x2001FFFF;

/* 堆栈大小 */
define symbol __ICFEDIT_size_cstack__ = 0x1000;
define symbol __ICFEDIT_size_heap__   = 0x1000;

/* 定义内存 */
define memory mem with size = 4G;

/* 定义区域 */
define region ROM_region = mem:[from __ICFEDIT_region_ROM_start__ 
                                to __ICFEDIT_region_ROM_end__];
define region RAM_region = mem:[from __ICFEDIT_region_RAM_start__ 
                                to __ICFEDIT_region_RAM_end__];

/* 定义块 */
define block CSTACK with alignment = 8, size = __ICFEDIT_size_cstack__ { };
define block HEAP   with alignment = 8, size = __ICFEDIT_size_heap__   { };

/* 初始化策略 */
initialize by copy { readwrite };
do not initialize  { section .noinit };

/* 放置规则 */
place at address mem:0x08000000 { readonly section .intvec };

place in ROM_region { readonly };
place in RAM_region { readwrite, block CSTACK, block HEAP };
```

### 关键语法

|         语法         |      说明      |
| :------------------: | :------------: |
|   `define symbol`    |  定义符号常量  |
|   `define memory`    |  定义地址空间  |
|   `define region`    |  定义内存区域  |
|    `define block`    |   定义内存块   |
|  `place at address`  | 放置到指定地址 |
|      `place in`      | 放置到指定区域 |
| `initialize by copy` |   复制初始化   |
| `do not initialize`  |    不初始化    |

------

## 五、三大平台对比

|      特性      | GCC (.ld) | KEIL (.sct) | IAR (.icf) |
| :------------: | :-------: | :---------: | :--------: |
|  **语法风格**  |  类C语言  |  层级结构   |   声明式   |
|  **学习曲线**  |   较陡    |    中等     |   较平缓   |
|   **灵活性**   |   最高    |     高      |    中等    |
|   **可读性**   |   中等    |     好      |    最好    |
|  **IDE支持**   |    无     | 图形编辑器  | 图形编辑器 |
| **表达式支持** |   完整    |    有限     |    有限    |

------

## 六、高阶用法

### 1. 自定义段放置

**GCC:**

```c
// C代码中定义
__attribute__((section(".ccmram"))) uint8_t fast_buffer[1024];

// 链接脚本中
.ccmram :
{
    . = ALIGN(4);
    *(.ccmram)
    *(.ccmram*)
} > CCMRAM
```

**KEIL:**

```c
// C代码中定义
__attribute__((section(".ARM.__at_0x10000000"))) uint8_t buffer[1024];
// 或
uint8_t buffer[1024] __attribute__((at(0x10000000)));
```

**IAR:**

```c
// C代码中定义
#pragma location = ".ccmram"
uint8_t fast_buffer[1024];
// 或
__no_init uint8_t buffer[1024] @ 0x10000000;
```

------

### 2. Bootloader + App 双区设计

**GCC:**

```ld
MEMORY
{
    BOOT_FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 32K
    APP_FLASH (rx)  : ORIGIN = 0x08008000, LENGTH = 480K
    RAM (rwx)       : ORIGIN = 0x20000000, LENGTH = 128K
}

/* Bootloader 链接脚本 */
SECTIONS
{
    .text : { *(.text*) } > BOOT_FLASH
}

/* App 链接脚本 */
SECTIONS
{
    .isr_vector : { KEEP(*(.isr_vector)) } > APP_FLASH
    .text : { *(.text*) } > APP_FLASH
}
```

**KEIL:**

```text
; App 分散加载文件
LR_APP 0x08008000 0x00078000 {
    ER_APP 0x08008000 0x00078000 {
        *.o (RESET, +First)
        .ANY (+RO +XO)
    }
    RW_RAM 0x20000000 0x00020000 {
        .ANY (+RW +ZI)
    }
}
```

**IAR:**

```icf
define symbol __APP_START__ = 0x08008000;
define region APP_ROM = mem:[from __APP_START__ to 0x0807FFFF];
place at address mem:__APP_START__ { readonly section .intvec };
place in APP_ROM { readonly };
```

------

### 3. 不初始化区域（保持复位前数据）

**GCC:**

```ld
.noinit (NOLOAD) :
{
    . = ALIGN(4);
    *(.noinit)
    *(.noinit*)
    . = ALIGN(4);
} > RAM
__attribute__((section(".noinit"))) uint32_t persistent_data;
```

**KEIL:**

```text
RW_NOINIT 0x20010000 UNINIT 0x1000 {
    *(.noinit)
}
```

**IAR:**

```icf
define block NOINIT with alignment = 4 { section .noinit };
do not initialize { section .noinit };
place in RAM_region { block NOINIT };
```

------

### 4. 函数放入RAM执行（提高速度）

**GCC:**

```ld
.ramfunc :
{
    . = ALIGN(4);
    _sramfunc = .;
    *(.ramfunc)
    *(.ramfunc*)
    . = ALIGN(4);
    _eramfunc = .;
} > RAM AT > FLASH

_siramfunc = LOADADDR(.ramfunc);
__attribute__((section(".ramfunc"))) void fast_function(void) {
    // 在RAM中执行的代码
}
```

**KEIL:**

```c
// 方法1：使用属性
__attribute__((section("RAMFUNC"))) void fast_func(void);

// 方法2：使用 __RAM_FUNC 宏（需定义）
#define __RAM_FUNC __attribute__((section("RAMFUNC")))
; 分散加载文件
RW_IRAM1 0x20000000 0x00020000 {
    *(RAMFUNC)
    .ANY (+RW +ZI)
}
```

**IAR:**

```c
__ramfunc void fast_function(void) {
    // 自动复制到RAM执行
}
```

------

### 5. 内存重叠检测与断言

**GCC:**

```ld
ASSERT(_estack <= ORIGIN(RAM) + LENGTH(RAM), "Stack overflow!")
ASSERT(_end_heap < _sstack, "Heap and stack collision!")
```

------

### 6. 多核/多Bank Flash 配置

**GCC:**

```ld
MEMORY
{
    FLASH_BANK1 (rx) : ORIGIN = 0x08000000, LENGTH = 256K
    FLASH_BANK2 (rx) : ORIGIN = 0x08040000, LENGTH = 256K
    RAM_D1 (rwx)     : ORIGIN = 0x20000000, LENGTH = 128K
    RAM_D2 (rwx)     : ORIGIN = 0x30000000, LENGTH = 32K
}

SECTIONS
{
    .text : { *(.text*) } > FLASH_BANK1
    .rodata : { *(.rodata*) } > FLASH_BANK2
    .data : { *(.data*) } > RAM_D1 AT > FLASH_BANK1
    .shared : { *(.shared*) } > RAM_D2
}
```

------

### 7. 版本信息固定地址

**GCC:**

```ld
.version 0x0800FF00 :
{
    KEEP(*(.version))
} > FLASH
__attribute__((section(".version"))) const struct {
    uint32_t major;
    uint32_t minor;
    char build_date[16];
} firmware_version = {1, 0, __DATE__};
```

------

### 8. 符号导出与使用

**链接脚本中定义:**

```ld
_flash_start = ORIGIN(FLASH);
_flash_size = LENGTH(FLASH);
_ram_start = ORIGIN(RAM);
_ram_size = LENGTH(RAM);
```

**C代码中使用:**

```c
extern uint32_t _flash_start;
extern uint32_t _flash_size;

void print_memory_info(void) {
    printf("Flash: 0x%08X, Size: %u KB\n", 
           (uint32_t)&_flash_start, 
           (uint32_t)&_flash_size / 1024);
}
```

------

## 七、调试技巧

### 查看内存映射

|   平台   |                      命令/方法                      |
| :------: | :-------------------------------------------------: |
| **GCC**  | `arm-none-eabi-nm -S output.elf` 或查看 `.map` 文件 |
| **KEIL** |        查看 `.map` 文件或 Build Output 窗口         |
| **IAR**  |         查看 `.map` 文件或 Linker Map 窗口          |

### 常见问题排查

1. **链接错误 "region overflow"** → 代码/数据超出定义的内存区域
2. **Hard Fault** → 检查栈大小、对齐问题
3. **数据未初始化** → 检查启动代码是否正确复制 `.data` 段
4. **变量值异常** → 检查 `.bss` 段是否正确清零

------

如需针对特定芯片或场景的链接脚本示例，请告诉我具体需求！





```
STM32在各种编译平台链接脚本及各种 RTOS 下的用法
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

# STM32 各编译平台链接脚本及RTOS配置详解

------

## 一、STM32 内存架构概览

### 典型STM32系列内存布局

|    系列     | Flash起始  | Flash大小 |  RAM起始   |   RAM类型    |
| :---------: | :--------: | :-------: | :--------: | :----------: |
| **STM32F1** | 0x08000000 | 64K-512K  | 0x20000000 |     SRAM     |
| **STM32F4** | 0x08000000 |  512K-2M  | 0x20000000 |   SRAM+CCM   |
| **STM32F7** | 0x08000000 |  512K-2M  | 0x20000000 | DTCM+SRAM1/2 |
| **STM32H7** | 0x08000000 |   1M-2M   | 0x20000000 | DTCM+多域RAM |
| **STM32L4** | 0x08000000 |  256K-1M  | 0x20000000 | SRAM1+SRAM2  |

### STM32H7 复杂内存架构示例

```text
┌─────────────────────────────────────────────────────────┐
│                    STM32H743 内存映射                    │
├─────────────────────────────────────────────────────────┤
│ ITCM-RAM    │ 0x00000000 │ 64KB  │ 指令紧耦合(最快)     │
│ DTCM-RAM    │ 0x20000000 │ 128KB │ 数据紧耦合(最快)     │
│ AXI-SRAM    │ 0x24000000 │ 512KB │ D1域                 │
│ SRAM1       │ 0x30000000 │ 128KB │ D2域                 │
│ SRAM2       │ 0x30020000 │ 128KB │ D2域                 │
│ SRAM3       │ 0x30040000 │ 32KB  │ D2域                 │
│ SRAM4       │ 0x38000000 │ 64KB  │ D3域(低功耗可访问)   │
│ Backup SRAM │ 0x38800000 │ 4KB   │ 备份域               │
│ FLASH       │ 0x08000000 │ 2MB   │ 双Bank               │
└─────────────────────────────────────────────────────────┘
```

------

## 二、GCC 平台完整链接脚本

### 1. STM32F4 基础链接脚本

```ld
/* STM32F407VGT6 链接脚本 */
/* 入口点 */
ENTRY(Reset_Handler)

/* 堆栈配置 */
_Min_Heap_Size  = 0x200;   /* 512 bytes */
_Min_Stack_Size = 0x400;   /* 1KB */

/* 内存定义 */
MEMORY
{
    FLASH (rx)     : ORIGIN = 0x08000000, LENGTH = 1024K
    RAM (xrw)      : ORIGIN = 0x20000000, LENGTH = 128K
    CCMRAM (rw)    : ORIGIN = 0x10000000, LENGTH = 64K
}

SECTIONS
{
    /* 中断向量表 - 必须放在Flash起始位置 */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* 代码段 */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)         /* ARM-Thumb 胶水代码 */
        *(.glue_7t)
        *(.eh_frame)

        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* 只读数据 */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* ARM 异常处理 */
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    /* C++ 构造/析构函数 */
    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    _sidata = LOADADDR(.data);

    /* 已初始化数据 - 从Flash加载到RAM */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        *(.RamFunc)        /* RAM中执行的函数 */
        *(.RamFunc*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    /* CCM RAM 数据 */
    _siccmram = LOADADDR(.ccmram);
    
    .ccmram :
    {
        . = ALIGN(4);
        _sccmram = .;
        *(.ccmram)
        *(.ccmram*)
        . = ALIGN(4);
        _eccmram = .;
    } > CCMRAM AT > FLASH

    /* 未初始化数据 */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > RAM

    /* 不初始化区域 - 复位后保持数据 */
    .noinit (NOLOAD) :
    {
        . = ALIGN(4);
        _snoinit = .;
        *(.noinit)
        *(.noinit*)
        . = ALIGN(4);
        _enoinit = .;
    } > RAM

    /* 堆和栈 */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > RAM

    /* 栈顶指针 */
    _estack = ORIGIN(RAM) + LENGTH(RAM);

    /* 移除调试信息 */
    /DISCARD/ :
    {
        libc.a(*)
        libm.a(*)
        libgcc.a(*)
    }

    /* 导出符号供启动代码使用 */
    .ARM.attributes 0 : { *(.ARM.attributes) }
}

/* 断言检查 */
ASSERT(_estack >= _ebss + _Min_Stack_Size, "RAM overflow")
```

### 2. STM32H7 多域RAM链接脚本

```ld
/* STM32H743 链接脚本 - 多域RAM配置 */
ENTRY(Reset_Handler)

_Min_Heap_Size  = 0x1000;
_Min_Stack_Size = 0x1000;

MEMORY
{
    ITCMRAM (xrw)   : ORIGIN = 0x00000000, LENGTH = 64K
    DTCMRAM (xrw)   : ORIGIN = 0x20000000, LENGTH = 128K
    FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 2048K
    AXIRAM (xrw)    : ORIGIN = 0x24000000, LENGTH = 512K
    SRAM1 (xrw)     : ORIGIN = 0x30000000, LENGTH = 128K
    SRAM2 (xrw)     : ORIGIN = 0x30020000, LENGTH = 128K
    SRAM3 (xrw)     : ORIGIN = 0x30040000, LENGTH = 32K
    SRAM4 (xrw)     : ORIGIN = 0x38000000, LENGTH = 64K
    BKPSRAM (rw)    : ORIGIN = 0x38800000, LENGTH = 4K
}

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    _sidata = LOADADDR(.data);

    /* 主数据段 - 放在DTCM (最快访问) */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        *(.RamFunc)
        *(.RamFunc*)
        . = ALIGN(4);
        _edata = .;
    } > DTCMRAM AT > FLASH

    /* ITCM中执行的关键代码 */
    _siitcm = LOADADDR(.itcm);
    .itcm :
    {
        . = ALIGN(4);
        _sitcm = .;
        *(.itcm)
        *(.itcm*)
        . = ALIGN(4);
        _eitcm = .;
    } > ITCMRAM AT > FLASH

    /* AXI RAM - 大缓冲区 */
    _siaxi = LOADADDR(.axiram);
    .axiram :
    {
        . = ALIGN(4);
        _saxi = .;
        *(.axiram)
        *(.axiram*)
        . = ALIGN(4);
        _eaxi = .;
    } > AXIRAM AT > FLASH

    /* D2域 SRAM - DMA缓冲区 */
    .sram1 (NOLOAD) :
    {
        . = ALIGN(4);
        _ssram1 = .;
        *(.sram1)
        *(.sram1*)
        . = ALIGN(4);
        _esram1 = .;
    } > SRAM1

    .sram2 (NOLOAD) :
    {
        . = ALIGN(4);
        _ssram2 = .;
        *(.sram2)
        *(.sram2*)
        . = ALIGN(4);
        _esram2 = .;
    } > SRAM2

    /* D3域 SRAM4 - 低功耗模式可访问 */
    .sram4 (NOLOAD) :
    {
        . = ALIGN(4);
        _ssram4 = .;
        *(.sram4)
        *(.sram4*)
        . = ALIGN(4);
        _esram4 = .;
    } > SRAM4

    /* 备份SRAM - 电池供电保持 */
    .bkpsram (NOLOAD) :
    {
        . = ALIGN(4);
        _sbkp = .;
        *(.bkpsram)
        *(.bkpsram*)
        . = ALIGN(4);
        _ebkp = .;
    } > BKPSRAM

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > DTCMRAM

    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > DTCMRAM

    _estack = ORIGIN(DTCMRAM) + LENGTH(DTCMRAM);

    /DISCARD/ :
    {
        libc.a(*)
        libm.a(*)
        libgcc.a(*)
    }
}
```

### 3. STM32H7 C代码中使用各内存域

```c
/* 放置到ITCM - 高速执行代码 */
__attribute__((section(".itcm"))) 
void critical_isr_handler(void) {
    // 中断处理代码
}

/* 放置到AXI RAM - 大缓冲区 */
__attribute__((section(".axiram"))) 
uint8_t frame_buffer[800*480*2];

/* 放置到SRAM1 - DMA缓冲区 (D2域) */
__attribute__((section(".sram1"), aligned(32))) 
uint8_t dma_rx_buffer[4096];

__attribute__((section(".sram2"), aligned(32))) 
uint8_t dma_tx_buffer[4096];

/* 放置到SRAM4 - 低功耗模式保持 */
__attribute__((section(".sram4"))) 
uint32_t lowpower_data[256];

/* 放置到备份SRAM - 电池保持 */
__attribute__((section(".bkpsram"))) 
struct {
    uint32_t magic;
    uint32_t boot_count;
    uint8_t  config[64];
} backup_data;

/* 不初始化区域 - 复位保持 */
__attribute__((section(".noinit"))) 
uint32_t reset_reason;
```

------

## 三、KEIL 平台链接脚本

### 1. STM32F4 分散加载文件

```text
; STM32F407VGT6 分散加载文件
; Flash: 1MB, RAM: 128KB, CCM: 64KB

LR_IROM1 0x08000000 0x00100000  {    ; 加载区域 1MB

    ER_IROM1 0x08000000 0x00100000  { ; 执行区域 Flash
        *.o (RESET, +First)           ; 中断向量表
        *(InRoot$$Sections)           ; ARM库初始化
        .ANY (+RO)                    ; 所有只读代码和数据
        .ANY (+XO)
    }
    
    RW_IRAM1 0x20000000 0x00020000  { ; 执行区域 RAM 128KB
        .ANY (+RW +ZI)                ; 读写和零初始化数据
    }
    
    RW_CCMRAM 0x10000000 0x00010000 { ; CCM RAM 64KB
        *(.ccmram)                    ; 自定义CCM段
        *(ccmram)
    }
}
```

### 2. STM32F4 高级分散加载文件

```text
; STM32F407 高级配置

LR_IROM1 0x08000000 0x00100000 {

    ; Flash 执行区域
    ER_IROM1 0x08000000 0x00100000 {
        *.o (RESET, +First)
        *(InRoot$$Sections)
        .ANY (+RO)
    }
    
    ; 主RAM区域
    RW_IRAM1 0x20000000 0x0001C000 {
        .ANY (+RW +ZI)
    }
    
    ; RAM中执行的函数
    ER_RAMFUNC 0x2001C000 0x00002000 {
        *(RAMFUNC)
        *(.ramfunc)
    }
    
    ; 不初始化区域 - 复位保持
    RW_NOINIT 0x2001E000 UNINIT 0x00002000 {
        *(.noinit)
        *(noinit)
    }
    
    ; CCM RAM - 快速访问
    RW_CCMRAM 0x10000000 0x00010000 {
        *(.ccmram)
        *(ccmram)
        ; 可以指定特定文件
        ; dsp_math.o (+RW +ZI)
    }
}
```

### 3. STM32H7 KEIL 分散加载文件

```text
; STM32H743 分散加载文件 - 多域RAM

LR_IROM1 0x08000000 0x00200000 {    ; 2MB Flash

    ; Flash 代码区
    ER_IROM1 0x08000000 0x00200000 {
        *.o (RESET, +First)
        *(InRoot$$Sections)
        .ANY (+RO)
    }
    
    ; DTCM - 主数据区 (最快)
    RW_DTCMRAM 0x20000000 0x00020000 {
        .ANY (+RW +ZI)
    }
    
    ; ITCM - 关键代码执行
    ER_ITCMRAM 0x00000000 0x00010000 {
        *(ITCM)
        *(.itcm)
        ; critical_code.o (+RO)
    }
    
    ; AXI SRAM - 大缓冲区
    RW_AXIRAM 0x24000000 0x00080000 {
        *(AXIRAM)
        *(.axiram)
        ; framebuffer.o (+RW +ZI)
    }
    
    ; SRAM1 - DMA缓冲区 (D2域)
    RW_SRAM1 0x30000000 UNINIT 0x00020000 {
        *(SRAM1)
        *(.sram1)
        ; 以太网DMA缓冲区
        ; ethernetif.o (+RW +ZI)
    }
    
    ; SRAM2 - DMA缓冲区 (D2域)
    RW_SRAM2 0x30020000 UNINIT 0x00020000 {
        *(SRAM2)
        *(.sram2)
    }
    
    ; SRAM4 - 低功耗保持 (D3域)
    RW_SRAM4 0x38000000 UNINIT 0x00010000 {
        *(SRAM4)
        *(.sram4)
    }
    
    ; 备份SRAM
    RW_BKPSRAM 0x38800000 UNINIT 0x00001000 {
        *(BKPSRAM)
        *(.bkpsram)
    }
}
```

### 4. KEIL C代码中使用

```c
/* 方法1: 使用 section 属性 */
__attribute__((section("CCMRAM"))) uint8_t ccm_buffer[1024];
__attribute__((section("SRAM1"))) uint8_t dma_buffer[4096];
__attribute__((section("RAMFUNC"))) void fast_func(void);

/* 方法2: 使用 at 属性指定绝对地址 */
uint32_t fixed_var __attribute__((at(0x20000000)));

/* 方法3: 使用 ARM 编译器扩展 */
#pragma arm section rwdata = "CCMRAM"
uint8_t ccm_data[512];
#pragma arm section

/* 方法4: 不初始化变量 */
__attribute__((section(".noinit"), zero_init)) uint32_t noinit_var;

/* 方法5: RAM函数 */
__attribute__((section("RAMFUNC")))
void flash_program(uint32_t addr, uint32_t data) {
    // Flash编程代码 - 必须在RAM中执行
}
```

------

## 四、IAR 平台链接脚本

### 1. STM32F4 ICF文件

```icf
/*###ICF### Section handled by ICF editor, don't touch! ****/
/*-Editor annotation file-*/
/* IcfEditorFile="$TOOLKIT_DIR$\config\ide\IcfEditor\cortex_v1_0.xml" */

/*-Specials-*/
define symbol __ICFEDIT_intvec_start__ = 0x08000000;

/*-Memory Regions-*/
define symbol __ICFEDIT_region_ROM_start__    = 0x08000000;
define symbol __ICFEDIT_region_ROM_end__      = 0x080FFFFF;
define symbol __ICFEDIT_region_RAM_start__    = 0x20000000;
define symbol __ICFEDIT_region_RAM_end__      = 0x2001FFFF;
define symbol __ICFEDIT_region_CCMRAM_start__ = 0x10000000;
define symbol __ICFEDIT_region_CCMRAM_end__   = 0x1000FFFF;

/*-Sizes-*/
define symbol __ICFEDIT_size_cstack__ = 0x1000;
define symbol __ICFEDIT_size_heap__   = 0x1000;
/**** End of ICF editor section. ###ICF###*/

define memory mem with size = 4G;

define region ROM_region    = mem:[from __ICFEDIT_region_ROM_start__ 
                                   to __ICFEDIT_region_ROM_end__];
define region RAM_region    = mem:[from __ICFEDIT_region_RAM_start__ 
                                   to __ICFEDIT_region_RAM_end__];
define region CCMRAM_region = mem:[from __ICFEDIT_region_CCMRAM_start__ 
                                   to __ICFEDIT_region_CCMRAM_end__];

define block CSTACK with alignment = 8, size = __ICFEDIT_size_cstack__ { };
define block HEAP   with alignment = 8, size = __ICFEDIT_size_heap__   { };

initialize by copy { readwrite };
do not initialize  { section .noinit };

place at address mem:__ICFEDIT_intvec_start__ { readonly section .intvec };

place in ROM_region    { readonly };
place in RAM_region    { readwrite, block CSTACK, block HEAP };
place in CCMRAM_region { section .ccmram };
```

### 2. STM32H7 ICF文件

```icf
/*###ICF### STM32H743 配置 ###*/

/*-Memory Regions-*/
define symbol __ICFEDIT_region_ITCMRAM_start__ = 0x00000000;
define symbol __ICFEDIT_region_ITCMRAM_end__   = 0x0000FFFF;
define symbol __ICFEDIT_region_DTCMRAM_start__ = 0x20000000;
define symbol __ICFEDIT_region_DTCMRAM_end__   = 0x2001FFFF;
define symbol __ICFEDIT_region_FLASH_start__   = 0x08000000;
define symbol __ICFEDIT_region_FLASH_end__     = 0x081FFFFF;
define symbol __ICFEDIT_region_AXIRAM_start__  = 0x24000000;
define symbol __ICFEDIT_region_AXIRAM_end__    = 0x2407FFFF;
define symbol __ICFEDIT_region_SRAM1_start__   = 0x30000000;
define symbol __ICFEDIT_region_SRAM1_end__     = 0x3001FFFF;
define symbol __ICFEDIT_region_SRAM2_start__   = 0x30020000;
define symbol __ICFEDIT_region_SRAM2_end__     = 0x3003FFFF;
define symbol __ICFEDIT_region_SRAM3_start__   = 0x30040000;
define symbol __ICFEDIT_region_SRAM3_end__     = 0x30047FFF;
define symbol __ICFEDIT_region_SRAM4_start__   = 0x38000000;
define symbol __ICFEDIT_region_SRAM4_end__     = 0x3800FFFF;
define symbol __ICFEDIT_region_BKPSRAM_start__ = 0x38800000;
define symbol __ICFEDIT_region_BKPSRAM_end__   = 0x38800FFF;

/*-Sizes-*/
define symbol __ICFEDIT_size_cstack__ = 0x2000;
define symbol __ICFEDIT_size_heap__   = 0x2000;

define memory mem with size = 4G;

define region ITCMRAM_region = mem:[from __ICFEDIT_region_ITCMRAM_start__ 
                                    to __ICFEDIT_region_ITCMRAM_end__];
define region DTCMRAM_region = mem:[from __ICFEDIT_region_DTCMRAM_start__ 
                                    to __ICFEDIT_region_DTCMRAM_end__];
define region FLASH_region   = mem:[from __ICFEDIT_region_FLASH_start__ 
                                    to __ICFEDIT_region_FLASH_end__];
define region AXIRAM_region  = mem:[from __ICFEDIT_region_AXIRAM_start__ 
                                    to __ICFEDIT_region_AXIRAM_end__];
define region SRAM1_region   = mem:[from __ICFEDIT_region_SRAM1_start__ 
                                    to __ICFEDIT_region_SRAM1_end__];
define region SRAM2_region   = mem:[from __ICFEDIT_region_SRAM2_start__ 
                                    to __ICFEDIT_region_SRAM2_end__];
define region SRAM4_region   = mem:[from __ICFEDIT_region_SRAM4_start__ 
                                    to __ICFEDIT_region_SRAM4_end__];
define region BKPSRAM_region = mem:[from __ICFEDIT_region_BKPSRAM_start__ 
                                    to __ICFEDIT_region_BKPSRAM_end__];

define block CSTACK with alignment = 8, size = __ICFEDIT_size_cstack__ { };
define block HEAP   with alignment = 8, size = __ICFEDIT_size_heap__   { };

initialize by copy { readwrite };
initialize by copy { section .itcm };
initialize by copy { section .axiram };
do not initialize  { section .noinit };
do not initialize  { section .sram1, section .sram2, section .sram4 };
do not initialize  { section .bkpsram };

place at address mem:0x08000000 { readonly section .intvec };

place in FLASH_region   { readonly };
place in DTCMRAM_region { readwrite, block CSTACK, block HEAP };
place in ITCMRAM_region { section .itcm };
place in AXIRAM_region  { section .axiram };
place in SRAM1_region   { section .sram1 };
place in SRAM2_region   { section .sram2 };
place in SRAM4_region   { section .sram4 };
place in BKPSRAM_region { section .bkpsram };
```

### 3. IAR C代码中使用

```c
/* 方法1: 使用 #pragma location */
#pragma location = ".ccmram"
uint8_t ccm_buffer[1024];

/* 方法2: 使用 @ 操作符 */
uint8_t dma_buffer[4096] @ ".sram1";

/* 方法3: 指定绝对地址 */
__no_init uint32_t fixed_var @ 0x20000000;

/* 方法4: 不初始化变量 */
__no_init uint32_t noinit_var @ ".noinit";

/* 方法5: RAM函数 */
__ramfunc void flash_program(uint32_t addr, uint32_t data) {
    // 自动复制到RAM执行
}

/* 方法6: ITCM中的关键代码 */
#pragma location = ".itcm"
void critical_handler(void) {
    // 高速执行
}

/* 方法7: 备份SRAM */
#pragma location = ".bkpsram"
__no_init struct {
    uint32_t magic;
    uint32_t data[100];
} backup_data;
```

------

## 五、FreeRTOS 配置

### 1. GCC + FreeRTOS 链接脚本

```ld
/* FreeRTOS 专用链接脚本 */
ENTRY(Reset_Handler)

/* FreeRTOS 堆配置 - 使用 heap_4.c */
_Min_Heap_Size  = 0x0;      /* FreeRTOS管理堆,系统堆可设为0 */
_Min_Stack_Size = 0x400;    /* 主栈(MSP) - 用于中断 */

/* FreeRTOS 总堆大小 - 在 FreeRTOSConfig.h 中配置 */
/* #define configTOTAL_HEAP_SIZE ((size_t)(64 * 1024)) */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
    RAM (xrw)   : ORIGIN = 0x20000000, LENGTH = 128K
    CCMRAM (rw) : ORIGIN = 0x10000000, LENGTH = 64K
}

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        *(.RamFunc)
        *(.RamFunc*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    /* FreeRTOS 堆放在CCM中 - 提高性能 */
    .freertos_heap (NOLOAD) :
    {
        . = ALIGN(8);
        _freertos_heap_start = .;
        *(.freertos_heap)
        . = ALIGN(8);
        _freertos_heap_end = .;
    } > CCMRAM

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > RAM

    /* 主栈 - 用于中断处理 */
    ._main_stack :
    {
        . = ALIGN(8);
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > RAM

    _estack = ORIGIN(RAM) + LENGTH(RAM);
}
```

### 2. FreeRTOS 堆配置 (heap_4.c 修改)

```c
/* FreeRTOSConfig.h */
#define configTOTAL_HEAP_SIZE    ((size_t)(60 * 1024))
#define configAPPLICATION_ALLOCATED_HEAP 1  /* 使用自定义堆位置 */

/* 在C文件中定义堆位置 */
#if (configAPPLICATION_ALLOCATED_HEAP == 1)
    /* 方法1: 放在CCM中 */
    __attribute__((section(".freertos_heap"), aligned(8)))
    uint8_t ucHeap[configTOTAL_HEAP_SIZE];
    
    /* 方法2: 放在AXI RAM中 (STM32H7) */
    // __attribute__((section(".axiram"), aligned(8)))
    // uint8_t ucHeap[configTOTAL_HEAP_SIZE];
#endif
```

### 3. FreeRTOS 任务栈放置

```c
/* 静态任务 - 栈放在指定内存 */

/* 任务栈放在CCM */
__attribute__((section(".ccmram")))
static StackType_t xTask1Stack[512];
static StaticTask_t xTask1TCB;

/* 任务栈放在普通RAM */
static StackType_t xTask2Stack[256];
static StaticTask_t xTask2TCB;

void create_tasks(void) {
    /* 创建静态任务 */
    xTaskCreateStatic(
        task1_function,
        "Task1",
        512,
        NULL,
        2,
        xTask1Stack,
        &xTask1TCB
    );
    
    xTaskCreateStatic(
        task2_function,
        "Task2", 
        256,
        NULL,
        1,
        xTask2Stack,
        &xTask2TCB
    );
}
```

### 4. FreeRTOS + STM32H7 多域配置

```c
/* FreeRTOSConfig.h - STM32H7 优化配置 */
#define configTOTAL_HEAP_SIZE           ((size_t)(128 * 1024))
#define configAPPLICATION_ALLOCATED_HEAP 1
#define configSUPPORT_STATIC_ALLOCATION  1
#define configSUPPORT_DYNAMIC_ALLOCATION 1

/* 堆放在AXI RAM - 大容量 */
__attribute__((section(".axiram"), aligned(8)))
uint8_t ucHeap[configTOTAL_HEAP_SIZE];

/* 高优先级任务栈放在DTCM - 最快访问 */
__attribute__((section(".dtcmram")))
static StackType_t xHighPriorityTaskStack[1024];

/* DMA任务栈放在D2域SRAM */
__attribute__((section(".sram1")))
static StackType_t xDMATaskStack[512];

/* 空闲任务和定时器任务配置 */
__attribute__((section(".dtcmram")))
static StackType_t xIdleTaskStack[configMINIMAL_STACK_SIZE];
static StaticTask_t xIdleTaskTCB;

__attribute__((section(".dtcmram")))
static StackType_t xTimerTaskStack[configTIMER_TASK_STACK_DEPTH];
static StaticTask_t xTimerTaskTCB;

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize) {
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = xIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize) {
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = xTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
```

------

## 六、RT-Thread 配置

### 1. GCC + RT-Thread 链接脚本

```ld
/* RT-Thread 链接脚本 */
ENTRY(Reset_Handler)

/* RT-Thread 使用自己的堆管理 */
_system_stack_size = 0x400;  /* 主栈大小 */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM (xrw)   : ORIGIN = 0x20000000, LENGTH = 128K
    CCMRAM (rw) : ORIGIN = 0x10000000, LENGTH = 64K
}

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        
        /* RT-Thread 组件自动初始化 */
        . = ALIGN(4);
        __rt_init_start = .;
        KEEP(*(SORT(.rti_fn*)))
        __rt_init_end = .;
        
        /* Finsh 命令表 */
        . = ALIGN(4);
        __fsymtab_start = .;
        KEEP(*(FSymTab))
        __fsymtab_end = .;
        
        . = ALIGN(4);
        __vsymtab_start = .;
        KEEP(*(VSymTab))
        __vsymtab_end = .;
        
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        
        KEEP(*(.init))
        KEEP(*(.fini))
        
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        *(.RamFunc)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    .bss :
    {
        . = ALIGN(4);
        __bss_start = .;
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end = .;
    } > RAM

    /* RT-Thread 主栈 */
    .stack :
    {
        . = ALIGN(8);
        _sstack = .;
        . = . + _system_stack_size;
        . = ALIGN(8);
        _estack = .;
        __StackTop = .;
    } > RAM

    /* RT-Thread 堆 - 剩余RAM空间 */
    .heap :
    {
        . = ALIGN(8);
        __heap_start = .;
        PROVIDE(__heap_begin = .);
        . = ORIGIN(RAM) + LENGTH(RAM);
        __heap_end = .;
        PROVIDE(__heap_limit = .);
    } > RAM

    /* CCM RAM */
    .ccmram :
    {
        . = ALIGN(4);
        *(.ccmram)
        *(.ccmram*)
        . = ALIGN(4);
    } > CCMRAM
}
```

### 2. RT-Thread board.c 堆配置

```c
/* board.c */
#include <rtthread.h>
#include <board.h>

/* 从链接脚本获取堆边界 */
extern int __heap_start;
extern int __heap_end;

/* 堆初始化 */
void rt_hw_board_init(void)
{
    /* 系统时钟配置 */
    SystemClock_Config();
    
    /* SysTick 配置 */
    SysTick_Config(SystemCoreClock / RT_TICK_PER_SECOND);
    
    /* 堆初始化 */
    rt_system_heap_init((void *)&__heap_start, (void *)&__heap_end);
    
    /* USART 初始化 (用于 rt_kprintf) */
    rt_hw_usart_init();
    
#ifdef RT_USING_CONSOLE
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif

#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif
}
```

### 3. RT-Thread 多内存堆配置

```c
/* 使用 memheap 管理多个内存区域 */
#include <rtthread.h>

/* CCM RAM 作为额外堆 */
__attribute__((section(".ccmram")))
static uint8_t ccm_heap[64 * 1024];
static struct rt_memheap ccm_memheap;

/* AXI RAM 作为额外堆 (STM32H7) */
__attribute__((section(".axiram")))
static uint8_t axi_heap[256 * 1024];
static struct rt_memheap axi_memheap;

int rt_hw_memheap_init(void)
{
    /* 初始化CCM堆 */
    rt_memheap_init(&ccm_memheap, "ccm", ccm_heap, sizeof(ccm_heap));
    
    /* 初始化AXI堆 */
    rt_memheap_init(&axi_memheap, "axi", axi_heap, sizeof(axi_heap));
    
    return 0;
}
INIT_BOARD_EXPORT(rt_hw_memheap_init);

/* 从指定堆分配内存 */
void *ccm_malloc(rt_size_t size)
{
    return rt_memheap_alloc(&ccm_memheap, size);
}

void ccm_free(void *ptr)
{
    rt_memheap_free(ptr);
}

void *axi_malloc(rt_size_t size)
{
    return rt_memheap_alloc(&axi_memheap, size);
}

void axi_free(void *ptr)
{
    rt_memheap_free(ptr);
}
```

------

## 七、ThreadX (Azure RTOS) 配置

### 1. GCC + ThreadX 链接脚本

```ld
/* ThreadX 链接脚本 */
ENTRY(Reset_Handler)

_Min_Stack_Size = 0x400;

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
    RAM (xrw)   : ORIGIN = 0x20000000, LENGTH = 128K
    CCMRAM (rw) : ORIGIN = 0x10000000, LENGTH = 64K
}

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        *(.RamFunc)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > RAM

    /* ThreadX 字节池 */
    .tx_byte_pool (NOLOAD) :
    {
        . = ALIGN(8);
        __tx_byte_pool_start = .;
        *(.tx_byte_pool)
        . = ALIGN(8);
    } > RAM

    /* ThreadX 块池 */
    .tx_block_pool (NOLOAD) :
    {
        . = ALIGN(8);
        __tx_block_pool_start = .;
        *(.tx_block_pool)
        . = ALIGN(8);
    } > RAM

    /* 主栈 */
    ._user_stack :
    {
        . = ALIGN(8);
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > RAM

    /* ThreadX 空闲内存 - 用于动态分配 */
    .tx_free_mem (NOLOAD) :
    {
        . = ALIGN(8);
        __tx_free_memory_start = .;
        . = ORIGIN(RAM) + LENGTH(RAM);
        __tx_free_memory_end = .;
    } > RAM

    _estack = ORIGIN(RAM) + LENGTH(RAM);
}
```

### 2. ThreadX 内存池配置

```c
/* tx_application.c */
#include "tx_api.h"

/* 从链接脚本获取内存边界 */
extern UCHAR __tx_free_memory_start;
extern UCHAR __tx_free_memory_end;

/* 字节池 */
TX_BYTE_POOL byte_pool_0;
#define BYTE_POOL_SIZE  (64 * 1024)

/* 块池 - 用于固定大小分配 */
TX_BLOCK_POOL block_pool_0;
#define BLOCK_POOL_SIZE (32 * 1024)
#define BLOCK_SIZE      64

/* 线程栈 */
#define THREAD_STACK_SIZE 1024

/* 线程控制块 */
TX_THREAD thread_0;
TX_THREAD thread_1;

/* 内存区域 */
__attribute__((section(".tx_byte_pool")))
UCHAR byte_pool_memory[BYTE_POOL_SIZE];

__attribute__((section(".tx_block_pool")))
UCHAR block_pool_memory[BLOCK_POOL_SIZE];

void tx_application_define(void *first_unused_memory)
{
    CHAR *pointer;
    
    /* 创建字节池 */
    tx_byte_pool_create(&byte_pool_0, "byte pool 0",
                        byte_pool_memory, BYTE_POOL_SIZE);
    
    /* 创建块池 */
    tx_block_pool_create(&block_pool_0, "block pool 0",
                         BLOCK_SIZE, block_pool_memory, BLOCK_POOL_SIZE);
    
    /* 从字节池分配线程栈 */
    tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, 
                     THREAD_STACK_SIZE, TX_NO_WAIT);
    
    /* 创建线程 */
    tx_thread_create(&thread_0, "thread 0",
                     thread_0_entry, 0,
                     pointer, THREAD_STACK_SIZE,
                     1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);
    
    /* 分配第二个线程栈 */
    tx_byte_allocate(&byte_pool_0, (VOID **)&pointer,
                     THREAD_STACK_SIZE, TX_NO_WAIT);
    
    tx_thread_create(&thread_1, "thread 1",
                     thread_1_entry, 0,
                     pointer, THREAD_STACK_SIZE,
                     2, 2, TX_NO_TIME_SLICE, TX_AUTO_START);
}
```

------

## 八、μC/OS-III 配置

### 1. GCC + μC/OS-III 链接脚本

```ld
/* uC/OS-III 链接脚本 */
ENTRY(Reset_Handler)

_Min_Stack_Size = 0x400;

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM (xrw)   : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        *(.RamFunc)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > RAM

    /* uC/OS-III 任务栈区域 */
    .os_stacks (NOLOAD) :
    {
        . = ALIGN(8);
        __os_stacks_start = .;
        *(.os_stacks)
        . = ALIGN(8);
        __os_stacks_end = .;
    } > RAM

    /* 主栈 */
    ._user_stack :
    {
        . = ALIGN(8);
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > RAM

    /* 堆 */
    .heap (NOLOAD) :
    {
        . = ALIGN(8);
        __heap_start__ = .;
        . = ORIGIN(RAM) + LENGTH(RAM);
        __heap_end__ = .;
    } > RAM

    _estack = ORIGIN(RAM) + LENGTH(RAM);
}
```

### 2. μC/OS-III 任务配置

```c
/* app.c */
#include "os.h"

/* 任务栈 - 放在指定段 */
__attribute__((section(".os_stacks")))
static CPU_STK AppTaskStartStk[512];

__attribute__((section(".os_stacks")))
static CPU_STK AppTask1Stk[256];

__attribute__((section(".os_stacks")))
static CPU_STK AppTask2Stk[256];

/* 任务控制块 */
static OS_TCB AppTaskStartTCB;
static OS_TCB AppTask1TCB;
static OS_TCB AppTask2TCB;

int main(void)
{
    OS_ERR err;
    
    /* 硬件初始化 */
    HAL_Init();
    SystemClock_Config();
    
    /* 初始化 uC/OS-III */
    OSInit(&err);
    
    /* 创建启动任务 */
    OSTaskCreate(&AppTaskStartTCB,
                 "App Task Start",
                 AppTaskStart,
                 0,
                 APP_CFG_TASK_START_PRIO,
                 &AppTaskStartStk[0],
                 APP_CFG_TASK_START_STK_SIZE / 10,
                 APP_CFG_TASK_START_STK_SIZE,
                 0, 0, 0,
                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 &err);
    
    /* 启动多任务 */
    OSStart(&err);
    
    return 0;
}

static void AppTaskStart(void *p_arg)
{
    OS_ERR err;
    
    (void)p_arg;
    
    /* BSP 初始化 */
    BSP_Init();
    
    /* 创建应用任务 */
    OSTaskCreate(&AppTask1TCB,
                 "App Task 1",
                 AppTask1,
                 0,
                 APP_CFG_TASK1_PRIO,
                 &AppTask1Stk[0],
                 256 / 10,
                 256,
                 0, 0, 0,
                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 &err);
    
    OSTaskCreate(&AppTask2TCB,
                 "App Task 2",
                 AppTask2,
                 0,
                 APP_CFG_TASK2_PRIO,
                 &AppTask2Stk[0],
                 256 / 10,
                 256,
                 0, 0, 0,
                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 &err);
    
    while (1) {
        OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}
```

------

## 九、Bootloader + App 双区配置

### 1. GCC Bootloader 链接脚本

```ld
/* bootloader.ld - Bootloader 链接脚本 */
ENTRY(Reset_Handler)
```





```
继续
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

## 九、Bootloader + App 双区配置（续）

### 1. GCC Bootloader 链接脚本

```ld
/* bootloader.ld - Bootloader 链接脚本 */
ENTRY(Reset_Handler)

_Min_Stack_Size = 0x400;

MEMORY
{
    /* Bootloader 区域: 0x08000000 - 0x08007FFF (32KB) */
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 32K
    RAM (xrw)   : ORIGIN = 0x20000000, LENGTH = 128K
}

/* App 起始地址 - 供 Bootloader 跳转使用 */
__app_start_address = 0x08008000;

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    ._user_heap_stack :
    {
        . = ALIGN(8);
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > RAM

    _estack = ORIGIN(RAM) + LENGTH(RAM);
}
```

### 2. GCC Application 链接脚本

```ld
/* application.ld - 应用程序链接脚本 */
ENTRY(Reset_Handler)

_Min_Stack_Size = 0x800;
_Min_Heap_Size  = 0x400;

MEMORY
{
    /* App 区域: 0x08008000 - 0x080FFFFF (992KB) */
    FLASH (rx)  : ORIGIN = 0x08008000, LENGTH = 992K
    RAM (xrw)   : ORIGIN = 0x20000000, LENGTH = 128K
    CCMRAM (rw) : ORIGIN = 0x10000000, LENGTH = 64K
}

SECTIONS
{
    /* 中断向量表 - 必须在 App Flash 起始位置 */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* App 版本信息 - 固定偏移便于 Bootloader 读取 */
    .app_header 0x08008200 :
    {
        KEEP(*(.app_header))
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        *(.RamFunc)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    .ccmram :
    {
        . = ALIGN(4);
        _sccmram = .;
        *(.ccmram)
        *(.ccmram*)
        . = ALIGN(4);
        _eccmram = .;
    } > CCMRAM AT > FLASH
    _siccmram = LOADADDR(.ccmram);

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > RAM

    _estack = ORIGIN(RAM) + LENGTH(RAM);
}
```

### 3. KEIL Bootloader 分散加载文件

```text
; bootloader.sct - Bootloader 分散加载文件
; Bootloader: 32KB

LR_IROM1 0x08000000 0x00008000 {
    ER_IROM1 0x08000000 0x00008000 {
        *.o (RESET, +First)
        *(InRoot$$Sections)
        .ANY (+RO)
    }
    
    RW_IRAM1 0x20000000 0x00020000 {
        .ANY (+RW +ZI)
    }
}
```

### 4. KEIL Application 分散加载文件

```text
; application.sct - 应用程序分散加载文件
; App 起始: 0x08008000, 大小: 992KB

LR_IROM1 0x08008000 0x000F8000 {
    
    ; 中断向量表
    ER_IROM1 0x08008000 0x000F8000 {
        *.o (RESET, +First)
        *(InRoot$$Sections)
        .ANY (+RO)
    }
    
    ; App 头信息 - 固定地址
    ER_APP_HEADER 0x08008200 FIXED 0x00000100 {
        *(.app_header)
        *(app_header)
    }
    
    RW_IRAM1 0x20000000 0x00020000 {
        .ANY (+RW +ZI)
    }
    
    RW_CCMRAM 0x10000000 0x00010000 {
        *(.ccmram)
    }
}
```

### 5. IAR Bootloader ICF文件

```icf
/* bootloader.icf */
define symbol __ICFEDIT_intvec_start__ = 0x08000000;
define symbol __ICFEDIT_region_ROM_start__ = 0x08000000;
define symbol __ICFEDIT_region_ROM_end__   = 0x08007FFF;  /* 32KB */
define symbol __ICFEDIT_region_RAM_start__ = 0x20000000;
define symbol __ICFEDIT_region_RAM_end__   = 0x2001FFFF;

define symbol __ICFEDIT_size_cstack__ = 0x400;
define symbol __ICFEDIT_size_heap__   = 0x200;

/* App 起始地址 */
define exported symbol __app_start = 0x08008000;

define memory mem with size = 4G;
define region ROM_region = mem:[from __ICFEDIT_region_ROM_start__ 
                                to __ICFEDIT_region_ROM_end__];
define region RAM_region = mem:[from __ICFEDIT_region_RAM_start__ 
                                to __ICFEDIT_region_RAM_end__];

define block CSTACK with alignment = 8, size = __ICFEDIT_size_cstack__ { };
define block HEAP   with alignment = 8, size = __ICFEDIT_size_heap__   { };

initialize by copy { readwrite };
do not initialize  { section .noinit };

place at address mem:__ICFEDIT_intvec_start__ { readonly section .intvec };
place in ROM_region { readonly };
place in RAM_region { readwrite, block CSTACK, block HEAP };
```

### 6. IAR Application ICF文件

```icf
/* application.icf */
define symbol __ICFEDIT_intvec_start__ = 0x08008000;
define symbol __ICFEDIT_region_ROM_start__ = 0x08008000;
define symbol __ICFEDIT_region_ROM_end__   = 0x080FFFFF;  /* 992KB */
define symbol __ICFEDIT_region_RAM_start__ = 0x20000000;
define symbol __ICFEDIT_region_RAM_end__   = 0x2001FFFF;
define symbol __ICFEDIT_region_CCMRAM_start__ = 0x10000000;
define symbol __ICFEDIT_region_CCMRAM_end__   = 0x1000FFFF;

define symbol __ICFEDIT_size_cstack__ = 0x800;
define symbol __ICFEDIT_size_heap__   = 0x400;

/* App 头信息地址 */
define symbol __app_header_start = 0x08008200;

define memory mem with size = 4G;
define region ROM_region    = mem:[from __ICFEDIT_region_ROM_start__ 
                                   to __ICFEDIT_region_ROM_end__];
define region RAM_region    = mem:[from __ICFEDIT_region_RAM_start__ 
                                   to __ICFEDIT_region_RAM_end__];
define region CCMRAM_region = mem:[from __ICFEDIT_region_CCMRAM_start__ 
                                   to __ICFEDIT_region_CCMRAM_end__];

define block CSTACK with alignment = 8, size = __ICFEDIT_size_cstack__ { };
define block HEAP   with alignment = 8, size = __ICFEDIT_size_heap__   { };

initialize by copy { readwrite };
initialize by copy { section .ccmram };
do not initialize  { section .noinit };

place at address mem:__ICFEDIT_intvec_start__ { readonly section .intvec };
place at address mem:__app_header_start { readonly section .app_header };
place in ROM_region    { readonly };
place in RAM_region    { readwrite, block CSTACK, block HEAP };
place in CCMRAM_region { section .ccmram };
```

### 7. Bootloader 跳转代码

```c
/* bootloader 跳转到 App */
#include "stm32f4xx.h"

#define APP_ADDRESS     0x08008000U
#define APP_HEADER_ADDR 0x08008200U

/* App 头信息结构 */
typedef struct {
    uint32_t magic;          /* 魔数: 0xDEADBEEF */
    uint32_t version;        /* 版本号 */
    uint32_t size;           /* App 大小 */
    uint32_t crc32;          /* CRC32 校验 */
    char     build_date[16]; /* 编译日期 */
    char     build_time[16]; /* 编译时间 */
} app_header_t;

typedef void (*pFunction)(void);

/* 检查 App 是否有效 */
uint8_t is_app_valid(void)
{
    uint32_t app_stack_ptr = *(volatile uint32_t *)APP_ADDRESS;
    app_header_t *header = (app_header_t *)APP_HEADER_ADDR;
    
    /* 检查栈指针是否在 RAM 范围内 */
    if ((app_stack_ptr < 0x20000000) || (app_stack_ptr > 0x20020000)) {
        return 0;
    }
    
    /* 检查魔数 */
    if (header->magic != 0xDEADBEEF) {
        return 0;
    }
    
    /* 可选: CRC 校验 */
    // if (!verify_crc32(...)) return 0;
    
    return 1;
}

/* 跳转到 App */
void jump_to_app(void)
{
    uint32_t app_stack_ptr;
    uint32_t app_reset_handler;
    pFunction JumpToApplication;
    
    /* 获取 App 的栈指针和复位向量 */
    app_stack_ptr = *(volatile uint32_t *)APP_ADDRESS;
    app_reset_handler = *(volatile uint32_t *)(APP_ADDRESS + 4);
    
    /* 关闭所有中断 */
    __disable_irq();
    
    /* 关闭 SysTick */
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;
    
    /* 清除所有挂起的中断 */
    for (int i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }
    
    /* 重新映射向量表到 App 地址 */
    SCB->VTOR = APP_ADDRESS;
    
    /* 设置主栈指针 */
    __set_MSP(app_stack_ptr);
    
    /* 跳转到 App 的 Reset_Handler */
    JumpToApplication = (pFunction)app_reset_handler;
    
    /* 开启中断 */
    __enable_irq();
    
    /* 跳转 */
    JumpToApplication();
    
    /* 永远不会执行到这里 */
    while (1);
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    
    /* 检查是否需要进入升级模式 */
    if (check_upgrade_request()) {
        /* 进入固件升级流程 */
        firmware_upgrade_process();
    }
    
    /* 检查 App 是否有效 */
    if (is_app_valid()) {
        /* 跳转到 App */
        jump_to_app();
    }
    
    /* App 无效，进入升级模式 */
    firmware_upgrade_process();
    
    while (1);
}
```

### 8. Application 头信息定义

```c
/* app_header.c */
#include <stdint.h>

/* App 头信息 - 放在固定地址 */
__attribute__((section(".app_header"), used))
const struct {
    uint32_t magic;
    uint32_t version;
    uint32_t size;
    uint32_t crc32;
    char     build_date[16];
    char     build_time[16];
} app_header = {
    .magic      = 0xDEADBEEF,
    .version    = 0x00010000,  /* v1.0.0 */
    .size       = 0,           /* 由后处理脚本填充 */
    .crc32      = 0,           /* 由后处理脚本填充 */
    .build_date = __DATE__,
    .build_time = __TIME__,
};

/* App 启动时重定位向量表 */
void app_vector_table_relocate(void)
{
    /* 确保向量表指向 App 地址 */
    SCB->VTOR = 0x08008000;
}
```

------

## 十、OTA 双Bank升级配置

### 1. 双Bank Flash布局

```text
┌─────────────────────────────────────────────────────────────┐
│                    STM32 双Bank OTA 布局                     │
├─────────────────────────────────────────────────────────────┤
│ Bootloader  │ 0x08000000 │ 32KB  │ 固定不变               │
│ App Bank A  │ 0x08008000 │ 480KB │ 主应用区               │
│ App Bank B  │ 0x08080000 │ 480KB │ 备份/升级区            │
│ Config      │ 0x080F8000 │ 16KB  │ 配置参数区             │
│ OTA Info    │ 0x080FC000 │ 16KB  │ OTA 状态信息           │
└─────────────────────────────────────────────────────────────┘
```

### 2. GCC 双Bank链接脚本

```ld
/* app_bank_a.ld - Bank A 应用链接脚本 */
ENTRY(Reset_Handler)

_Min_Stack_Size = 0x800;

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08008000, LENGTH = 480K
    RAM (xrw)   : ORIGIN = 0x20000000, LENGTH = 128K
    CCMRAM (rw) : ORIGIN = 0x10000000, LENGTH = 64K
}

/* 导出 Bank 信息 */
__bank_start = ORIGIN(FLASH);
__bank_size  = LENGTH(FLASH);
__bank_id    = 0;  /* Bank A = 0, Bank B = 1 */

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .app_header 0x08008200 :
    {
        KEEP(*(.app_header))
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        *(.RamFunc)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > RAM

    _estack = ORIGIN(RAM) + LENGTH(RAM);
}
/* app_bank_b.ld - Bank B 应用链接脚本 */
/* 只需修改 FLASH 起始地址和 bank_id */
ENTRY(Reset_Handler)

_Min_Stack_Size = 0x800;

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08080000, LENGTH = 480K  /* Bank B 起始 */
    RAM (xrw)   : ORIGIN = 0x20000000, LENGTH = 128K
    CCMRAM (rw) : ORIGIN = 0x10000000, LENGTH = 64K
}

__bank_start = ORIGIN(FLASH);
__bank_size  = LENGTH(FLASH);
__bank_id    = 1;  /* Bank B */

/* 其余 SECTIONS 与 Bank A 相同 */
```

### 3. OTA 状态管理

```c
/* ota_info.h */
#ifndef __OTA_INFO_H
#define __OTA_INFO_H

#include <stdint.h>

#define OTA_INFO_ADDR       0x080FC000
#define OTA_MAGIC           0x4F544131  /* "OTA1" */

typedef enum {
    OTA_STATE_IDLE = 0,
    OTA_STATE_DOWNLOADING,
    OTA_STATE_DOWNLOADED,
    OTA_STATE_VERIFYING,
    OTA_STATE_VERIFIED,
    OTA_STATE_SWITCHING,
    OTA_STATE_ROLLBACK,
} ota_state_t;

typedef struct {
    uint32_t magic;
    uint32_t active_bank;      /* 当前活动 Bank: 0=A, 1=B */
    uint32_t update_bank;      /* 待更新 Bank */
    ota_state_t state;
    uint32_t new_version;
    uint32_t new_size;
    uint32_t new_crc32;
    uint32_t download_offset;
    uint32_t retry_count;
    uint32_t boot_count;       /* 启动计数，用于回滚判断 */
    uint32_t crc;              /* 结构体 CRC */
} ota_info_t;

#endif
/* ota_manager.c */
#include "ota_info.h"
#include "stm32f4xx_hal.h"

#define BANK_A_ADDR     0x08008000
#define BANK_B_ADDR     0x08080000
#define BANK_SIZE       (480 * 1024)

/* OTA 信息 - 放在专用 Flash 区域 */
static ota_info_t *ota_info = (ota_info_t *)OTA_INFO_ADDR;

/* 获取当前运行的 Bank */
uint32_t ota_get_current_bank(void)
{
    extern uint32_t __bank_id;
    return (uint32_t)&__bank_id;
}

/* 获取待更新的 Bank 地址 */
uint32_t ota_get_update_bank_addr(void)
{
    if (ota_info->active_bank == 0) {
        return BANK_B_ADDR;
    } else {
        return BANK_A_ADDR;
    }
}

/* 开始 OTA 升级 */
int ota_start_update(uint32_t new_version, uint32_t new_size, uint32_t new_crc)
{
    ota_info_t info;
    
    info.magic = OTA_MAGIC;
    info.active_bank = ota_get_current_bank();
    info.update_bank = (info.active_bank == 0) ? 1 : 0;
    info.state = OTA_STATE_DOWNLOADING;
    info.new_version = new_version;
    info.new_size = new_size;
    info.new_crc32 = new_crc;
    info.download_offset = 0;
    info.retry_count = 0;
    info.boot_count = 0;
    
    /* 擦除更新 Bank */
    if (flash_erase_bank(ota_get_update_bank_addr(), BANK_SIZE) != 0) {
        return -1;
    }
    
    /* 保存 OTA 信息 */
    return ota_save_info(&info);
}

/* 写入固件数据 */
int ota_write_data(uint32_t offset, uint8_t *data, uint32_t len)
{
    uint32_t addr = ota_get_update_bank_addr() + offset;
    
    if (flash_write(addr, data, len) != 0) {
        return -1;
    }
    
    /* 更新下载偏移 */
    ota_info_t info = *ota_info;
    info.download_offset = offset + len;
    ota_save_info(&info);
    
    return 0;
}

/* 完成下载，验证固件 */
int ota_finish_download(void)
{
    uint32_t addr = ota_get_update_bank_addr();
    uint32_t crc;
    
    /* 计算 CRC */
    crc = crc32_calculate((uint8_t *)addr, ota_info->new_size);
    
    if (crc != ota_info->new_crc32) {
        return -1;
    }
    
    /* 更新状态 */
    ota_info_t info = *ota_info;
    info.state = OTA_STATE_VERIFIED;
    ota_save_info(&info);
    
    return 0;
}

/* 切换到新固件 */
int ota_switch_bank(void)
{
    ota_info_t info = *ota_info;
    
    info.state = OTA_STATE_SWITCHING;
    info.active_bank = info.update_bank;
    info.boot_count = 0;
    ota_save_info(&info);
    
    /* 重启系统 */
    NVIC_SystemReset();
    
    return 0;
}

/* Bootloader 中调用 - 获取启动地址 */
uint32_t bootloader_get_app_addr(void)
{
    if (ota_info->magic != OTA_MAGIC) {
        return BANK_A_ADDR;  /* 默认 Bank A */
    }
    
    /* 检查是否需要回滚 */
    if (ota_info->boot_count > 3) {
        /* 启动失败超过3次，回滚 */
        ota_info_t info = *ota_info;
        info.active_bank = (info.active_bank == 0) ? 1 : 0;
        info.boot_count = 0;
        info.state = OTA_STATE_ROLLBACK;
        ota_save_info(&info);
    }
    
    /* 增加启动计数 */
    ota_info_t info = *ota_info;
    info.boot_count++;
    ota_save_info(&info);
    
    if (ota_info->active_bank == 0) {
        return BANK_A_ADDR;
    } else {
        return BANK_B_ADDR;
    }
}

/* App 启动成功后调用 - 确认启动 */
void ota_confirm_boot(void)
{
    if (ota_info->magic == OTA_MAGIC && ota_info->boot_count > 0) {
        ota_info_t info = *ota_info;
        info.boot_count = 0;
        info.state = OTA_STATE_IDLE;
        ota_save_info(&info);
    }
}
```

------

## 十一、MPU 内存保护配置

### STM32F7/H7 MPU 配置示例

```c
/* mpu_config.c - 配合链接脚本使用 */
#include "stm32h7xx.h"

/* 从链接脚本获取内存边界 */
extern uint32_t _stext, _etext;
extern uint32_t _sdata, _edata;
extern uint32_t _sbss, _ebss;
extern uint32_t _sstack, _estack;

void MPU_Config(void)
{
    MPU_Region_InitTypeDef MPU_InitStruct = {0};
    
    /* 禁用 MPU */
    HAL_MPU_Disable();
    
    /* Region 0: Flash - 只读可执行 */
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER0;
    MPU_InitStruct.BaseAddress = 0x08000000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_2MB;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStruct.AccessPermission = MPU_REGION_PRIV_RO_URO;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    
    /* Region 1: DTCM RAM - 读写不可执行 */
    MPU_InitStruct.Number = MPU_REGION_NUMBER1;
    MPU_InitStruct.BaseAddress = 0x20000000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_128KB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;  /* DTCM 不需要缓存 */
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    
    /* Region 2: AXI SRAM - 读写可缓存 */
    MPU_InitStruct.Number = MPU_REGION_NUMBER2;
    MPU_InitStruct.BaseAddress = 0x24000000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    
    /* Region 3: SRAM1/2 (D2域) - DMA 缓冲区，不可缓存 */
    MPU_InitStruct.Number = MPU_REGION_NUMBER3;
    MPU_InitStruct.BaseAddress = 0x30000000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_256KB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    
    /* Region 4: 外设区域 - Device 类型 */
    MPU_InitStruct.Number = MPU_REGION_NUMBER4;
    MPU_InitStruct.BaseAddress = 0x40000000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_512MB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    
    /* 启用 MPU */
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}
```

------

## 十二、调试与问题排查

### 1. 查看内存映射

```bash
# GCC - 查看符号表
arm-none-eabi-nm -S -n output.elf

# GCC - 查看段大小
arm-none-eabi-size output.elf

# GCC - 详细段信息
arm-none-eabi-objdump -h output.elf

# 生成详细 map 文件
arm-none-eabi-gcc ... -Wl,-Map=output.map
```

### 2. Map 文件分析

```text
# GCC map 文件关键信息
Memory Configuration
Name             Origin             Length
FLASH            0x08000000         0x00100000
RAM              0x20000000         0x00020000

Linker script and memory map
.text           0x08000000    0x1234
                0x08000000       0x1bc startup_stm32f407xx.o
                0x080001bc       0x48  system_stm32f4xx.o
                ...

.data           0x20000000      0x100 load address 0x08001234
                0x20000000       0x10  main.o
                ...

.bss            0x20000100      0x200
                0x20000100       0x80  main.o
                ...
```

### 3. 常见问题及解决

|            问题             |          原因           |               解决方案                |
| :-------------------------: | :---------------------: | :-----------------------------------: |
| `region 'FLASH' overflowed` |    代码超出Flash大小    |        优化代码大小，使用 -Os         |
|  `region 'RAM' overflowed`  |     数据超出RAM大小     |      减少全局变量，使用动态分配       |
|   Hard Fault 在跳转App后    |     向量表未重定位      |            设置 SCB->VTOR             |
|      DMA 传输失败 (H7)      |   缓冲区在错误内存域    |        将DMA缓冲区放在D2域SRAM        |
|      Cache 一致性问题       |   DMA与CPU缓存不同步    | 使用 SCB_CleanDCache/InvalidateDCache |
|         变量值异常          | .data/.bss 未正确初始化 |        检查启动代码和链接脚本         |
|           栈溢出            |       栈空间不足        |         增加 _Min_Stack_Size          |

### 4. 内存使用统计脚本

```python
#!/usr/bin/env python3
# memory_usage.py - 分析 map 文件

import re
import sys

def parse_map_file(filename):
    with open(filename, 'r') as f:
        content = f.read()
    
    # 解析内存区域
    regions = {}
    region_pattern = r'(\w+)\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)'
    for match in re.finditer(region_pattern, content):
        name, origin, length = match.groups()
        regions[name] = {
            'origin': int(origin, 16),
            'length': int(length, 16),
            'used': 0
        }
    
    # 解析段使用
    section_pattern = r'\.(\w+)\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)'
    for match in re.finditer(section_pattern, content):
        name, addr, size = match.groups()
        addr = int(addr, 16)
        size = int(size, 16)
        
        for region_name, region in regions.items():
            if region['origin'] <= addr < region['origin'] + region['length']:
                region['used'] += size
                break
    
    # 打印结果
    print("\n=== Memory Usage ===")
    print(f"{'Region':<10} {'Used':>10} {'Total':>10} {'Usage':>8}")
    print("-" * 40)
    for name, region in regions.items():
        if region['length'] > 0:
            usage = region['used'] / region['length'] * 100
            print(f"{name:<10} {region['used']:>10} {region['length']:>10} {usage:>7.1f}%")

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python memory_usage.py <map_file>")
        sys.exit(1)
    parse_map_file(sys.argv[1])
```

------

## 十三、最佳实践总结

### 链接脚本设计原则

|       原则       |                说明                |
| :--------------: | :--------------------------------: |
| **明确内存边界** |  精确定义每个内存区域的起始和大小  |
|  **合理分配段**  |  根据访问频率和特性分配到合适内存  |
| **预留扩展空间** |       为未来功能预留内存空间       |
|   **对齐要求**   | 遵守 ARM 架构的对齐要求 (4/8字节)  |
|  **DMA 缓冲区**  | 放在非缓存区域或正确处理缓存一致性 |
|    **栈保护**    |    合理设置栈大小，考虑中断嵌套    |

### RTOS 内存配置建议

|     RTOS      |     堆位置建议      |     任务栈建议     |
| :-----------: | :-----------------: | :----------------: |
| **FreeRTOS**  | CCM/DTCM (快速访问) | 静态分配到指定内存 |
| **RT-Thread** |  主RAM，可扩展多堆  | memheap 管理多区域 |
|  **ThreadX**  |   字节池/块池分离   |  从字节池动态分配  |
| **μC/OS-III** |        主RAM        |   静态数组指定段   |

------

如需针对特定芯片型号或应用场景的更详细配置，请告诉我具体需求！





```
针对于多个bank的flash或者多块ram要如何避免跨区域访问还有 icache，dcache 的访问
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

# STM32 多Bank Flash/RAM管理与Cache优化详解

------

## 一、多Bank Flash架构与管理

### 1. STM32 Flash Bank架构

```text
┌─────────────────────────────────────────────────────────────────────┐
│                    STM32H743 双Bank Flash架构                        │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  Bank 1 (1MB)                    Bank 2 (1MB)                       │
│  ┌─────────────────────┐        ┌─────────────────────┐            │
│  │ 0x08000000          │        │ 0x08100000          │            │
│  │                     │        │                     │            │
│  │ Sector 0-7         │        │ Sector 0-7         │            │
│  │ (每个128KB)         │        │ (每个128KB)         │            │
│  │                     │        │                     │            │
│  │ 0x080FFFFF          │        │ 0x081FFFFF          │            │
│  └─────────────────────┘        └─────────────────────┘            │
│                                                                     │
│  特性:                                                              │
│  - 支持 Read-While-Write (RWW): Bank1读取时可写Bank2               │
│  - 独立擦除: 可单独擦除某个Bank                                     │
│  - 双Boot: 可配置从Bank1或Bank2启动                                 │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                    STM32F4 单Bank Flash架构                          │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌─────────────────────────────────────────────────────┐           │
│  │ Sector 0   │ 16KB  │ 0x08000000 - 0x08003FFF       │           │
│  │ Sector 1   │ 16KB  │ 0x08004000 - 0x08007FFF       │           │
│  │ Sector 2   │ 16KB  │ 0x08008000 - 0x0800BFFF       │           │
│  │ Sector 3   │ 16KB  │ 0x0800C000 - 0x0800FFFF       │           │
│  │ Sector 4   │ 64KB  │ 0x08010000 - 0x0801FFFF       │           │
│  │ Sector 5-11│ 128KB │ 0x08020000 - 0x080FFFFF       │           │
│  └─────────────────────────────────────────────────────┘           │
│                                                                     │
│  注意: 单Bank不支持RWW，擦写时CPU会stall                            │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### 2. 防止Flash跨Bank访问的链接脚本

```ld
/* stm32h743_dual_bank.ld - 双Bank Flash链接脚本 */
ENTRY(Reset_Handler)

_Min_Stack_Size = 0x1000;
_Min_Heap_Size  = 0x1000;

MEMORY
{
    /* Flash Bank 1 - 主程序 */
    FLASH_BANK1 (rx)    : ORIGIN = 0x08000000, LENGTH = 1024K
    
    /* Flash Bank 2 - OTA/数据存储 */
    FLASH_BANK2 (rx)    : ORIGIN = 0x08100000, LENGTH = 896K
    
    /* Flash Bank 2 末尾 - 配置参数 */
    FLASH_CONFIG (r)    : ORIGIN = 0x081E0000, LENGTH = 128K
    
    /* RAM 区域 */
    DTCMRAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 128K
    AXIRAM (xrw)        : ORIGIN = 0x24000000, LENGTH = 512K
    SRAM1 (xrw)         : ORIGIN = 0x30000000, LENGTH = 128K
    SRAM2 (xrw)         : ORIGIN = 0x30020000, LENGTH = 128K
    SRAM3 (xrw)         : ORIGIN = 0x30040000, LENGTH = 32K
    SRAM4 (xrw)         : ORIGIN = 0x38000000, LENGTH = 64K
    BKPSRAM (rw)        : ORIGIN = 0x38800000, LENGTH = 4K
}

/* Bank 边界符号 - 供运行时检查 */
__flash_bank1_start = ORIGIN(FLASH_BANK1);
__flash_bank1_end   = ORIGIN(FLASH_BANK1) + LENGTH(FLASH_BANK1);
__flash_bank2_start = ORIGIN(FLASH_BANK2);
__flash_bank2_end   = ORIGIN(FLASH_BANK2) + LENGTH(FLASH_BANK2);

SECTIONS
{
    /* ============ Bank 1: 主程序代码 ============ */
    
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH_BANK1

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        
        KEEP(*(.init))
        KEEP(*(.fini))
        
        . = ALIGN(4);
        _etext = .;
    } > FLASH_BANK1

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH_BANK1

    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH_BANK1
    
    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH_BANK1

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH_BANK1

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH_BANK1

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH_BANK1

    /* ============ Bank 2: OTA 固件存储 ============ */
    
    .ota_firmware (NOLOAD) :
    {
        . = ALIGN(4);
        __ota_firmware_start = .;
        . = . + 896K;  /* 预留OTA空间 */
        __ota_firmware_end = .;
    } > FLASH_BANK2

    /* ============ 配置参数区 ============ */
    
    .config_data :
    {
        . = ALIGN(4);
        __config_start = .;
        KEEP(*(.config_data))
        KEEP(*(.config_data*))
        . = ALIGN(4);
        __config_end = .;
    } > FLASH_CONFIG

    /* ============ RAM 数据段 ============ */
    
    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        *(.RamFunc)
        *(.RamFunc*)
        . = ALIGN(4);
        _edata = .;
    } > DTCMRAM AT > FLASH_BANK1

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > DTCMRAM

    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > DTCMRAM

    _estack = ORIGIN(DTCMRAM) + LENGTH(DTCMRAM);

    /* ============ 断言检查 - 防止跨Bank ============ */
    
    /* 确保代码不超出 Bank1 */
    ASSERT(_etext <= __flash_bank1_end, 
           "Error: Code exceeds Flash Bank1 boundary!")
    
    /* 确保 .data 加载地址在 Bank1 内 */
    ASSERT(LOADADDR(.data) + SIZEOF(.data) <= __flash_bank1_end,
           "Error: .data LMA exceeds Flash Bank1 boundary!")
}
```

### 3. 运行时Flash Bank边界检查

```c
/* flash_bank_check.h */
#ifndef __FLASH_BANK_CHECK_H
#define __FLASH_BANK_CHECK_H

#include <stdint.h>
#include <stdbool.h>

/* Flash Bank 定义 */
#define FLASH_BANK1_START   0x08000000UL
#define FLASH_BANK1_END     0x080FFFFFUL
#define FLASH_BANK2_START   0x08100000UL
#define FLASH_BANK2_END     0x081FFFFFUL

#define FLASH_SECTOR_SIZE_128K  (128 * 1024)

typedef enum {
    FLASH_BANK_1 = 1,
    FLASH_BANK_2 = 2,
    FLASH_BANK_INVALID = 0
} flash_bank_t;

/* 获取地址所在Bank */
static inline flash_bank_t flash_get_bank(uint32_t addr)
{
    if (addr >= FLASH_BANK1_START && addr <= FLASH_BANK1_END) {
        return FLASH_BANK_1;
    } else if (addr >= FLASH_BANK2_START && addr <= FLASH_BANK2_END) {
        return FLASH_BANK_2;
    }
    return FLASH_BANK_INVALID;
}

/* 检查地址范围是否跨Bank */
static inline bool flash_check_cross_bank(uint32_t start_addr, uint32_t size)
{
    uint32_t end_addr = start_addr + size - 1;
    flash_bank_t start_bank = flash_get_bank(start_addr);
    flash_bank_t end_bank = flash_get_bank(end_addr);
    
    if (start_bank == FLASH_BANK_INVALID || end_bank == FLASH_BANK_INVALID) {
        return true;  /* 无效地址 */
    }
    
    return (start_bank != end_bank);  /* 跨Bank返回true */
}

/* 获取Bank内剩余空间 */
static inline uint32_t flash_get_remaining_in_bank(uint32_t addr)
{
    flash_bank_t bank = flash_get_bank(addr);
    
    if (bank == FLASH_BANK_1) {
        return FLASH_BANK1_END - addr + 1;
    } else if (bank == FLASH_BANK_2) {
        return FLASH_BANK2_END - addr + 1;
    }
    return 0;
}

#endif /* __FLASH_BANK_CHECK_H */
/* flash_operations.c - 安全的Flash操作 */
#include "flash_bank_check.h"
#include "stm32h7xx_hal.h"

/* 安全的Flash擦除 - 防止跨Bank */
HAL_StatusTypeDef safe_flash_erase(uint32_t start_addr, uint32_t size)
{
    FLASH_EraseInitTypeDef erase_init;
    uint32_t sector_error;
    HAL_StatusTypeDef status;
    
    /* 检查是否跨Bank */
    if (flash_check_cross_bank(start_addr, size)) {
        /* 跨Bank需要分两次擦除 */
        uint32_t bank1_size = 0;
        uint32_t bank2_size = 0;
        uint32_t bank2_start = 0;
        
        flash_bank_t start_bank = flash_get_bank(start_addr);
        
        if (start_bank == FLASH_BANK_1) {
            bank1_size = FLASH_BANK1_END - start_addr + 1;
            bank2_start = FLASH_BANK2_START;
            bank2_size = size - bank1_size;
        } else {
            /* 起始在Bank2，不应该跨到Bank1 */
            return HAL_ERROR;
        }
        
        /* 擦除Bank1部分 */
        status = safe_flash_erase(start_addr, bank1_size);
        if (status != HAL_OK) return status;
        
        /* 擦除Bank2部分 */
        return safe_flash_erase(bank2_start, bank2_size);
    }
    
    /* 单Bank内擦除 */
    HAL_FLASH_Unlock();
    
    flash_bank_t bank = flash_get_bank(start_addr);
    uint32_t first_sector = (start_addr - (bank == FLASH_BANK_1 ? 
                            FLASH_BANK1_START : FLASH_BANK2_START)) / 
                            FLASH_SECTOR_SIZE_128K;
    uint32_t num_sectors = (size + FLASH_SECTOR_SIZE_128K - 1) / 
                           FLASH_SECTOR_SIZE_128K;
    
    erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_init.Banks = (bank == FLASH_BANK_1) ? FLASH_BANK_1 : FLASH_BANK_2;
    erase_init.Sector = first_sector;
    erase_init.NbSectors = num_sectors;
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    
    status = HAL_FLASHEx_Erase(&erase_init, &sector_error);
    
    HAL_FLASH_Lock();
    
    return status;
}

/* 安全的Flash写入 - 防止跨Bank */
HAL_StatusTypeDef safe_flash_write(uint32_t addr, uint8_t *data, uint32_t size)
{
    HAL_StatusTypeDef status;
    
    /* 检查是否跨Bank */
    if (flash_check_cross_bank(addr, size)) {
        uint32_t remaining = flash_get_remaining_in_bank(addr);
        
        /* 写入当前Bank */
        status = safe_flash_write(addr, data, remaining);
        if (status != HAL_OK) return status;
        
        /* 写入下一个Bank */
        flash_bank_t current_bank = flash_get_bank(addr);
        uint32_t next_addr = (current_bank == FLASH_BANK_1) ? 
                             FLASH_BANK2_START : FLASH_BANK1_START;
        
        return safe_flash_write(next_addr, data + remaining, size - remaining);
    }
    
    /* 单Bank内写入 */
    HAL_FLASH_Unlock();
    
    /* STM32H7 使用256位(32字节)Flash字写入 */
    uint32_t write_addr = addr;
    uint32_t remaining = size;
    uint8_t *src = data;
    
    /* 对齐到32字节边界 */
    uint32_t flash_word[8];  /* 256位 = 32字节 = 8个uint32_t */
    
    while (remaining > 0) {
        /* 准备32字节数据 */
        memset(flash_word, 0xFF, sizeof(flash_word));
        uint32_t copy_size = (remaining >= 32) ? 32 : remaining;
        memcpy(flash_word, src, copy_size);
        
        /* 写入Flash */
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, 
                                   write_addr, (uint32_t)flash_word);
        if (status != HAL_OK) {
            HAL_FLASH_Lock();
            return status;
        }
        
        write_addr += 32;
        src += copy_size;
        remaining -= copy_size;
    }
    
    HAL_FLASH_Lock();
    return HAL_OK;
}
```

------

## 二、多RAM区域管理

### 1. STM32H7 RAM架构详解

```text
┌─────────────────────────────────────────────────────────────────────┐
│                    STM32H7 多域RAM架构                               │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                         D1 域 (CPU)                          │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │   │
│  │  │   ITCM      │  │   DTCM      │  │     AXI SRAM        │  │   │
│  │  │   64KB      │  │   128KB     │  │     512KB           │  │   │
│  │  │ 0x00000000  │  │ 0x20000000  │  │   0x24000000        │  │   │
│  │  │             │  │             │  │                     │  │   │
│  │  │ 最快执行    │  │ 最快数据    │  │   大缓冲区          │  │   │
│  │  │ 无Cache     │  │ 无Cache     │  │   可Cache           │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────────────┘  │   │
│  └─────────────────────────────────────────────────────────────┘   │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                    D2 域 (DMA/外设)                          │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │   │
│  │  │   SRAM1     │  │   SRAM2     │  │     SRAM3           │  │   │
│  │  │   128KB     │  │   128KB     │  │     32KB            │  │   │
│  │  │ 0x30000000  │  │ 0x30020000  │  │   0x30040000        │  │   │
│  │  │             │  │             │  │                     │  │   │
│  │  │ DMA1/2      │  │ DMA1/2      │  │   BDMA              │  │   │
│  │  │ ETH/USB     │  │ ETH/USB     │  │                     │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────────────┘  │   │
│  └─────────────────────────────────────────────────────────────┘   │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                    D3 域 (低功耗)                            │   │
│  │  ┌─────────────────────┐  ┌─────────────────────────────┐   │   │
│  │  │       SRAM4         │  │      Backup SRAM            │   │   │
│  │  │       64KB          │  │        4KB                  │   │   │
│  │  │    0x38000000       │  │     0x38800000              │   │   │
│  │  │                     │  │                             │   │   │
│  │  │  BDMA可访问         │  │   电池保持                  │   │   │
│  │  │  低功耗模式保持     │  │   RTC备份                   │   │   │
│  │  └─────────────────────┘  └─────────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────────────┘   │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### 2. 完整的多RAM链接脚本

```ld
/* stm32h743_multi_ram.ld - 多RAM区域管理 */
ENTRY(Reset_Handler)

_Min_Stack_Size = 0x2000;  /* 8KB 主栈 */
_Min_Heap_Size  = 0x2000;  /* 8KB 堆 */

MEMORY
{
    /* Flash */
    FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 2048K
    
    /* D1 域 RAM - CPU直接访问最快 */
    ITCMRAM (xrw)   : ORIGIN = 0x00000000, LENGTH = 64K   /* 指令TCM */
    DTCMRAM (xrw)   : ORIGIN = 0x20000000, LENGTH = 128K  /* 数据TCM */
    AXIRAM (xrw)    : ORIGIN = 0x24000000, LENGTH = 512K  /* AXI SRAM */
    
    /* D2 域 RAM - DMA可访问 */
    SRAM1 (xrw)     : ORIGIN = 0x30000000, LENGTH = 128K
    SRAM2 (xrw)     : ORIGIN = 0x30020000, LENGTH = 128K
    SRAM3 (xrw)     : ORIGIN = 0x30040000, LENGTH = 32K
    
    /* D3 域 RAM - 低功耗可访问 */
    SRAM4 (xrw)     : ORIGIN = 0x38000000, LENGTH = 64K
    BKPSRAM (rw)    : ORIGIN = 0x38800000, LENGTH = 4K
}

/* RAM 区域边界符号 */
__dtcm_start = ORIGIN(DTCMRAM);
__dtcm_end   = ORIGIN(DTCMRAM) + LENGTH(DTCMRAM);
__axi_start  = ORIGIN(AXIRAM);
__axi_end    = ORIGIN(AXIRAM) + LENGTH(AXIRAM);
__sram1_start = ORIGIN(SRAM1);
__sram1_end   = ORIGIN(SRAM1) + LENGTH(SRAM1);
__sram2_start = ORIGIN(SRAM2);
__sram2_end   = ORIGIN(SRAM2) + LENGTH(SRAM2);
__sram4_start = ORIGIN(SRAM4);
__sram4_end   = ORIGIN(SRAM4) + LENGTH(SRAM4);

SECTIONS
{
    /* ============ Flash 代码段 ============ */
    
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    /* ============ ITCM - 关键代码 ============ */
    
    _siitcm = LOADADDR(.itcm);
    
    .itcm :
    {
        . = ALIGN(4);
        _sitcm = .;
        *(.itcm)
        *(.itcm*)
        /* 中断处理函数 */
        *(.itcm.isr)
        /* DSP 函数 */
        *libarm_cortexM7lfsp_math.a:(.text*)
        . = ALIGN(4);
        _eitcm = .;
    } > ITCMRAM AT > FLASH

    /* ============ DTCM - 主数据区 ============ */
    
    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        *(.RamFunc)
        *(.RamFunc*)
        . = ALIGN(4);
        _edata = .;
    } > DTCMRAM AT > FLASH

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > DTCMRAM

    /* DTCM 中的不初始化区域 */
    .dtcm_noinit (NOLOAD) :
    {
        . = ALIGN(4);
        *(.dtcm_noinit)
        *(.dtcm_noinit*)
        . = ALIGN(4);
    } > DTCMRAM

    /* 堆和栈 - 放在DTCM */
    ._user_heap_stack (NOLOAD) :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        __heap_start = .;
        . = . + _Min_Heap_Size;
        __heap_end = .;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > DTCMRAM

    _estack = ORIGIN(DTCMRAM) + LENGTH(DTCMRAM);

    /* ============ AXI SRAM - 大缓冲区 ============ */
    
    _siaxi = LOADADDR(.axiram);
    
    .axiram :
    {
        . = ALIGN(4);
        _saxi = .;
        *(.axiram)
        *(.axiram*)
        . = ALIGN(4);
        _eaxi = .;
    } > AXIRAM AT > FLASH

    /* AXI SRAM 不初始化区域 - 帧缓冲等 */
    .axi_noinit (NOLOAD) :
    {
        . = ALIGN(32);  /* Cache line 对齐 */
        __axi_noinit_start = .;
        *(.axi_noinit)
        *(.axi_noinit*)
        *(.framebuffer)
        . = ALIGN(32);
        __axi_noinit_end = .;
    } > AXIRAM

    /* ============ D2 SRAM1 - DMA 缓冲区 ============ */
    
    .sram1 (NOLOAD) :
    {
        . = ALIGN(32);  /* DMA 和 Cache line 对齐 */
        __sram1_bss_start = .;
        *(.sram1)
        *(.sram1*)
        *(.sram1_bss)
        *(.eth_buffers)     /* 以太网 DMA 缓冲区 */
        *(.usb_buffers)     /* USB DMA 缓冲区 */
        . = ALIGN(32);
        __sram1_bss_end = .;
    } > SRAM1

    /* ============ D2 SRAM2 - DMA 缓冲区 ============ */
    
    .sram2 (NOLOAD) :
    {
        . = ALIGN(32);
        __sram2_bss_start = .;
        *(.sram2)
        *(.sram2*)
        *(.sram2_bss)
        *(.dma_buffers)     /* 通用 DMA 缓冲区 */
        . = ALIGN(32);
        __sram2_bss_end = .;
    } > SRAM2

    /* ============ D2 SRAM3 - BDMA 缓冲区 ============ */
    
    .sram3 (NOLOAD) :
    {
        . = ALIGN(4);
        __sram3_bss_start = .;
        *(.sram3)
        *(.sram3*)
        *(.bdma_buffers)
        . = ALIGN(4);
        __sram3_bss_end = .;
    } > SRAM3

    /* ============ D3 SRAM4 - 低功耗保持 ============ */
    
    .sram4 (NOLOAD) :
    {
        . = ALIGN(4);
        __sram4_bss_start = .;
        *(.sram4)
        *(.sram4*)
        *(.lowpower_data)
        . = ALIGN(4);
        __sram4_bss_end = .;
    } > SRAM4

    /* ============ 备份 SRAM ============ */
    
    .bkpsram (NOLOAD) :
    {
        . = ALIGN(4);
        __bkpsram_start = .;
        *(.bkpsram)
        *(.bkpsram*)
        *(.backup_data)
        . = ALIGN(4);
        __bkpsram_end = .;
    } > BKPSRAM

    /* ============ 调试信息 ============ */
    
    .ARM.attributes 0 : { *(.ARM.attributes) }
    
    /DISCARD/ :
    {
        libc.a(*)
        libm.a(*)
        libgcc.a(*)
    }
}

/* ============ 断言检查 ============ */

/* 确保各段不超出对应内存区域 */
ASSERT(_eitcm <= ORIGIN(ITCMRAM) + LENGTH(ITCMRAM), 
       "ITCM overflow!")
ASSERT(_estack <= ORIGIN(DTCMRAM) + LENGTH(DTCMRAM), 
       "DTCM overflow!")
ASSERT(__axi_noinit_end <= ORIGIN(AXIRAM) + LENGTH(AXIRAM), 
       "AXI SRAM overflow!")
ASSERT(__sram1_bss_end <= ORIGIN(SRAM1) + LENGTH(SRAM1), 
       "SRAM1 overflow!")
ASSERT(__sram2_bss_end <= ORIGIN(SRAM2) + LENGTH(SRAM2), 
       "SRAM2 overflow!")
```

### 3. RAM区域边界检查

```c
/* ram_region_check.h */
#ifndef __RAM_REGION_CHECK_H
#define __RAM_REGION_CHECK_H

#include <stdint.h>
#include <stdbool.h>

/* RAM 区域定义 */
typedef enum {
    RAM_REGION_ITCM = 0,
    RAM_REGION_DTCM,
    RAM_REGION_AXI,
    RAM_REGION_SRAM1,
    RAM_REGION_SRAM2,
    RAM_REGION_SRAM3,
    RAM_REGION_SRAM4,
    RAM_REGION_BKPSRAM,
    RAM_REGION_INVALID
} ram_region_t;

/* RAM 区域信息 */
typedef struct {
    uint32_t start;
    uint32_t end;
    const char *name;
    bool cacheable;
    bool dma_accessible;  /* DMA1/DMA2 可访问 */
    bool bdma_accessible; /* BDMA 可访问 */
} ram_region_info_t;

/* RAM 区域表 */
static const ram_region_info_t ram_regions[] = {
    /* ITCM */
    {0x00000000, 0x0000FFFF, "ITCM",    false, false, false},
    /* DTCM */
    {0x20000000, 0x2001FFFF, "DTCM",    false, false, false},
    /* AXI SRAM */
    {0x24000000, 0x2407FFFF, "AXI",     true,  true,  false},
    /* SRAM1 */
    {0x30000000, 0x3001FFFF, "SRAM1",   true,  true,  false},
    /* SRAM2 */
    {0x30020000, 0x3003FFFF, "SRAM2",   true,  true,  false},
    /* SRAM3 */
    {0x30040000, 0x30047FFF, "SRAM3",   true,  true,  true},
    /* SRAM4 */
    {0x38000000, 0x3800FFFF, "SRAM4",   true,  false, true},
    /* Backup SRAM */
    {0x38800000, 0x38800FFF, "BKPSRAM", false, false, false},
};

#define RAM_REGION_COUNT (sizeof(ram_regions) / sizeof(ram_regions[0]))

/* 获取地址所在RAM区域 */
static inline ram_region_t ram_get_region(uint32_t addr)
{
    for (int i = 0; i < RAM_REGION_COUNT; i++) {
        if (addr >= ram_regions[i].start && addr <= ram_regions[i].end) {
            return (ram_region_t)i;
        }
    }
    return RAM_REGION_INVALID;
}

/* 检查地址范围是否跨RAM区域 */
static inline bool ram_check_cross_region(uint32_t start_addr, uint32_t size)
{
    if (size == 0) return false;
    
    uint32_t end_addr = start_addr + size - 1;
    ram_region_t start_region = ram_get_region(start_addr);
    ram_region_t end_region = ram_get_region(end_addr);
    
    if (start_region == RAM_REGION_INVALID || 
        end_region == RAM_REGION_INVALID) {
        return true;
    }
    
    return (start_region != end_region);
}

/* 检查地址是否可被DMA访问 */
static inline bool ram_is_dma_accessible(uint32_t addr)
{
    ram_region_t region = ram_get_region(addr);
    if (region == RAM_REGION_INVALID) return false;
    return ram_regions[region].dma_accessible;
}

/* 检查地址是否可被BDMA访问 */
static inline bool ram_is_bdma_accessible(uint32_t addr)
{
    ram_region_t region = ram_get_region(addr);
    if (region == RAM_REGION_INVALID) return false;
    return ram_regions[region].bdma_accessible;
}

/* 检查地址是否可缓存 */
static inline bool ram_is_cacheable(uint32_t addr)
{
    ram_region_t region = ram_get_region(addr);
    if (region == RAM_REGION_INVALID) return false;
    return ram_regions[region].cacheable;
}

/* 获取区域内剩余空间 */
static inline uint32_t ram_get_remaining(uint32_t addr)
{
    ram_region_t region = ram_get_region(addr);
    if (region == RAM_REGION_INVALID) return 0;
    return ram_regions[region].end - addr + 1;
}

/* 获取区域信息 */
static inline const ram_region_info_t* ram_get_region_info(ram_region_t region)
{
    if (region >= RAM_REGION_COUNT) return NULL;
    return &ram_regions[region];
}

#endif /* __RAM_REGION_CHECK_H */
```

### 4. 安全的内存分配器

```c
/* safe_malloc.c - 区域感知的内存分配 */
#include "ram_region_check.h"
#include <string.h>

/* 各区域的简单内存池 */
typedef struct {
    uint8_t *pool_start;
    uint8_t *pool_end;
    uint8_t *current;
    ram_region_t region;
} region_allocator_t;

/* 从链接脚本获取边界 */
extern uint32_t __axi_noinit_start, __axi_noinit_end;
extern uint32_t __sram1_bss_start, __sram1_bss_end;
extern uint32_t __sram2_bss_start, __sram2_bss_end;

/* 区域分配器实例 */
static region_allocator_t axi_allocator;
static region_allocator_t sram1_allocator;
static region_allocator_t sram2_allocator;

/* 初始化区域分配器 */
void region_allocator_init(void)
{
    /* AXI SRAM 分配器 */
    axi_allocator.pool_start = (uint8_t *)&__axi_noinit_start;
    axi_allocator.pool_end = (uint8_t *)&__axi_noinit_end;
    axi_allocator.current = axi_allocator.pool_start;
    axi_allocator.region = RAM_REGION_AXI;
    
    /* SRAM1 分配器 */
    sram1_allocator.pool_start = (uint8_t *)&__sram1_bss_start;
    sram1_allocator.pool_end = (uint8_t *)&__sram1_bss_end;
    sram1_allocator.current = sram1_allocator.pool_start;
    sram1_allocator.region = RAM_REGION_SRAM1;
    
    /* SRAM2 分配器 */
    sram2_allocator.pool_start = (uint8_t *)&__sram2_bss_start;
    sram2_allocator.pool_end = (uint8_t *)&__sram2_bss_end;
    sram2_allocator.current = sram2_allocator.pool_start;
    sram2_allocator.region = RAM_REGION_SRAM2;
}

/* 从指定区域分配内存 */
void *region_malloc(ram_region_t region, size_t size, size_t alignment)
{
    region_allocator_t *allocator = NULL;
    
    switch (region) {
        case RAM_REGION_AXI:
            allocator = &axi_allocator;
            break;
        case RAM_REGION_SRAM1:
            allocator = &sram1_allocator;
            break;
        case RAM_REGION_SRAM2:
            allocator = &sram2_allocator;
            break;
        default:
            return NULL;
    }
    
    /* 对齐 */
    uint32_t aligned_addr = ((uint32_t)allocator->current + alignment - 1) 
                            & ~(alignment - 1);
    uint8_t *result = (uint8_t *)aligned_addr;
    
    /* 检查是否有足够空间 */
    if (result + size > allocator->pool_end) {
        return NULL;
    }
    
    /* 检查是否会跨区域 */
    if (ram_check_cross_region((uint32_t)result, size)) {
        return NULL;
    }
    
    allocator->current = result + size;
    
    return result;
}

/* DMA 安全的内存分配 */
void *dma_malloc(size_t size)
{
    /* 优先从 SRAM1 分配，32字节对齐 */
    void *ptr = region_malloc(RAM_REGION_SRAM1, size, 32);
    if (ptr) return ptr;
    
    /* SRAM1 不足，尝试 SRAM2 */
    ptr = region_malloc(RAM_REGION_SRAM2, size, 32);
    if (ptr) return ptr;
    
    /* 最后尝试 AXI SRAM */
    return region_malloc(RAM_REGION_AXI, size, 32);
}

/* 帧缓冲分配 - 从 AXI SRAM */
void *framebuffer_malloc(size_t size)
{
    return region_malloc(RAM_REGION_AXI, size, 32);
}

/* 验证DMA缓冲区 */
bool validate_dma_buffer(void *buffer, size_t size)
{
    uint32_t addr = (uint32_t)buffer;
    
    /* 检查是否DMA可访问 */
    if (!ram_is_dma_accessible(addr)) {
        return false;
    }
    
    /* 检查是否跨区域 */
    if (ram_check_cross_region(addr, size)) {
        return false;
    }
    
    /* 检查对齐 (32字节对齐用于Cache) */
    if (addr & 0x1F) {
        return false;
    }
    
    return true;
}
```

------

## 三、Cache 管理详解

### 1. STM32H7 Cache 架构

```text
┌─────────────────────────────────────────────────────────────────────┐
│                    STM32H7 Cache 架构                                │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                      Cortex-M7 CPU                           │   │
│  │  ┌─────────────────┐      ┌─────────────────┐               │   │
│  │  │    I-Cache      │      │    D-Cache      │               │   │
│  │  │    16KB         │      │    16KB         │               │   │
│  │  │  4-way set      │      │  4-way set      │               │   │
│  │  │  32B line       │      │  32B line       │               │   │
│  │  └────────┬────────┘      └────────┬────────┘               │   │
│  │           │                        │                         │   │
│  └───────────┼────────────────────────┼─────────────────────────┘   │
│              │                        │                             │
│              ▼                        ▼                             │
│  ┌───────────────────────────────────────────────────────────────┐ │
│  │                         AXI Bus                                │ │
│  └───────────────────────────────────────────────────────────────┘ │
│              │                        │                             │
│              ▼                        ▼                             │
│  ┌─────────────────┐      ┌─────────────────────────────────────┐ │
│  │     Flash       │      │           RAM                        │ │
│  │   (Cacheable)   │      │  AXI SRAM: Cacheable                │ │
│  │                 │      │  SRAM1/2:  Cacheable (可配置)       │ │
│  │                 │      │  DTCM:     Not Cacheable (直连)     │ │
│  │                 │      │  ITCM:     Not Cacheable (直连)     │ │
│  └─────────────────┘      └─────────────────────────────────────┘ │
│                                                                     │
│  Cache Line = 32 bytes                                              │
│  Cache 操作必须以 32 字节对齐                                        │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### 2. Cache 一致性问题场景

```text
┌─────────────────────────────────────────────────────────────────────┐
│                    DMA 与 Cache 一致性问题                           │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  场景1: CPU写入 -> DMA读取 (TX)                                     │
│  ┌─────────┐                      ┌─────────┐                      │
│  │   CPU   │ ──写入──> D-Cache    │   DMA   │                      │
│  └─────────┘           │          └────┬────┘                      │
│                        │               │                            │
│                        ▼               │                            │
│                   ┌─────────┐          │                            │
│                   │   RAM   │ <────────┘ 读取旧数据!                │
│                   └─────────┘                                       │
│                                                                     │
│  解决: CPU写入后执行 SCB_CleanDCache_by_Addr()                      │
│                                                                     │
│  ─────────────────────────────────────────────────────────────────  │
│                                                                     │
│  场景2: DMA写入 -> CPU读取 (RX)                                     │
│  ┌─────────┐                      ┌─────────┐                      │
│  │   CPU   │ <──读取── D-Cache    │   DMA   │                      │
│  └─────────┘           │          └────┬────┘                      │
│                        │               │                            │
│                   ┌─────────┐          │                            │
│                   │   RAM   │ <────────┘ 写入新数据                 │
│                   └─────────┘                                       │
│                        │                                            │
│                   Cache中是旧数据!                                   │
│                                                                     │
│  解决: DMA完成后执行 SCB_InvalidateDCache_by_Addr()                 │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### 3. 完整的Cache管理代码

```c
/* cache_manager.h */
#ifndef __CACHE_MANAGER_H
#define __CACHE_MANAGER_H

#include "stm32h7xx.h"
#include "ram_region_check.h"
#include <stdint.h>
#include <stdbool.h>

/* Cache line 大小 */
#define CACHE_LINE_SIZE     32

/* 对齐宏 */
#define CACHE_ALIGN         __attribute__((aligned(CACHE_LINE_SIZE)))
#define CACHE_ALIGN_UP(x)   (((x) + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1))
#define CACHE_ALIGN_DOWN(x) ((x) & ~(CACHE_LINE_SIZE - 1))

/* DMA 缓冲区声明宏 */
#define DMA_BUFFER_SECTION  __attribute__((section(".sram1")))
#define DMA_BUFFER          DMA_BUFFER_SECTION CACHE_ALIGN

/* 非缓存区域声明宏 */
#define NONCACHEABLE        __attribute__((section(".sram1")))

/*
 * Cache 初始化
 */
void cache_init(void);

/*
 * 启用/禁用 Cache
 */
void cache_enable(void);
void cache_disable(void);

/*
 * D-Cache 操作
 */
void cache_clean_all(void);
void cache_invalidate_all(void);
void cache_clean_invalidate_all(void);

/*
 * 按地址操作 D-Cache
 */
void cache_clean_by_addr(void *addr, uint32_t size);
void cache_invalidate_by_addr(void *addr, uint32_t size);
void cache_clean_invalidate_by_addr(void *addr, uint32_t size);

/*
 * DMA 传输前后的 Cache 处理
 */
void cache_prepare_dma_tx(void *buffer, uint32_t size);
void cache_complete_dma_rx(void *buffer, uint32_t size);

/*
 * 检查地址是否需要 Cache 操作
 */
bool cache_needs_maintenance(uint32_t addr);

#endif /* __CACHE_MANAGER_H */
/* cache_manager.c */
#include "cache_manager.h"
#include <string.h>

/*
 * Cache 初始化
 */
void cache_init(void)
{
    /* 启用 I-Cache */
    SCB_EnableICache();
    
    /* 启用 D-Cache */
    SCB_EnableDCache();
}

/*
 * 启用 Cache
 */
void cache_enable(void)
{
    SCB_EnableICache();
    SCB_EnableDCache();
}

/*
 * 禁用 Cache
 */
void cache_disable(void)
{
    SCB_DisableICache();
    SCB_DisableDCache();
}

/*
 * 清除整个 D-Cache (写回到内存)
 */
void cache_clean_all(void)
{
    SCB_CleanDCache();
}

/*
 * 无效化整个 D-Cache
 */
void cache_invalidate_all(void)
{
    SCB_InvalidateDCache();
}

/*
 * 清除并无效化整个 D-Cache
 */
void cache_clean_invalidate_all(void)
{
    SCB_CleanInvalidateDCache();
}

/*
 * 检查地址是否需要 Cache 维护
 */
bool cache_needs_maintenance(uint32_t addr)
{
    /* DTCM 和 ITCM 不经过 Cache */
    if (addr >= 0x20000000 && addr < 0x20020000) {
        return false;  /* DTCM */
    }
    if (addr < 0x00010000) {
        return false;  /* ITCM */
    }
    
    /* 检查 MPU 配置的非缓存区域 */
    /* 这里假设 SRAM1/2 配置为非缓存 */
    if (addr >= 0x30000000 && addr < 0x30048000) {
        return false;  /* D2 SRAM (如果配置为非缓存) */
    }
    
    return true;
}

/*
 * 按地址清除 D-Cache
 * 将 Cache 中的数据写回到内存
 */
void cache_clean_by_addr(void *addr, uint32_t size)
{
    uint32_t start_addr = (uint32_t)addr;
    
    /* 检查是否需要 Cache 操作 */
    if (!cache_needs_maintenance(start_addr)) {
        return;
    }
    
    /* 对齐到 Cache line */
    uint32_t aligned_addr = CACHE_ALIGN_DOWN(start_addr);
    uint32_t aligned_size = CACHE_ALIGN_UP(start_addr + size - aligned_addr);
    
    SCB_CleanDCache_by_Addr((uint32_t *)aligned_addr, aligned_size);
}

/*
 * 按地址无效化 D-Cache
 * 丢弃 Cache 中的数据，下次读取从内存获取
 */
void cache_invalidate_by_addr(void *addr, uint32_t size)
{
    uint32_t start_addr = (uint32_t)addr;
    
    if (!cache_needs_maintenance(start_addr)) {
        return;
    }
    
    uint32_t aligned_addr = CACHE_ALIGN_DOWN(start_addr);
    uint32_t aligned_size = CACHE_ALIGN_UP(start_addr + size - aligned_addr);
    
    SCB_InvalidateDCache_by_Addr((uint32_t *)aligned_addr, aligned_size);
}

/*
 * 按地址清除并无效化 D-Cache
 */
void cache_clean_invalidate_by_addr(void *addr, uint32_t size)
{
    uint32_t start_addr = (uint32_t)addr;
    
    if (!cache_needs_maintenance(start_addr)) {
        return;
    }
    
    uint32_t aligned_addr = CACHE_ALIGN_DOWN(start_addr);
    uint32_t aligned_size = CACHE_ALIGN_UP(start_addr + size - aligned_addr);
    
    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)aligned_addr, aligned_size);
}

/*
 * DMA 发送前准备
 * CPU 写入数据后，DMA 读取前调用
 */
void cache_prepare_dma_tx(void *buffer, uint32_t size)
{
    /* 将 Cache 中的数据写回内存，确保 DMA 读取到最新数据 */
    cache_clean_by_addr(buffer, size);
    
    /* 数据同步屏障 */
    __DSB();
}

/*
 * DMA 接收完成后处理
 * DMA 写入数据后，CPU 读取前调用
 */
void cache_complete_dma_rx(void *buffer, uint32_t size)
{
    /* 数据同步屏障 */
    __DSB();
    
    /* 无效化 Cache，确保 CPU 从内存读取最新数据 */
    cache_invalidate_by_addr(buffer, size);
}
```

### 4. DMA 安全的缓冲区管理

```c
/* dma_buffer.h */
#ifndef __DMA_BUFFER_H
#define __DMA_BUFFER_H

#include "cache_manager.h"
#include <stdint.h>
#include <stdbool.h>

/* DMA 缓冲区结构 */
typedef struct {
    uint8_t *data;
    uint32_t size;
    uint32_t capacity;
    bool is_cacheable;
} dma_buffer_t;

/* 创建 DMA 缓冲区 */
bool dma_buffer_create(dma_buffer_t *buf, uint32_t capacity);

/* 销毁 DMA 缓冲区 */
void dma_buffer_destroy(dma_buffer_t *buf);

/* 准备 DMA 发送 */
void dma_buffer_prepare_tx(dma_buffer_t *buf);

/* DMA 接收完成 */
void dma_buffer_complete_rx(dma_buffer_t *buf, uint32_t received_size);

/* 写入数据到缓冲区 */
bool dma_buffer_write(dma_buffer_t *buf, const void *data, uint32_t size);

/* 从缓冲区读取数据 */
bool dma_buffer_read(dma_buffer_t *buf, void *data, uint32_t size);

#endif /* __DMA_BUFFER_H */
/* dma_buffer.c */
#include "dma_buffer.h"
#include "ram_region_check.h"
#include <string.h>
#include <stdlib.h>

/* 静态 DMA 缓冲区池 - 放在 SRAM1 */
#define DMA_POOL_SIZE   (64 * 1024)

__attribute__((section(".sram1"), aligned(32)))
static uint8_t dma_buffer_pool[DMA_POOL_SIZE];
static uint32_t dma_pool_offset = 0;

/*
 * 从 DMA 安全区域分配内存
 */
static void *dma_alloc(uint32_t size)
{
    /* 32字节对齐 */
    uint32_t aligned_offset = CACHE_ALIGN_UP(dma_pool_offset);
    uint32_t aligned_size = CACHE_ALIGN_UP(size);
    
    if (aligned_offset + aligned_size > DMA_POOL_SIZE) {
        return NULL;
    }
    
    void *ptr = &dma_buffer_pool[aligned_offset];
    dma_pool_offset = aligned_offset + aligned_size;
    
    return ptr;
}

/*
 * 创建 DMA 缓冲区
 */
bool dma_buffer_create(dma_buffer_t *buf, uint32_t capacity)
{
    if (!buf || capacity == 0) {
        return false;
    }
    
    /* 从 DMA 安全区域分配 */
    buf->data = (uint8_t *)dma_alloc(capacity);
    if (!buf->data) {
        return false;
    }
    
    buf->capacity = CACHE_ALIGN_UP(capacity);
    buf->size = 0;
    
    /* 检查是否在可缓存区域 */
    buf->is_cacheable = ram_is_cacheable((uint32_t)buf->data);
    
    /* 验证 DMA 可访问性 */
    if (!ram_is_dma_accessible((uint32_t)buf->data)) {
        return false;
    }
    
    return true;
}

/*
 * 销毁 DMA 缓冲区
 */
void dma_buffer_destroy(dma_buffer_t *buf)
{
    if (buf) {
        /* 简单池分配器不支持释放，仅清除结构 */
        memset(buf, 0, sizeof(dma_buffer_t));
    }
}

/*
 * 准备 DMA 发送
 */
void dma_buffer_prepare_tx(dma_buffer_t *buf)
{
    if (!buf || !buf->data) return;
    
    if (buf->is_cacheable) {
        /* 清除 Cache，将数据写回内存 */
        cache_prepare_dma_tx(buf->data, buf->size);
    }
}

/*
 * DMA 接收完成
 */
void dma_buffer_complete_rx(dma_buffer_t *buf, uint32_t received_size)
{
    if (!buf || !buf->data) return;
    
    buf->size = received_size;
    
    if (buf->is_cacheable) {
        /* 无效化 Cache，确保读取最新数据 */
        cache_complete_dma_rx(buf->data, buf->capacity);
    }
}

/*
 * 写入数据到缓冲区
 */
bool dma_buffer_write(dma_buffer_t *
```





```
继续
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

```c
/*
 * 写入数据到缓冲区
 */
bool dma_buffer_write(dma_buffer_t *buf, const void *data, uint32_t size)
{
    if (!buf || !buf->data || !data) return false;
    if (size > buf->capacity) return false;
    
    memcpy(buf->data, data, size);
    buf->size = size;
    
    return true;
}

/*
 * 从缓冲区读取数据
 */
bool dma_buffer_read(dma_buffer_t *buf, void *data, uint32_t size)
{
    if (!buf || !buf->data || !data) return false;
    if (size > buf->size) return false;
    
    memcpy(data, buf->data, size);
    
    return true;
}
```

### 5. MPU 配置实现非缓存区域

```c
/* mpu_config.c - MPU 配置实现 Cache 策略 */
#include "stm32h7xx.h"

/*
 * MPU 区域属性定义
 */
typedef enum {
    MPU_ATTR_STRONGLY_ORDERED = 0,  /* 强序，不可缓存 */
    MPU_ATTR_DEVICE,                 /* 设备类型 */
    MPU_ATTR_NORMAL_NC,              /* 普通内存，不可缓存 */
    MPU_ATTR_NORMAL_WT,              /* 普通内存，写透缓存 */
    MPU_ATTR_NORMAL_WB,              /* 普通内存，写回缓存 */
    MPU_ATTR_NORMAL_WB_WA,           /* 普通内存，写回+写分配 */
} mpu_attr_t;

/*
 * 配置 MPU 区域
 */
static void mpu_config_region(uint8_t region_num, 
                              uint32_t base_addr, 
                              uint32_t size,
                              mpu_attr_t attr,
                              uint8_t access_permission,
                              uint8_t executable)
{
    MPU_Region_InitTypeDef MPU_InitStruct = {0};
    
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.Number = region_num;
    MPU_InitStruct.BaseAddress = base_addr;
    MPU_InitStruct.Size = size;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.AccessPermission = access_permission;
    MPU_InitStruct.DisableExec = executable ? 
                                 MPU_INSTRUCTION_ACCESS_ENABLE : 
                                 MPU_INSTRUCTION_ACCESS_DISABLE;
    
    switch (attr) {
        case MPU_ATTR_STRONGLY_ORDERED:
            MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
            MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
            MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
            MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
            break;
            
        case MPU_ATTR_DEVICE:
            MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
            MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
            MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
            MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
            break;
            
        case MPU_ATTR_NORMAL_NC:
            MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
            MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
            MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
            MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
            break;
            
        case MPU_ATTR_NORMAL_WT:
            MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
            MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
            MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
            MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
            break;
            
        case MPU_ATTR_NORMAL_WB:
            MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
            MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
            MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
            MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
            break;
            
        case MPU_ATTR_NORMAL_WB_WA:
            MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
            MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
            MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
            MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
            break;
    }
    
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
}

/*
 * 完整的 MPU 配置 - STM32H7
 */
void MPU_Config(void)
{
    /* 禁用 MPU */
    HAL_MPU_Disable();
    
    /*
     * Region 0: Flash (0x08000000, 2MB)
     * - 可缓存，写透
     * - 只读，可执行
     */
    mpu_config_region(
        MPU_REGION_NUMBER0,
        0x08000000,
        MPU_REGION_SIZE_2MB,
        MPU_ATTR_NORMAL_WT,
        MPU_REGION_PRIV_RO_URO,
        1  /* 可执行 */
    );
    
    /*
     * Region 1: DTCM RAM (0x20000000, 128KB)
     * - 不可缓存 (直连CPU)
     * - 读写，不可执行
     */
    mpu_config_region(
        MPU_REGION_NUMBER1,
        0x20000000,
        MPU_REGION_SIZE_128KB,
        MPU_ATTR_NORMAL_NC,
        MPU_REGION_FULL_ACCESS,
        0  /* 不可执行 */
    );
    
    /*
     * Region 2: AXI SRAM (0x24000000, 512KB)
     * - 可缓存，写回+写分配
     * - 读写，不可执行
     */
    mpu_config_region(
        MPU_REGION_NUMBER2,
        0x24000000,
        MPU_REGION_SIZE_512KB,
        MPU_ATTR_NORMAL_WB_WA,
        MPU_REGION_FULL_ACCESS,
        0
    );
    
    /*
     * Region 3: SRAM1 (0x30000000, 128KB) - DMA 缓冲区
     * - 不可缓存 (避免 DMA 一致性问题)
     * - 读写，不可执行
     */
    mpu_config_region(
        MPU_REGION_NUMBER3,
        0x30000000,
        MPU_REGION_SIZE_128KB,
        MPU_ATTR_NORMAL_NC,
        MPU_REGION_FULL_ACCESS,
        0
    );
    
    /*
     * Region 4: SRAM2 (0x30020000, 128KB) - DMA 缓冲区
     * - 不可缓存
     * - 读写，不可执行
     */
    mpu_config_region(
        MPU_REGION_NUMBER4,
        0x30020000,
        MPU_REGION_SIZE_128KB,
        MPU_ATTR_NORMAL_NC,
        MPU_REGION_FULL_ACCESS,
        0
    );
    
    /*
     * Region 5: SRAM3 (0x30040000, 32KB) - BDMA 缓冲区
     * - 不可缓存
     * - 读写，不可执行
     */
    mpu_config_region(
        MPU_REGION_NUMBER5,
        0x30040000,
        MPU_REGION_SIZE_32KB,
        MPU_ATTR_NORMAL_NC,
        MPU_REGION_FULL_ACCESS,
        0
    );
    
    /*
     * Region 6: SRAM4 (0x38000000, 64KB) - D3 域
     * - 不可缓存
     * - 读写，不可执行
     */
    mpu_config_region(
        MPU_REGION_NUMBER6,
        0x38000000,
        MPU_REGION_SIZE_64KB,
        MPU_ATTR_NORMAL_NC,
        MPU_REGION_FULL_ACCESS,
        0
    );
    
    /*
     * Region 7: 外设区域 (0x40000000, 512MB)
     * - Device 类型
     * - 读写，不可执行
     */
    mpu_config_region(
        MPU_REGION_NUMBER7,
        0x40000000,
        MPU_REGION_SIZE_512MB,
        MPU_ATTR_DEVICE,
        MPU_REGION_FULL_ACCESS,
        0
    );
    
    /* 启用 MPU，使用默认内存映射作为背景 */
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/*
 * 动态配置某区域为非缓存
 * 用于运行时调整
 */
void MPU_SetRegionNonCacheable(uint8_t region_num, uint32_t base_addr, uint32_t size)
{
    HAL_MPU_Disable();
    
    mpu_config_region(
        region_num,
        base_addr,
        size,
        MPU_ATTR_NORMAL_NC,
        MPU_REGION_FULL_ACCESS,
        0
    );
    
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}
```

------

## 四、实际应用示例

### 1. 以太网 DMA 缓冲区配置

```c
/* ethernetif.c - 以太网 DMA 缓冲区 */
#include "stm32h7xx_hal.h"
#include "lwip/opt.h"
#include "cache_manager.h"

/* 以太网描述符和缓冲区 - 必须放在 D2 SRAM */
#define ETH_RX_DESC_CNT     4
#define ETH_TX_DESC_CNT     4
#define ETH_BUFF_SIZE       1536

/* DMA 描述符 - 放在 SRAM1，32字节对齐 */
__attribute__((section(".sram1"), aligned(32)))
ETH_DMADescTypeDef DMARxDscrTab[ETH_RX_DESC_CNT];

__attribute__((section(".sram1"), aligned(32)))
ETH_DMADescTypeDef DMATxDscrTab[ETH_TX_DESC_CNT];

/* 接收缓冲区 - 放在 SRAM2，32字节对齐 */
__attribute__((section(".sram2"), aligned(32)))
uint8_t Rx_Buff[ETH_RX_DESC_CNT][ETH_BUFF_SIZE];

/* 发送缓冲区 - 放在 SRAM2，32字节对齐 */
__attribute__((section(".sram2"), aligned(32)))
uint8_t Tx_Buff[ETH_TX_DESC_CNT][ETH_BUFF_SIZE];

/*
 * 以太网初始化
 */
void ETH_Init(void)
{
    ETH_HandleTypeDef heth;
    
    /* 配置以太网 */
    heth.Instance = ETH;
    heth.Init.MACAddr[0] = 0x00;
    heth.Init.MACAddr[1] = 0x80;
    heth.Init.MACAddr[2] = 0xE1;
    heth.Init.MACAddr[3] = 0x00;
    heth.Init.MACAddr[4] = 0x00;
    heth.Init.MACAddr[5] = 0x01;
    heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
    heth.Init.RxBuffLen = ETH_BUFF_SIZE;
    
    /* 验证缓冲区位置 */
    assert_param(ram_is_dma_accessible((uint32_t)DMARxDscrTab));
    assert_param(ram_is_dma_accessible((uint32_t)Rx_Buff));
    
    HAL_ETH_Init(&heth);
    
    /* 配置 DMA 描述符 */
    HAL_ETH_DescAssignMemory(&heth, 0, Rx_Buff[0], NULL);
}

/*
 * 发送以太网帧
 */
HAL_StatusTypeDef ETH_Transmit(uint8_t *data, uint16_t len)
{
    ETH_BufferTypeDef TxBuffer;
    
    /* 复制数据到 DMA 缓冲区 */
    memcpy(Tx_Buff[0], data, len);
    
    /* 如果 SRAM2 配置为可缓存，需要清除 Cache */
    #if ETH_BUFFER_CACHEABLE
    cache_prepare_dma_tx(Tx_Buff[0], len);
    #endif
    
    TxBuffer.buffer = Tx_Buff[0];
    TxBuffer.len = len;
    TxBuffer.next = NULL;
    
    return HAL_ETH_Transmit(&heth, &TxBuffer, 100);
}

/*
 * 接收以太网帧
 */
HAL_StatusTypeDef ETH_Receive(uint8_t *data, uint16_t *len)
{
    ETH_BufferTypeDef RxBuffer;
    
    if (HAL_ETH_ReadData(&heth, (void **)&RxBuffer) != HAL_OK) {
        return HAL_ERROR;
    }
    
    /* 如果 SRAM2 配置为可缓存，需要无效化 Cache */
    #if ETH_BUFFER_CACHEABLE
    cache_complete_dma_rx(RxBuffer.buffer, RxBuffer.len);
    #endif
    
    memcpy(data, RxBuffer.buffer, RxBuffer.len);
    *len = RxBuffer.len;
    
    return HAL_OK;
}
```

### 2. SPI DMA 传输示例

```c
/* spi_dma.c - SPI DMA 传输 */
#include "stm32h7xx_hal.h"
#include "cache_manager.h"

/* SPI DMA 缓冲区 - 放在 SRAM1 */
#define SPI_BUFFER_SIZE     4096

__attribute__((section(".sram1"), aligned(32)))
static uint8_t spi_tx_buffer[SPI_BUFFER_SIZE];

__attribute__((section(".sram1"), aligned(32)))
static uint8_t spi_rx_buffer[SPI_BUFFER_SIZE];

static SPI_HandleTypeDef hspi1;
static DMA_HandleTypeDef hdma_spi1_tx;
static DMA_HandleTypeDef hdma_spi1_rx;

/* 传输完成标志 */
static volatile uint8_t spi_tx_complete = 0;
static volatile uint8_t spi_rx_complete = 0;

/*
 * SPI DMA 初始化
 */
void SPI_DMA_Init(void)
{
    /* SPI1 配置 */
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;
    HAL_SPI_Init(&hspi1);
    
    /* DMA TX 配置 */
    hdma_spi1_tx.Instance = DMA1_Stream0;
    hdma_spi1_tx.Init.Request = DMA_REQUEST_SPI1_TX;
    hdma_spi1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_spi1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_spi1_tx.Init.Mode = DMA_NORMAL;
    hdma_spi1_tx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_spi1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_spi1_tx);
    __HAL_LINKDMA(&hspi1, hdmatx, hdma_spi1_tx);
    
    /* DMA RX 配置 */
    hdma_spi1_rx.Instance = DMA1_Stream1;
    hdma_spi1_rx.Init.Request = DMA_REQUEST_SPI1_RX;
    hdma_spi1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_spi1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi1_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_spi1_rx.Init.Mode = DMA_NORMAL;
    hdma_spi1_rx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_spi1_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_spi1_rx);
    __HAL_LINKDMA(&hspi1, hdmarx, hdma_spi1_rx);
    
    /* 使能 DMA 中断 */
    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
    HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
}

/*
 * SPI DMA 发送
 */
HAL_StatusTypeDef SPI_DMA_Transmit(uint8_t *data, uint16_t size)
{
    if (size > SPI_BUFFER_SIZE) {
        return HAL_ERROR;
    }
    
    /* 复制数据到 DMA 缓冲区 */
    memcpy(spi_tx_buffer, data, size);
    
    /* 清除 Cache - 确保 DMA 读取最新数据 */
    /* 注意: 如果 SRAM1 配置为非缓存，此步骤可省略 */
    cache_prepare_dma_tx(spi_tx_buffer, size);
    
    spi_tx_complete = 0;
    
    return HAL_SPI_Transmit_DMA(&hspi1, spi_tx_buffer, size);
}

/*
 * SPI DMA 接收
 */
HAL_StatusTypeDef SPI_DMA_Receive(uint8_t *data, uint16_t size)
{
    HAL_StatusTypeDef status;
    
    if (size > SPI_BUFFER_SIZE) {
        return HAL_ERROR;
    }
    
    spi_rx_complete = 0;
    
    status = HAL_SPI_Receive_DMA(&hspi1, spi_rx_buffer, size);
    if (status != HAL_OK) {
        return status;
    }
    
    /* 等待接收完成 */
    while (!spi_rx_complete) {
        /* 可以添加超时处理 */
    }
    
    /* 无效化 Cache - 确保 CPU 读取最新数据 */
    cache_complete_dma_rx(spi_rx_buffer, size);
    
    /* 复制数据 */
    memcpy(data, spi_rx_buffer, size);
    
    return HAL_OK;
}

/*
 * SPI DMA 全双工传输
 */
HAL_StatusTypeDef SPI_DMA_TransmitReceive(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    HAL_StatusTypeDef status;
    
    if (size > SPI_BUFFER_SIZE) {
        return HAL_ERROR;
    }
    
    /* 复制发送数据 */
    memcpy(spi_tx_buffer, tx_data, size);
    
    /* 清除 TX 缓冲区 Cache */
    cache_prepare_dma_tx(spi_tx_buffer, size);
    
    spi_tx_complete = 0;
    spi_rx_complete = 0;
    
    status = HAL_SPI_TransmitReceive_DMA(&hspi1, spi_tx_buffer, spi_rx_buffer, size);
    if (status != HAL_OK) {
        return status;
    }
    
    /* 等待完成 */
    while (!spi_rx_complete) {
        /* 可以添加超时处理 */
    }
    
    /* 无效化 RX 缓冲区 Cache */
    cache_complete_dma_rx(spi_rx_buffer, size);
    
    /* 复制接收数据 */
    memcpy(rx_data, spi_rx_buffer, size);
    
    return HAL_OK;
}

/*
 * DMA 中断回调
 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == &hspi1) {
        spi_tx_complete = 1;
    }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == &hspi1) {
        spi_rx_complete = 1;
    }
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == &hspi1) {
        spi_tx_complete = 1;
        spi_rx_complete = 1;
    }
}
```

### 3. LCD 帧缓冲配置

```c
/* lcd_framebuffer.c - LCD 帧缓冲 */
#include "stm32h7xx_hal.h"
#include "cache_manager.h"

/* 显示参数 */
#define LCD_WIDTH       800
#define LCD_HEIGHT      480
#define LCD_BPP         2       /* 16位色 RGB565 */
#define FB_SIZE         (LCD_WIDTH * LCD_HEIGHT * LCD_BPP)

/* 双缓冲 - 放在 AXI SRAM */
__attribute__((section(".axi_noinit"), aligned(32)))
static uint16_t framebuffer_0[LCD_WIDTH * LCD_HEIGHT];

__attribute__((section(".axi_noinit"), aligned(32)))
static uint16_t framebuffer_1[LCD_WIDTH * LCD_HEIGHT];

/* 当前显示和绘制缓冲区 */
static uint16_t *display_buffer = framebuffer_0;
static uint16_t *draw_buffer = framebuffer_1;

static LTDC_HandleTypeDef hltdc;

/*
 * LTDC 初始化
 */
void LCD_Init(void)
{
    LTDC_LayerCfgTypeDef layer_cfg;
    
    /* LTDC 配置 */
    hltdc.Instance = LTDC;
    hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
    hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
    hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
    hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
    
    /* 时序配置 (根据实际LCD调整) */
    hltdc.Init.HorizontalSync = 40;
    hltdc.Init.VerticalSync = 9;
    hltdc.Init.AccumulatedHBP = 53;
    hltdc.Init.AccumulatedVBP = 11;
    hltdc.Init.AccumulatedActiveW = 853;
    hltdc.Init.AccumulatedActiveH = 491;
    hltdc.Init.TotalWidth = 928;
    hltdc.Init.TotalHeigh = 525;
    hltdc.Init.Backcolor.Red = 0;
    hltdc.Init.Backcolor.Green = 0;
    hltdc.Init.Backcolor.Blue = 0;
    
    HAL_LTDC_Init(&hltdc);
    
    /* Layer 0 配置 */
    layer_cfg.WindowX0 = 0;
    layer_cfg.WindowX1 = LCD_WIDTH;
    layer_cfg.WindowY0 = 0;
    layer_cfg.WindowY1 = LCD_HEIGHT;
    layer_cfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
    layer_cfg.Alpha = 255;
    layer_cfg.Alpha0 = 0;
    layer_cfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
    layer_cfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
    layer_cfg.FBStartAdress = (uint32_t)display_buffer;
    layer_cfg.ImageWidth = LCD_WIDTH;
    layer_cfg.ImageHeight = LCD_HEIGHT;
    layer_cfg.Backcolor.Red = 0;
    layer_cfg.Backcolor.Green = 0;
    layer_cfg.Backcolor.Blue = 0;
    
    HAL_LTDC_ConfigLayer(&hltdc, &layer_cfg, 0);
}

/*
 * 获取绘制缓冲区
 */
uint16_t *LCD_GetDrawBuffer(void)
{
    return draw_buffer;
}

/*
 * 交换缓冲区
 */
void LCD_SwapBuffers(void)
{
    /* 清除绘制缓冲区的 Cache，确保 LTDC 读取最新数据 */
    cache_clean_by_addr(draw_buffer, FB_SIZE);
    
    /* 等待垂直消隐期 */
    while (!(LTDC->CDSR & LTDC_CDSR_VSYNCS)) {}
    
    /* 交换缓冲区 */
    uint16_t *temp = display_buffer;
    display_buffer = draw_buffer;
    draw_buffer = temp;
    
    /* 更新 LTDC 帧缓冲地址 */
    HAL_LTDC_SetAddress(&hltdc, (uint32_t)display_buffer, 0);
}

/*
 * 绘制像素
 */
void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x < LCD_WIDTH && y < LCD_HEIGHT) {
        draw_buffer[y * LCD_WIDTH + x] = color;
    }
}

/*
 * 填充矩形
 */
void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    for (uint16_t j = y; j < y + h && j < LCD_HEIGHT; j++) {
        for (uint16_t i = x; i < x + w && i < LCD_WIDTH; i++) {
            draw_buffer[j * LCD_WIDTH + i] = color;
        }
    }
}

/*
 * 清屏
 */
void LCD_Clear(uint16_t color)
{
    for (uint32_t i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        draw_buffer[i] = color;
    }
}

/*
 * 使用 DMA2D 加速填充
 */
void LCD_FillRect_DMA2D(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    DMA2D_HandleTypeDef hdma2d;
    
    hdma2d.Instance = DMA2D;
    hdma2d.Init.Mode = DMA2D_R2M;  /* 寄存器到内存 */
    hdma2d.Init.ColorMode = DMA2D_OUTPUT_RGB565;
    hdma2d.Init.OutputOffset = LCD_WIDTH - w;
    
    HAL_DMA2D_Init(&hdma2d);
    
    uint32_t dest_addr = (uint32_t)&draw_buffer[y * LCD_WIDTH + x];
    
    HAL_DMA2D_Start(&hdma2d, color, dest_addr, w, h);
    HAL_DMA2D_PollForTransfer(&hdma2d, 100);
}
```

------

## 五、KEIL 和 IAR 的对应配置

### 1. KEIL 分散加载文件 (多RAM + Cache)

```text
; stm32h743_multi_ram.sct

LR_IROM1 0x08000000 0x00200000 {

    ; Flash 代码区
    ER_IROM1 0x08000000 0x00200000 {
        *.o (RESET, +First)
        *(InRoot$$Sections)
        .ANY (+RO)
    }
    
    ; DTCM - 主数据区 (不可缓存，最快)
    RW_DTCMRAM 0x20000000 0x00020000 {
        .ANY (+RW +ZI)
    }
    
    ; ITCM - 关键代码 (不可缓存)
    ER_ITCMRAM 0x00000000 0x00010000 {
        *(ITCM)
        *(.itcm)
        ; 中断处理函数
        *_it.o (+RO)
    }
    
    ; AXI SRAM - 大缓冲区 (可缓存)
    RW_AXIRAM 0x24000000 0x00080000 {
        *(AXIRAM)
        *(.axiram)
        *(.framebuffer)
    }
    
    ; SRAM1 - DMA 缓冲区 (配置为不可缓存)
    RW_SRAM1 0x30000000 UNINIT 0x00020000 {
        *(SRAM1)
        *(.sram1)
        *(.eth_buffers)
        *(.dma_buffers)
    }
    
    ; SRAM2 - DMA 缓冲区 (配置为不可缓存)
    RW_SRAM2 0x30020000 UNINIT 0x00020000 {
        *(SRAM2)
        *(.sram2)
        *(.usb_buffers)
    }
    
    ; SRAM3 - BDMA 缓冲区
    RW_SRAM3 0x30040000 UNINIT 0x00008000 {
        *(SRAM3)
        *(.sram3)
        *(.bdma_buffers)
    }
    
    ; SRAM4 - D3 域 (低功耗保持)
    RW_SRAM4 0x38000000 UNINIT 0x00010000 {
        *(SRAM4)
        *(.sram4)
        *(.lowpower_data)
    }
    
    ; 备份 SRAM
    RW_BKPSRAM 0x38800000 UNINIT 0x00001000 {
        *(BKPSRAM)
        *(.bkpsram)
    }
}
```

### 2. KEIL C代码中使用

```c
/* KEIL 特定的段放置 */

/* ITCM 中的关键代码 */
__attribute__((section("ITCM")))
void critical_isr(void) {
    // 高速中断处理
}

/* AXI SRAM 中的帧缓冲 */
__attribute__((section("AXIRAM"), aligned(32)))
uint16_t framebuffer[800 * 480];

/* SRAM1 中的 DMA 缓冲区 */
__attribute__((section("SRAM1"), aligned(32)))
uint8_t eth_rx_buffer[1536 * 4];

__attribute__((section("SRAM1"), aligned(32)))
uint8_t eth_tx_buffer[1536 * 4];

/* SRAM2 中的 USB 缓冲区 */
__attribute__((section("SRAM2"), aligned(32)))
uint8_t usb_buffer[4096];

/* SRAM4 中的低功耗数据 */
__attribute__((section("SRAM4")))
uint32_t lowpower_config[64];

/* 备份 SRAM */
__attribute__((section("BKPSRAM")))
struct {
    uint32_t magic;
    uint32_t boot_count;
    uint8_t  config[256];
} backup_data;
```

### 3. IAR ICF 文件 (多RAM + Cache)

```icf
/*###ICF### STM32H743 多RAM配置 ###*/

/*-Memory Regions-*/
define symbol __ICFEDIT_region_ITCMRAM_start__ = 0x00000000;
define symbol __ICFEDIT_region_ITCMRAM_end__   = 0x0000FFFF;
define symbol __ICFEDIT_region_DTCMRAM_start__ = 0x20000000;
define symbol __ICFEDIT_region_DTCMRAM_end__   = 0x2001FFFF;
define symbol __ICFEDIT_region_FLASH_start__   = 0x08000000;
define symbol __ICFEDIT_region_FLASH_end__     = 0x081FFFFF;
define symbol __ICFEDIT_region_AXIRAM_start__  = 0x24000000;
define symbol __ICFEDIT_region_AXIRAM_end__    = 0x2407FFFF;
define symbol __ICFEDIT_region_SRAM1_start__   = 0x30000000;
define symbol __ICFEDIT_region_SRAM1_end__     = 0x3001FFFF;
define symbol __ICFEDIT_region_SRAM2_start__   = 0x30020000;
define symbol __ICFEDIT_region_SRAM2_end__     = 0x3003FFFF;
define symbol __ICFEDIT_region_SRAM3_start__   = 0x30040000;
define symbol __ICFEDIT_region_SRAM3_end__     = 0x30047FFF;
define symbol __ICFEDIT_region_SRAM4_start__   = 0x38000000;
define symbol __ICFEDIT_region_SRAM4_end__     = 0x3800FFFF;
define symbol __ICFEDIT_region_BKPSRAM_start__ = 0x38800000;
define symbol __ICFEDIT_region_BKPSRAM_end__   = 0x38800FFF;

/*-Sizes-*/
define symbol __ICFEDIT_size_cstack__ = 0x2000;
define symbol __ICFEDIT_size_heap__   = 0x2000;

define memory mem with size = 4G;

define region ITCMRAM_region = mem:[from __ICFEDIT_region_ITCMRAM_start__ 
                                    to __ICFEDIT_region_ITCMRAM_end__];
define region DTCMRAM_region = mem:[from __ICFEDIT_region_DTCMRAM_start__ 
                                    to __ICFEDIT_region_DTCMRAM_end__];
define region FLASH_region   = mem:[from __ICFEDIT_region_FLASH_start__ 
                                    to __ICFEDIT_region_FLASH_end__];
define region AXIRAM_region  = mem:[from __ICFEDIT_region_AXIRAM_start__ 
                                    to __ICFEDIT_region_AXIRAM_end__];
define region SRAM1_region   = mem:[from __ICFEDIT_region_SRAM1_start__ 
                                    to __ICFEDIT_region_SRAM1_end__];
define region SRAM2_region   = mem:[from __ICFEDIT_region_SRAM2_start__ 
                                    to __ICFEDIT_region_SRAM2_end__];
define region SRAM3_region   = mem:[from __ICFEDIT_region_SRAM3_start__ 
                                    to __ICFEDIT_region_SRAM3_end__];
define region SRAM4_region   = mem:[from __ICFEDIT_region_SRAM4_start__ 
                                    to __ICFEDIT_region_SRAM4_end__];
define region BKPSRAM_region = mem:[from __ICFEDIT_region_BKPSRAM_start__ 
                                    to __ICFEDIT_region_BKPSRAM_end__];

define block CSTACK with alignment = 8, size = __ICFEDIT_size_cstack__ { };
define block HEAP   with alignment = 8, size = __ICFEDIT_size_heap__   { };

/* 初始化策略 */
initialize by copy { readwrite };
initialize by copy { section .itcm };
initialize by copy { section .axiram };

/* 不初始化的段 */
do not initialize { section .noinit };
do not initialize { section .sram1, section .sram2, section .sram3 };
do not initialize { section .sram4, section .bkpsram };

/* 放置规则 */
place at address mem:0x08000000 { readonly section .intvec };

place in FLASH_region   { readonly };
place in DTCMRAM_region { readwrite, block CSTACK, block HEAP };
place in ITCMRAM_region { section .itcm };
place in AXIRAM_region  { section .axiram, section .framebuffer };
place in SRAM1_region   { section .sram1, section .eth_buffers, section .dma_buffers };
place in SRAM2_region   { section .sram2, section .usb_buffers };
place in SRAM3_region   { section .sram3, section .bdma_buffers };
place in SRAM4_region   { section .sram4, section .lowpower_data };
place in BKPSRAM_region { section .bkpsram };

/* 导出符号 */
define exported symbol __sram1_start = __ICFEDIT_region_SRAM1_start__;
define exported symbol __sram1_end   = __ICFEDIT_region_SRAM1_end__;
define exported symbol __sram2_start = __ICFEDIT_region_SRAM2_start__;
define exported symbol __sram2_end   = __ICFEDIT_region_SRAM2_end__;
```

### 4. IAR C代码中使用

```c
/* IAR 特定的段放置 */

/* ITCM 中的关键代码 */
#pragma location = ".itcm"
void critical_isr(void) {
    // 高速中断处理
}

/* 或使用 __ramfunc */
__ramfunc void flash_write(uint32_t addr, uint32_t data) {
    // Flash 编程代码
}

/* AXI SRAM 中的帧缓冲 */
#pragma location = ".framebuffer"
#pragma data_alignment = 32
uint16_t framebuffer[800 * 480];

/* SRAM1 中的 DMA 缓冲区 */
#pragma location = ".eth_buffers"
#pragma data_alignment = 32
__no_init uint8_t eth_rx_buffer[1536 * 4];

#pragma location = ".eth_buffers"
#pragma data_alignment = 32
__no_init uint8_t eth_tx_buffer[1536 * 4];

/* SRAM2 中的 USB 缓冲区 */
#pragma location = ".usb_buffers"
#pragma data_alignment = 32
__no_init uint8_t usb_buffer[4096];

/* SRAM4 中的低功耗数据 */
#pragma location = ".lowpower_data"
__no_init uint32_t lowpower_config[64];

/* 备份 SRAM */
#pragma location = ".bkpsram"
__no_init struct {
    uint32_t magic;
    uint32_t boot_count;
    uint8_t  config[256];
} backup_data;

/* 使用 @ 操作符指定地址 */
__no_init uint8_t dma_buffer[1024] @ 0x30000000;
```

------

## 六、最佳实践总结

### 1. 内存区域选择指南

|   内存区域   |      最佳用途      | Cache策略  | DMA访问 |
| :----------: | :----------------: | :--------: | :-----: |
|   **ITCM**   | 中断处理、关键算法 |  无Cache   |  不可   |
|   **DTCM**   |  栈、频繁访问变量  |  无Cache   |  不可   |
| **AXI SRAM** |   帧缓冲、大数组   |   可缓存   | DMA1/2  |
| **SRAM1/2**  |     DMA缓冲区      | 建议不缓存 | DMA1/2  |
|  **SRAM3**   |     BDMA缓冲区     | 建议不缓存 |  BDMA   |
|  **SRAM4**   |   低功耗保持数据   | 建议不缓存 |  BDMA   |
| **BKPSRAM**  |    电池备份数据    |  无Cache   |  不可   |

### 2. Cache 操作决策流程

```text
┌─────────────────────────────────────────────────────────────────┐
│                    Cache 操作决策流程                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────┐                                               │
│  │ 数据传输方向 │                                               │
│  └──────┬──────┘                                               │
│         │                                                       │
│    ┌────┴────┐                                                 │
│    ▼         ▼                                                 │
│ ┌─────┐  ┌─────┐                                              │
│ │ TX  │  │ RX  │                                              │
│ │CPU→ │  │→CPU │                                              │
│ │DMA  │  │DMA  │                                              │
│ └──┬──┘  └──┬──┘                                              │
│    │        │                                                   │
│    ▼        ▼                                                   │
│ ┌──────────────────┐  ┌──────────────────┐                    │
│ │ DMA传输前:       │  │ DMA传输后:       │                    │
│ │ Clean D-Cache    │  │ Invalidate       │                    │
│ │ (写回内存)       │  │ D-Cache          │                    │
│ │                  │  │ (丢弃缓存)       │                    │
│ └──────────────────┘  └──────────────────┘                    │
│                                                                 │
│  或者: 将DMA缓冲区放在非缓存区域 (SRAM1/2)                      │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 3. 常见问题排查清单

|      问题现象      |       可能原因        |           解决方案            |
| :----------------: | :-------------------: | :---------------------------: |
|  DMA传输数据错误   |      Cache一致性      | 使用非缓存区域或正确维护Cache |
|    DMA传输失败     |  缓冲区在错误内存域   |     将缓冲区移到D2域SRAM      |
|    以太网不工作    | 描述符/缓冲区位置错误 |          放在SRAM1/2          |
|    LCD显示异常     |   帧缓冲Cache未同步   |       刷新前Clean Cache       |
|   跨Bank访问崩溃   |   数据跨越内存边界    |       使用边界检查函数        |
| 低功耗唤醒数据丢失 |     数据不在D3域      |      使用SRAM4或BKPSRAM       |
|    性能不达预期    |    关键代码不在TCM    |      将热点代码放入ITCM       |

### 4. 性能优化建议

```c
/* 性能优化示例 */

/* 1. 中断处理放入 ITCM */
__attribute__((section(".itcm")))
void TIM1_UP_IRQHandler(void) {
    // 高速定时器中断
}

/* 2. 频繁访问的变量放入 DTCM */
__attribute__((section(".dtcm")))
volatile uint32_t system_tick;

/* 3. 大数组使用 AXI SRAM + Cache */
__attribute__((section(".axiram")))
float fft_buffer[4096];

/* 4. DMA 缓冲区使用非缓存 SRAM */
__attribute__((section(".sram1"), aligned(32)))
uint8_t adc_dma_buffer[1024];

/* 5. 使用 DMA2D 加速图形操作 */
void fast_memset_32(void *dst, uint32_t value, size_t size) {
    DMA2D->CR = DMA2D_R2M;
    DMA2D->OCOLR = value;
    DMA2D->OMAR = (uint32_t)dst;
    DMA2D->OOR = 0;
    DMA2D->NLR = (size / 4) | (1 << 16);
    DMA2D->CR |= DMA2D_CR_START;
    while (DMA2D->CR & DMA2D_CR_START);
}
```

------

如需针对特定应用场景（如音频处理、高速ADC采集、USB高速传输等）的更详细配置，请告诉我具体需求！





```
音频处理、高速ADC采集、USB高速传输场景配置
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

# STM32 音频处理、高速ADC采集、USB高速传输场景配置详解

------

## 一、整体架构设计

### 1. 多场景内存布局规划

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                    STM32H7 多场景内存布局                                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ITCM (64KB)                    DTCM (128KB)                           │
│  ┌─────────────────────┐       ┌─────────────────────────────────┐    │
│  │ 音频DSP算法 (32KB)  │       │ 系统栈 (8KB)                    │    │
│  │ ADC处理函数 (16KB)  │       │ 系统堆 (8KB)                    │    │
│  │ USB中断处理 (8KB)   │       │ 音频系数/状态 (32KB)            │    │
│  │ 关键中断 (8KB)      │       │ ADC处理缓存 (32KB)              │    │
│  └─────────────────────┘       │ USB控制结构 (16KB)              │    │
│                                │ 全局变量 (32KB)                 │    │
│                                └─────────────────────────────────┘    │
│                                                                         │
│  AXI SRAM (512KB)              SRAM1 (128KB) - DMA                     │
│  ┌─────────────────────┐       ┌─────────────────────────────────┐    │
│  │ 音频大缓冲 (256KB)  │       │ SAI DMA缓冲 (64KB)              │    │
│  │ - 输入环形缓冲      │       │ ADC DMA缓冲 (32KB)              │    │
│  │ - 输出环形缓冲      │       │ 通用DMA缓冲 (32KB)              │    │
│  │ - FFT工作区         │       └─────────────────────────────────┘    │
│  │ 显示帧缓冲 (256KB)  │                                               │
│  └─────────────────────┘       SRAM2 (128KB) - USB DMA                │
│                                ┌─────────────────────────────────┐    │
│  SRAM3 (32KB) - BDMA           │ USB HS缓冲 (64KB)               │    │
│  ┌─────────────────────┐       │ USB描述符 (4KB)                 │    │
│  │ LPUART DMA (8KB)    │       │ USB端点缓冲 (60KB)              │    │
│  │ I2C4 DMA (8KB)      │       └─────────────────────────────────┘    │
│  │ SPI6 DMA (16KB)     │                                               │
│  └─────────────────────┘       SRAM4 (64KB) - 低功耗                  │
│                                ┌─────────────────────────────────┐    │
│                                │ 低功耗保持数据 (64KB)           │    │
│                                └─────────────────────────────────┘    │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 2. 完整链接脚本

```ld
/* stm32h743_audio_adc_usb.ld - 音频/ADC/USB 综合配置 */
ENTRY(Reset_Handler)

/* 栈和堆配置 */
_Min_Stack_Size = 0x2000;  /* 8KB 主栈 */
_Min_Heap_Size  = 0x2000;  /* 8KB 堆 */

MEMORY
{
    /* Flash 双Bank */
    FLASH_BANK1 (rx)    : ORIGIN = 0x08000000, LENGTH = 1024K
    FLASH_BANK2 (rx)    : ORIGIN = 0x08100000, LENGTH = 1024K
    
    /* D1 域 - CPU 直连 */
    ITCMRAM (xrw)       : ORIGIN = 0x00000000, LENGTH = 64K
    DTCMRAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 128K
    AXIRAM (xrw)        : ORIGIN = 0x24000000, LENGTH = 512K
    
    /* D2 域 - DMA 可访问 */
    SRAM1 (xrw)         : ORIGIN = 0x30000000, LENGTH = 128K
    SRAM2 (xrw)         : ORIGIN = 0x30020000, LENGTH = 128K
    SRAM3 (xrw)         : ORIGIN = 0x30040000, LENGTH = 32K
    
    /* D3 域 - 低功耗可访问 */
    SRAM4 (xrw)         : ORIGIN = 0x38000000, LENGTH = 64K
    BKPSRAM (rw)        : ORIGIN = 0x38800000, LENGTH = 4K
}

/* 内存边界符号 */
__itcm_start = ORIGIN(ITCMRAM);
__itcm_end   = ORIGIN(ITCMRAM) + LENGTH(ITCMRAM);
__dtcm_start = ORIGIN(DTCMRAM);
__dtcm_end   = ORIGIN(DTCMRAM) + LENGTH(DTCMRAM);
__axi_start  = ORIGIN(AXIRAM);
__axi_end    = ORIGIN(AXIRAM) + LENGTH(AXIRAM);
__sram1_start = ORIGIN(SRAM1);
__sram1_end   = ORIGIN(SRAM1) + LENGTH(SRAM1);
__sram2_start = ORIGIN(SRAM2);
__sram2_end   = ORIGIN(SRAM2) + LENGTH(SRAM2);

SECTIONS
{
    /* ==================== Flash 代码段 ==================== */
    
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH_BANK1

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        
        KEEP(*(.init))
        KEEP(*(.fini))
        
        . = ALIGN(4);
        _etext = .;
    } > FLASH_BANK1

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        
        /* 音频系数表 */
        . = ALIGN(4);
        __audio_coef_start = .;
        *(.audio_coef)
        *(.audio_coef*)
        __audio_coef_end = .;
        
        . = ALIGN(4);
    } > FLASH_BANK1

    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH_BANK1
    
    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH_BANK1

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH_BANK1

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH_BANK1

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH_BANK1

    /* ==================== ITCM - 关键代码 ==================== */
    
    _siitcm = LOADADDR(.itcm);
    
    .itcm :
    {
        . = ALIGN(4);
        _sitcm = .;
        
        /* 音频DSP算法 */
        *(.itcm.audio)
        *(.itcm.audio*)
        *audio_dsp.o(.text*)
        *arm_*.o(.text*)           /* CMSIS-DSP 库 */
        
        /* ADC处理函数 */
        *(.itcm.adc)
        *(.itcm.adc*)
        *adc_process.o(.text*)
        
        /* USB中断处理 */
        *(.itcm.usb)
        *(.itcm.usb*)
        
        /* 关键中断处理 */
        *(.itcm.isr)
        *(.itcm.isr*)
        *stm32h7xx_it.o(.text*)
        
        . = ALIGN(4);
        _eitcm = .;
    } > ITCMRAM AT > FLASH_BANK1

    /* ==================== DTCM - 主数据区 ==================== */
    
    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        *(.RamFunc)
        *(.RamFunc*)
        . = ALIGN(4);
        _edata = .;
    } > DTCMRAM AT > FLASH_BANK1

    /* 音频处理状态和系数 - DTCM */
    .audio_state (NOLOAD) :
    {
        . = ALIGN(4);
        __audio_state_start = .;
        *(.audio_state)
        *(.audio_state*)
        *(.biquad_state)
        *(.fir_state)
        . = ALIGN(4);
        __audio_state_end = .;
    } > DTCMRAM

    /* ADC处理缓存 - DTCM */
    .adc_cache (NOLOAD) :
    {
        . = ALIGN(4);
        __adc_cache_start = .;
        *(.adc_cache)
        *(.adc_cache*)
        . = ALIGN(4);
        __adc_cache_end = .;
    } > DTCMRAM

    /* USB控制结构 - DTCM */
    .usb_ctrl (NOLOAD) :
    {
        . = ALIGN(4);
        __usb_ctrl_start = .;
        *(.usb_ctrl)
        *(.usb_ctrl*)
        . = ALIGN(4);
        __usb_ctrl_end = .;
    } > DTCMRAM

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > DTCMRAM

    ._user_heap_stack (NOLOAD) :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        __heap_start = .;
        . = . + _Min_Heap_Size;
        __heap_end = .;
        __stack_start = .;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
        __stack_end = .;
    } > DTCMRAM

    _estack = ORIGIN(DTCMRAM) + LENGTH(DTCMRAM);

    /* ==================== AXI SRAM - 大缓冲区 ==================== */
    
    _siaxi = LOADADDR(.axiram);
    
    .axiram :
    {
        . = ALIGN(4);
        _saxi = .;
        *(.axiram)
        *(.axiram*)
        . = ALIGN(4);
        _eaxi = .;
    } > AXIRAM AT > FLASH_BANK1

    /* 音频大缓冲区 - AXI SRAM */
    .audio_buffer (NOLOAD) :
    {
        . = ALIGN(32);
        __audio_buffer_start = .;
        *(.audio_buffer)
        *(.audio_buffer*)
        *(.audio_ringbuf)
        *(.fft_buffer)
        . = ALIGN(32);
        __audio_buffer_end = .;
    } > AXIRAM

    /* 显示帧缓冲 - AXI SRAM */
    .framebuffer (NOLOAD) :
    {
        . = ALIGN(32);
        __framebuffer_start = .;
        *(.framebuffer)
        *(.framebuffer*)
        . = ALIGN(32);
        __framebuffer_end = .;
    } > AXIRAM

    /* ==================== SRAM1 - 音频/ADC DMA ==================== */
    
    .sram1_dma (NOLOAD) :
    {
        . = ALIGN(32);
        __sram1_start = .;
        
        /* SAI DMA 缓冲区 */
        *(.sai_dma)
        *(.sai_dma*)
        *(.sai_tx_buffer)
        *(.sai_rx_buffer)
        
        /* ADC DMA 缓冲区 */
        *(.adc_dma)
        *(.adc_dma*)
        
        /* 通用 DMA 缓冲区 */
        *(.sram1)
        *(.sram1*)
        *(.dma_buffer)
        
        . = ALIGN(32);
        __sram1_end = .;
    } > SRAM1

    /* ==================== SRAM2 - USB DMA ==================== */
    
    .sram2_usb (NOLOAD) :
    {
        . = ALIGN(32);
        __sram2_start = .;
        
        /* USB 描述符 */
        *(.usb_desc)
        *(.usb_desc*)
        
        /* USB 端点缓冲区 */
        *(.usb_ep_buffer)
        *(.usb_ep_buffer*)
        
        /* USB 数据缓冲区 */
        *(.usb_data)
        *(.usb_data*)
        *(.sram2)
        *(.sram2*)
        
        . = ALIGN(32);
        __sram2_end = .;
    } > SRAM2

    /* ==================== SRAM3 - BDMA ==================== */
    
    .sram3_bdma (NOLOAD) :
    {
        . = ALIGN(4);
        __sram3_start = .;
        *(.sram3)
        *(.sram3*)
        *(.bdma_buffer)
        . = ALIGN(4);
        __sram3_end = .;
    } > SRAM3

    /* ==================== SRAM4 - 低功耗 ==================== */
    
    .sram4 (NOLOAD) :
    {
        . = ALIGN(4);
        __sram4_start = .;
        *(.sram4)
        *(.sram4*)
        *(.lowpower)
        . = ALIGN(4);
        __sram4_end = .;
    } > SRAM4

    /* ==================== 备份 SRAM ==================== */
    
    .bkpsram (NOLOAD) :
    {
        . = ALIGN(4);
        __bkpsram_start = .;
        *(.bkpsram)
        *(.bkpsram*)
        . = ALIGN(4);
        __bkpsram_end = .;
    } > BKPSRAM

    /* ==================== 调试信息 ==================== */
    
    .ARM.attributes 0 : { *(.ARM.attributes) }
}

/* ==================== 断言检查 ==================== */
ASSERT(_eitcm <= __itcm_end, "ITCM overflow!")
ASSERT(_estack <= __dtcm_end, "DTCM overflow!")
ASSERT(__audio_buffer_end <= __axi_end, "AXI SRAM overflow!")
ASSERT(__sram1_end <= ORIGIN(SRAM1) + LENGTH(SRAM1), "SRAM1 overflow!")
ASSERT(__sram2_end <= ORIGIN(SRAM2) + LENGTH(SRAM2), "SRAM2 overflow!")
```

------

## 二、音频处理配置

### 1. 音频系统架构

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                         音频处理系统架构                                 │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌─────────┐    ┌─────────┐    ┌─────────────┐    ┌─────────┐         │
│  │ 音频    │    │  SAI    │    │   DMA       │    │ SRAM1   │         │
│  │ Codec   │───▶│  接口   │───▶│  传输       │───▶│ 缓冲区  │         │
│  │ (I2S)   │    │         │    │             │    │         │         │
│  └─────────┘    └─────────┘    └─────────────┘    └────┬────┘         │
│                                                        │               │
│                                                        ▼               │
│                                              ┌─────────────────┐       │
│                                              │   环形缓冲区    │       │
│                                              │   (AXI SRAM)    │       │
│                                              └────────┬────────┘       │
│                                                       │               │
│                                                       ▼               │
│  ┌─────────────────────────────────────────────────────────────────┐ │
│  │                      音频处理流水线 (ITCM)                       │ │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌───────┐ │ │
│  │  │ 预处理  │─▶│ 滤波器  │─▶│  FFT    │─▶│ 效果器  │─▶│ 后处理│ │ │
│  │  │ (增益)  │  │ (EQ)    │  │ (频谱)  │  │ (混响)  │  │ (限幅)│ │ │
│  │  └─────────┘  └─────────┘  └─────────┘  └─────────┘  └───────┘ │ │
│  └─────────────────────────────────────────────────────────────────┘ │
│                                                       │               │
│                                                       ▼               │
│                                              ┌─────────────────┐       │
│                                              │   输出环形缓冲  │       │
│                                              │   (AXI SRAM)    │       │
│                                              └────────┬────────┘       │
│                                                       │               │
│                                                       ▼               │
│  ┌─────────┐    ┌─────────┐    ┌─────────────┐    ┌─────────┐         │
│  │ 音频    │◀───│  SAI    │◀───│   DMA       │◀───│ SRAM1   │         │
│  │ Codec   │    │  接口   │    │  传输       │    │ 缓冲区  │         │
│  │ (I2S)   │    │         │    │             │    │         │         │
│  └─────────┘    └─────────┘    └─────────────┘    └─────────┘         │
│                                                                         │
│  采样率: 48kHz / 96kHz / 192kHz                                        │
│  位深度: 16bit / 24bit / 32bit                                         │
│  通道数: 2 (立体声)                                                    │
│  帧大小: 256 / 512 / 1024 samples                                      │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 2. 音频缓冲区定义

```c
/* audio_buffer.h */
#ifndef __AUDIO_BUFFER_H
#define __AUDIO_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

/* 音频参数配置 */
#define AUDIO_SAMPLE_RATE       48000
#define AUDIO_BIT_DEPTH         24
#define AUDIO_CHANNELS          2
#define AUDIO_FRAME_SIZE        256     /* 每帧采样数 */
#define AUDIO_DMA_BUFFER_FRAMES 4       /* DMA 缓冲帧数 */
#define AUDIO_RING_BUFFER_FRAMES 16     /* 环形缓冲帧数 */

/* 计算缓冲区大小 */
#define AUDIO_SAMPLE_SIZE       (AUDIO_BIT_DEPTH / 8)
#define AUDIO_FRAME_BYTES       (AUDIO_FRAME_SIZE * AUDIO_CHANNELS * AUDIO_SAMPLE_SIZE)
#define AUDIO_DMA_BUFFER_SIZE   (AUDIO_FRAME_BYTES * AUDIO_DMA_BUFFER_FRAMES)
#define AUDIO_RING_BUFFER_SIZE  (AUDIO_FRAME_BYTES * AUDIO_RING_BUFFER_FRAMES)

/* FFT 配置 */
#define FFT_SIZE                1024
#define FFT_BUFFER_SIZE         (FFT_SIZE * sizeof(float) * 2)  /* 复数 */

/* 音频帧结构 */
typedef struct {
    int32_t left;
    int32_t right;
} audio_sample_t;

typedef struct {
    audio_sample_t samples[AUDIO_FRAME_SIZE];
    uint32_t timestamp;
    uint8_t  flags;
} audio_frame_t;

/* 环形缓冲区结构 */
typedef struct {
    audio_frame_t *buffer;
    uint32_t size;          /* 帧数 */
    volatile uint32_t head;
    volatile uint32_t tail;
    volatile uint32_t count;
} audio_ringbuf_t;

/* 初始化环形缓冲区 */
void audio_ringbuf_init(audio_ringbuf_t *rb, audio_frame_t *buffer, uint32_t size);

/* 写入帧 */
bool audio_ringbuf_write(audio_ringbuf_t *rb, const audio_frame_t *frame);

/* 读取帧 */
bool audio_ringbuf_read(audio_ringbuf_t *rb, audio_frame_t *frame);

/* 获取可用帧数 */
uint32_t audio_ringbuf_available(audio_ringbuf_t *rb);

/* 获取空闲帧数 */
uint32_t audio_ringbuf_free(audio_ringbuf_t *rb);

#endif /* __AUDIO_BUFFER_H */
/* audio_buffer.c */
#include "audio_buffer.h"
#include <string.h>

/* ==================== SAI DMA 缓冲区 - SRAM1 ==================== */

/* 接收 DMA 缓冲区 (双缓冲) */
__attribute__((section(".sai_rx_buffer"), aligned(32)))
uint8_t sai_rx_buffer[AUDIO_DMA_BUFFER_SIZE];

/* 发送 DMA 缓冲区 (双缓冲) */
__attribute__((section(".sai_tx_buffer"), aligned(32)))
uint8_t sai_tx_buffer[AUDIO_DMA_BUFFER_SIZE];

/* ==================== 环形缓冲区 - AXI SRAM ==================== */

/* 输入环形缓冲区 */
__attribute__((section(".audio_ringbuf"), aligned(32)))
audio_frame_t audio_input_buffer[AUDIO_RING_BUFFER_FRAMES];

/* 输出环形缓冲区 */
__attribute__((section(".audio_ringbuf"), aligned(32)))
audio_frame_t audio_output_buffer[AUDIO_RING_BUFFER_FRAMES];

/* ==================== FFT 工作区 - AXI SRAM ==================== */

__attribute__((section(".fft_buffer"), aligned(32)))
float fft_input[FFT_SIZE * 2];

__attribute__((section(".fft_buffer"), aligned(32)))
float fft_output[FFT_SIZE * 2];

__attribute__((section(".fft_buffer"), aligned(32)))
float fft_magnitude[FFT_SIZE / 2];

/* ==================== 音频处理状态 - DTCM ==================== */

__attribute__((section(".audio_state")))
static audio_ringbuf_t input_ringbuf;

__attribute__((section(".audio_state")))
static audio_ringbuf_t output_ringbuf;

/* ==================== 环形缓冲区实现 ==================== */

void audio_ringbuf_init(audio_ringbuf_t *rb, audio_frame_t *buffer, uint32_t size)
{
    rb->buffer = buffer;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

bool audio_ringbuf_write(audio_ringbuf_t *rb, const audio_frame_t *frame)
{
    if (rb->count >= rb->size) {
        return false;  /* 缓冲区满 */
    }
    
    memcpy(&rb->buffer[rb->head], frame, sizeof(audio_frame_t));
    rb->head = (rb->head + 1) % rb->size;
    __atomic_fetch_add(&rb->count, 1, __ATOMIC_SEQ_CST);
    
    return true;
}

bool audio_ringbuf_read(audio_ringbuf_t *rb, audio_frame_t *frame)
{
    if (rb->count == 0) {
        return false;  /* 缓冲区空 */
    }
    
    memcpy(frame, &rb->buffer[rb->tail], sizeof(audio_frame_t));
    rb->tail = (rb->tail + 1) % rb->size;
    __atomic_fetch_sub(&rb->count, 1, __ATOMIC_SEQ_CST);
    
    return true;
}

uint32_t audio_ringbuf_available(audio_ringbuf_t *rb)
{
    return rb->count;
}

uint32_t audio_ringbuf_free(audio_ringbuf_t *rb)
{
    return rb->size - rb->count;
}

/* 初始化音频缓冲系统 */
void audio_buffer_system_init(void)
{
    audio_ringbuf_init(&input_ringbuf, audio_input_buffer, AUDIO_RING_BUFFER_FRAMES);
    audio_ringbuf_init(&output_ringbuf, audio_output_buffer, AUDIO_RING_BUFFER_FRAMES);
}

/* 获取输入环形缓冲区 */
audio_ringbuf_t *audio_get_input_ringbuf(void)
{
    return &input_ringbuf;
}

/* 获取输出环形缓冲区 */
audio_ringbuf_t *audio_get_output_ringbuf(void)
{
    return &output_ringbuf;
}
```

### 3. SAI DMA 配置

```c
/* sai_audio.c - SAI 音频接口配置 */
#include "stm32h7xx_hal.h"
#include "audio_buffer.h"
#include "cache_manager.h"

/* 外部缓冲区声明 */
extern uint8_t sai_rx_buffer[];
extern uint8_t sai_tx_buffer[];

/* SAI 句柄 */
static SAI_HandleTypeDef hsai_rx;
static SAI_HandleTypeDef hsai_tx;
static DMA_HandleTypeDef hdma_sai_rx;
static DMA_HandleTypeDef hdma_sai_tx;

/* 回调函数指针 */
static void (*audio_rx_half_callback)(void) = NULL;
static void (*audio_rx_full_callback)(void) = NULL;
static void (*audio_tx_half_callback)(void) = NULL;
static void (*audio_tx_full_callback)(void) = NULL;

/*
 * SAI 初始化
 */
void SAI_Audio_Init(void)
{
    /* ==================== SAI RX 配置 (SAI1_A) ==================== */
    
    hsai_rx.Instance = SAI1_Block_A;
    hsai_rx.Init.AudioMode = SAI_MODEMASTER_RX;
    hsai_rx.Init.Synchro = SAI_ASYNCHRONOUS;
    hsai_rx.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;
    hsai_rx.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
    hsai_rx.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_1QF;
    hsai_rx.Init.AudioFrequency = AUDIO_SAMPLE_RATE;
    hsai_rx.Init.SynchroExt = SAI_SYNCEXT_DISABLE;
    hsai_rx.Init.MonoStereoMode = SAI_STEREOMODE;
    hsai_rx.Init.CompandingMode = SAI_NOCOMPANDING;
    hsai_rx.Init.Protocol = SAI_FREE_PROTOCOL;
    hsai_rx.Init.DataSize = SAI_DATASIZE_24;
    hsai_rx.Init.FirstBit = SAI_FIRSTBIT_MSB;
    hsai_rx.Init.ClockStrobing = SAI_CLOCKSTROBING_FALLINGEDGE;
    
    /* 帧配置 */
    hsai_rx.FrameInit.FrameLength = 64;
    hsai_rx.FrameInit.ActiveFrameLength = 32;
    hsai_rx.FrameInit.FSDefinition = SAI_FS_CHANNEL_IDENTIFICATION;
    hsai_rx.FrameInit.FSPolarity = SAI_FS_ACTIVE_LOW;
    hsai_rx.FrameInit.FSOffset = SAI_FS_BEFOREFIRSTBIT;
    
    /* 时隙配置 */
    hsai_rx.SlotInit.FirstBitOffset = 0;
    hsai_rx.SlotInit.SlotSize = SAI_SLOTSIZE_32B;
    hsai_rx.SlotInit.SlotNumber = 2;
    hsai_rx.SlotInit.SlotActive = SAI_SLOTACTIVE_0 | SAI_SLOTACTIVE_1;
    
    HAL_SAI_Init(&hsai_rx);
    
    /* ==================== SAI TX 配置 (SAI1_B) ==================== */
    
    hsai_tx.Instance = SAI1_Block_B;
    hsai_tx.Init.AudioMode = SAI_MODESLAVE_TX;
    hsai_tx.Init.Synchro = SAI_SYNCHRONOUS;
    hsai_tx.Init.OutputDrive = SAI_OUTPUTDRIVE_ENABLE;
    hsai_tx.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_1QF;
    hsai_tx.Init.SynchroExt = SAI_SYNCEXT_DISABLE;
    hsai_tx.Init.MonoStereoMode = SAI_STEREOMODE;
    hsai_tx.Init.CompandingMode = SAI_NOCOMPANDING;
    hsai_tx.Init.Protocol = SAI_FREE_PROTOCOL;
    hsai_tx.Init.DataSize = SAI_DATASIZE_24;
    hsai_tx.Init.FirstBit = SAI_FIRSTBIT_MSB;
    hsai_tx.Init.ClockStrobing = SAI_CLOCKSTROBING_FALLINGEDGE;
    
    hsai_tx.FrameInit.FrameLength = 64;
    hsai_tx.FrameInit.ActiveFrameLength = 32;
    hsai_tx.FrameInit.FSDefinition = SAI_FS_CHANNEL_IDENTIFICATION;
    hsai_tx.FrameInit.FSPolarity = SAI_FS_ACTIVE_LOW;
    hsai_tx.FrameInit.FSOffset = SAI_FS_BEFOREFIRSTBIT;
    
    hsai_tx.SlotInit.FirstBitOffset = 0;
    hsai_tx.SlotInit.SlotSize = SAI_SLOTSIZE_32B;
    hsai_tx.SlotInit.SlotNumber = 2;
    hsai_tx.SlotInit.SlotActive = SAI_SLOTACTIVE_0 | SAI_SLOTACTIVE_1;
    
    HAL_SAI_Init(&hsai_tx);
    
    /* ==================== DMA RX 配置 ==================== */
    
    hdma_sai_rx.Instance = DMA1_Stream0;
    hdma_sai_rx.Init.Request = DMA_REQUEST_SAI1_A;
    hdma_sai_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_sai_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_sai_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_sai_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_sai_rx.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_sai_rx.Init.Mode = DMA_CIRCULAR;
    hdma_sai_rx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_sai_rx.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    hdma_sai_rx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    hdma_sai_rx.Init.MemBurst = DMA_MBURST_SINGLE;
    hdma_sai_rx.Init.PeriphBurst = DMA_PBURST_SINGLE;
    
    HAL_DMA_Init(&hdma_sai_rx);
    __HAL_LINKDMA(&hsai_rx, hdmarx, hdma_sai_rx);
    
    /* ==================== DMA TX 配置 ==================== */
    
    hdma_sai_tx.Instance = DMA1_Stream1;
    hdma_sai_tx.Init.Request = DMA_REQUEST_SAI1_B;
    hdma_sai_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_sai_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_sai_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_sai_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_sai_tx.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_sai_tx.Init.Mode = DMA_CIRCULAR;
    hdma_sai_tx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_sai_tx.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    hdma_sai_tx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    hdma_sai_tx.Init.MemBurst = DMA_MBURST_SINGLE;
    hdma_sai_tx.Init.PeriphBurst = DMA_PBURST_SINGLE;
    
    HAL_DMA_Init(&hdma_sai_tx);
    __HAL_LINKDMA(&hsai_tx, hdmatx, hdma_sai_tx);
    
    /* ==================== 中断配置 ==================== */
    
    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
    
    HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
}

/*
 * 注册回调函数
 */
void SAI_Audio_RegisterCallbacks(
    void (*rx_half)(void),
    void (*rx_full)(void),
    void (*tx_half)(void),
    void (*tx_full)(void))
{
    audio_rx_half_callback = rx_half;
    audio_rx_full_callback = rx_full;
    audio_tx_half_callback = tx_half;
    audio_tx_full_callback = tx_full;
}

/*
 * 启动音频传输
 */
void SAI_Audio_Start(void)
{
    /* 清零发送缓冲区 */
    memset(sai_tx_buffer, 0, AUDIO_DMA_BUFFER_SIZE);
    
    /* 启动 DMA 传输 */
    HAL_SAI_Receive_DMA(&hsai_rx, sai_rx_buffer, 
                        AUDIO_DMA_BUFFER_SIZE / sizeof(uint32_t));
    HAL_SAI_Transmit_DMA(&hsai_tx, sai_tx_buffer, 
                         AUDIO_DMA_BUFFER_SIZE / sizeof(uint32_t));
}

/*
 * 停止音频传输
 */
void SAI_Audio_Stop(void)
{
    HAL_SAI_DMAStop(&hsai_rx);
    HAL_SAI_DMAStop(&hsai_tx);
}

/*
 * 获取接收缓冲区指针 (半缓冲)
 */
uint8_t *SAI_Audio_GetRxBuffer(uint8_t half)
{
    if (half == 0) {
        return sai_rx_buffer;
    } else {
        return sai_rx_buffer + (AUDIO_DMA_BUFFER_SIZE / 2);
    }
}

/*
 * 获取发送缓冲区指针 (半缓冲)
 */
uint8_t *SAI_Audio_GetTxBuffer(uint8_t half)
{
    if (half == 0) {
        return sai_tx_buffer;
    } else {
        return sai_tx_buffer + (AUDIO_DMA_BUFFER_SIZE / 2);
    }
}

/* ==================== DMA 中断回调 ==================== */

void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    if (hsai == &hsai_rx && audio_rx_half_callback) {
        /* SRAM1 配置为非缓存，无需 Cache 操作 */
        audio_rx_half_callback();
    }
}

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
    if (hsai == &hsai_rx && audio_rx_full_callback) {
        audio_rx_full_callback();
    }
}

void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    if (hsai == &hsai_tx && audio_tx_half_callback) {
        audio_tx_half_callback();
    }
}

void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{
    if (hsai == &hsai_tx && audio_tx_full_callback) {
        audio_tx_full_callback();
    }
}

/* ==================== 中断处理函数 ==================== */

void DMA1_Stream0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_sai_rx);
}

void DMA1_Stream1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_sai_tx);
}
```

### 4. 音频DSP处理 (ITCM优化)

```c
/* audio_dsp.c - 音频DSP处理 (放入ITCM) */
#include "audio_buffer.h"
#include "arm_math.h"

/* ==================== DSP 状态结构 - DTCM ==================== */

/* 双二阶滤波器状态 */
__attribute__((section(".biquad_state")))
static arm_biquad_casd_df1_inst_f32 biquad_lowpass;

__attribute__((section(".biquad_state")))
static arm_biquad_casd_df1_inst_f32 biquad_highpass;

__attribute__((section(".biquad_state")))
static float biquad_lowpass_state[4 * 2];  /* 2阶 */

__attribute__((section(".biquad_state")))
static float biquad_highpass_state[4 * 2];

/* FIR 滤波器状态 */
#define FIR_TAP_NUM     64

__attribute__((section(".fir_state")))
static arm_fir_instance_f32 fir_filter;

__attribute__((section(".fir_state")))
static float fir_state[FIR_TAP_NUM + AUDIO_FRAME_SIZE - 1];

/* FFT 实例 */
__attribute__((section(".audio_state")))
static arm_rfft_fast_instance_f32 fft_instance;

/* ==================== 滤波器系数 - Flash ==================== */

__attribute__((section(".audio_coef")))
static const float lowpass_coeffs[5 * 2] = {
    /* b0, b1, b2, a1, a2 for each stage */
    0.0675f, 0.1349f, 0.0675f, -1.1430f, 0.4128f,
    0.0675f, 0.1349f, 0.0675f, -1.1430f, 0.4128f,
};

__attribute__((section(".audio_coef")))
static const float highpass_coeffs[5 * 2] = {
    0.8008f, -1.6016f, 0.8008f, -1.5610f, 0.6414f,
    0.8008f, -1.6016f, 0.8008f, -1.5610f, 0.6414f,
};

__attribute__((section(".audio_coef")))
static const float fir_coeffs[FIR_TAP_NUM] = {
    /* FIR 滤波器系数 */
    0.0f, /* ... 64个系数 ... */
};

/* ==================== 处理缓冲区 - DTCM ==================== */

__attribute__((section(".adc_cache")))
static float process_buffer_l[AUDIO_FRAME_SIZE];

__attribute__((section(".adc_cache")))
static float process_buffer_r[AUDIO_FRAME_SIZE];

__attribute__((section(".adc_cache")))
static float temp_buffer[AUDIO_FRAME_SIZE];

/* ==================== DSP 初始化 ==================== */

void audio_dsp_init(void)
{
    /* 初始化双二阶滤波器 */
    arm_biquad_cascade_df1_init_f32(&biquad_lowpass, 2, 
                                    (float *)lowpass_coeffs, 
                                    biquad_lowpass_state);
    
    arm_biquad_cascade_df1_init_f32(&biquad_highpass, 2, 
                                    (float *)highpass_coeffs, 
                                    biquad_highpass_state);
    
    /* 初始化 FIR 滤波器 */
    arm_fir_init_f32(&fir_filter, FIR_TAP_NUM, 
                     (float *)fir_coeffs, fir_state, AUDIO_FRAME_SIZE);
    
    /* 初始化 FFT */
    arm_rfft_fast_init_f32(&fft_instance, FFT_SIZE);
}

/* ==================== 核心处理函数 - ITCM ==================== */

/*
 * 将 24bit 整数转换为浮点
 */
__attribute__((section(".itcm.audio")))
static inline float int24_to_float(int32_t sample)
{
    /* 24bit 有符号数，符号扩展 */
    if (sample & 0x800000) {
        sample |= 0xFF000000;
    }
    return (float)sample / 8388608.0f;  /* 2^23 */
}

/*
 * 将浮点转换为 24bit 整数
 */
__attribute__((section(".itcm.audio")))
static inline int32_t float_to_int24(float sample)
{
    /* 限幅 */
    if (sample > 1.0f) sample = 1.0f;
    if (sample < -1.0f) sample = -1.0f;
    
    return (int32_t)(sample * 8388607.0f);
}

/*
 * 解交织立体声数据
 */
__attribute__((section(".itcm.audio")))
void audio_deinterleave(const int32_t *input, float *left, float *right, uint32_t samples)
{
    for (uint32_t i = 0; i < samples; i++) {
        left[i] = int24_to_float(input[i * 2]);
        right[i] = int24_to_float(input[i * 2 + 1]);
    }
}

/*
 * 交织立体声数据
 */
__attribute__((section(".itcm.audio")))
void audio_interleave(const float *left, const float *right, int32_t *output, uint32_t samples)
{
    for (uint32_t i = 0; i < samples; i++) {
        output[i * 2] = float_to_int24(left[i]);
        output[i * 2 + 1] = float_to_int24(right[i]);
    }
}

/*
 * 应用增益
 */
__attribute__((section(".itcm.audio")))
void audio_apply_gain(float *buffer, uint32_t samples, float gain)
{
    arm_scale_f32(buffer, gain, buffer, samples);
}

/*
 * 低通滤波
 */
__attribute__((section(".itcm.audio")))
void audio_lowpass_filter(float *buffer, uint32_t samples)
{
    arm_biquad_cascade_df1_f32(&biquad_lowpass, buffer, buffer, samples);
}

/*
 * 高通滤波
 */
__attribute__((section(".itcm.audio")))
void audio_highpass_filter(float *buffer, uint32_t samples)
{
    arm_biquad_cascade_df1_f32(&biquad_highpass, buffer, buffer, samples);
}

/*
 * FIR 滤波
 */
__attribute__((section(".itcm.audio")))
void audio_fir_filter(float *input, float *output, uint32_t samples)
{
    arm_fir_f32(&fir_filter, input, output, samples);
}

/*
 * 软限幅器
 */
__attribute__((section(".itcm.audio")))
void audio_soft_limiter(float *buffer, uint32_t samples, float threshold)
{
    for (uint32_t i = 0; i < samples; i++) {
        float x = buffer[i];
        if (x > threshold) {
            buffer[i] = threshold + (1.0f - threshold) * tanhf((x - threshold) / (1.0f - threshold));
        } else if (x < -threshold) {
            buffer[i] = -threshold + (-1.0f + threshold) * tanhf((x + threshold) / (1.0f - threshold));
        }
    }
}

/*
 * 计算 FFT 频谱
 */
__attribute__((section(".itcm.audio")))
void audio_compute_spectrum(float *input, float *magnitude, uint32_t fft_size)
{
    extern float fft_output[];
    
    /* 执行 FFT */
    arm_rfft_fast_f32(&fft_instance, input, fft_output, 0);
    
    /* 计算幅度 */
    arm_cmplx_mag_f32(fft_output, magnitude, fft_size / 2);
}

/*
 * 完整的音频处理流水线
 */
__attribute__((section(".itcm.audio")))
void audio_process_frame(const int32_t *input, int32_t *output, uint32_t samples)
{
    /* 1. 解交织 */
    audio_deinterleave(input, process_buffer_l, process_buffer_r, samples);
    
    /* 2. 输入增益 */
    audio_apply_gain(process_buffer_l, samples, 1.0f);
    audio_apply_gain(process_buffer_r, samples, 1.0f);
    
    /* 3. 高通滤波 (去除直流) */
    audio_highpass_filter(process_buffer_l, samples);
    audio_highpass_filter(process_buffer_r, samples);
    
    /* 4. 低通滤波 (抗混叠) */
    audio_lowpass_filter(process_buffer_l, samples);
    audio_lowpass_filter(process_buffer_r, samples);
    
    /* 5. 软限幅 */
    audio_soft_limiter(process_buffer_l, samples, 0.9f);
    audio_soft_limiter(process_buffer_r, samples, 0.9f);
    
    /* 6. 交织输出 */
    audio_interleave(process_buffer_l, process_buffer_r, output, samples);
}
```

### 5. 音频处理主循环

```c
/* audio_main.c - 音频处理主程序 */
#include "audio_buffer.h"
#include "sai_audio.h"
#include "audio_dsp.h"

/* 处理标志 */
static volatile uint8_t rx_half_ready = 0;
static volatile uint8_t rx_full_ready = 0;

/* DMA 回调 */
static void on_rx_half_complete(void)
{
    rx_half_ready = 1;
}

static void on_rx_full_complete(void)
{
    rx_full_ready = 1;
}

static void on_tx_half_complete(void)
{
    /* 可用于同步 */
}

static void on_tx_full_complete(void)
{
    /* 可用于同步 */
}

/*
 * 音频系统初始化
 */
void audio_system_init(void)
{
    /* 初始化缓冲区系统 */
    audio_buffer_system_init();
    
    /* 初始化 DSP */
    audio_dsp_init();
    
    /* 初始化 SAI */
    SAI_Audio_Init();
    
    /* 注册回调 */
    SAI_Audio_RegisterCallbacks(
        on_rx_half_complete,
        on_rx_full_complete,
        on_tx_half_complete,
        on_tx_full_complete
    );
}

/*
 * 启动音频处理
 */
void audio_system_start(void)
{
    SAI_Audio_Start();
}

/*
 * 音频处理任务 (在主循环或 RTOS 任务中调用)
 */
void audio_process_task(void)
{
    /* 处理前半缓冲 */
    if (rx_half_ready) {
        rx_half_ready = 0;
        
        int32_t *rx_buf = (int32_t *)SAI_Audio_GetRxBuffer(0);
        int32_t *tx_buf = (int32_t *)SAI_Audio_GetTxBuffer(0);
        
        /* 处理音频帧 */
        audio_process_frame(rx_buf, tx_buf, 
                           AUDIO_FRAME_SIZE * AUDIO_DMA_BUFFER_FRAMES / 2);
    }
    
    /* 处理后半缓冲 */
    if (rx_full_ready) {
        rx_full_ready = 0;
        
        int32_t *rx_buf = (int32_t *)SAI_Audio_GetRxBuffer(1);
        int32_t *tx_buf = (int32_t *)SAI_Audio_GetTxBuffer(1);
        
        /* 处理音频帧 */
        audio_process_frame(rx_buf, tx_buf, 
                           AUDIO_FRAME_SIZE * AUDIO_DMA_BUFFER_FRAMES / 2);
    }
}
```

------

## 三、高速ADC采集配置

### 1. ADC系统架构

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                         高速ADC采集系统架构                              │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                        ADC 触发源                                │   │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐            │   │
│  │  │  TIM1   │  │  TIM2   │  │  TIM3   │  │ 外部触发 │            │   │
│  │  │ TRGO    │  │ TRGO    │  │ TRGO    │  │ EXTI    │            │   │
│  │  └────┬────┘  └────┬────┘  └────┬────┘  └────┬────┘            │   │
│  │       └────────────┴────────────┴────────────┘                  │   │
│  │                            │                                     │   │
│  └────────────────────────────┼─────────────────────────────────────┘   │
│                               ▼                                         │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                      ADC1/ADC2/ADC3                              │   │
│  │  ┌─────────────────────────────────────────────────────────┐    │   │
│  │  │ 模式: 独立 / 双ADC交替 / 三ADC交替                       │    │   │
│  │  │ 分辨率: 16bit / 14bit / 12bit / 10bit / 8bit            │    │   │
│  │  │ 采样率: 最高 3.6 MSPS (单ADC) / 7.2 MSPS (双ADC)        │    │   │
│  │  │ 过采样: 2x - 1024x                                       │    │   │
│  │  └─────────────────────────────────────────────────────────┘    │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                               │                                         │
│                               ▼                                         │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                      DMA 传输                                    │   │
│  │  ┌─────────────────────────────────────────────────────────┐    │   │
│  │  │ 模式: 循环 / 双缓冲                                      │    │   │
│  │  │ 目标: SRAM1 (D2域，非缓存)                               │    │   │
│  │  │ 中断: 半传输完成 / 传输完成                              │    │   │
│  │  └─────────────────────────────────────────────────────────┘    │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                               │                                         │
│                               ▼                                         │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                    数据处理流水线                                │   │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐            │   │
│  │  │ 数据    │─▶│ 滤波    │─▶│ FFT     │─▶│ 特征    │            │   │
│  │  │ 预处理  │  │ 处理    │  │ 分析    │  │ 提取    │            │   │
│  │  └─────────┘  └─────────┘  └─────────┘  └─────────┘            │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 2. ADC缓冲区定义

```c
/* adc_buffer.h */
#ifndef __ADC_BUFFER_H
#define __ADC_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

/* ADC 配置 */
#define ADC_SAMPLE_RATE         1000000     /* 1 MSPS */
#define ADC_RESOLUTION          16          /* 16位 */
#define ADC_CHANNELS            4           /* 4通道 */
#define ADC_DMA_BUFFER_SAMPLES  4096        /* DMA 缓冲采样数 */
#define ADC_RING_BUFFER_SAMPLES 16384       /* 环形缓冲采样数 */

/* 缓冲区大小计算 */
#define ADC_SAMPLE_SIZE         sizeof(uint16_t)
#define ADC_DMA_BUFFER_SIZE     (ADC_DMA_BUFFER_SAMPLES * ADC_CHANNELS * ADC_SAMPLE_SIZE)
#define ADC_RING_BUFFER_SIZE    (ADC_RING_BUFFER_SAMPLES * ADC_CHANNELS * ADC_SAMPLE_SIZE)

/* ADC 数据结构 */
typedef struct {
    uint16_t ch[ADC_CHANNELS];
} adc_sample_t;

/* ADC 环形缓冲区 */
typedef struct {
    adc_sample_t *buffer;
    uint32_t size;
    volatile uint32_t head;
    volatile uint32_t tail;
    volatile uint32_t count;
    volatile uint32_t overflow_count;
} adc_ringbuf_t;

/* 初始化 */
void adc_buffer_init(void);

/* 获取 DMA 缓冲区 */
uint16_t *adc_get_dma_buffer(void);

/* 环形缓冲区操作 */
bool adc_ringbuf_write_block(adc_ringbuf_t *rb, const adc_sample_t *data, uint32_t count);
bool adc_ringbuf_read_block(adc_ringbuf_t *rb, adc_sample_t *data, uint32_t count);
uint32_t adc_ringbuf_available(adc_ringbuf_t *rb);

#endif /* __ADC_BUFFER_H */
/* adc_buffer.c */
#include "adc_buffer.h"
#include <string.h>

/* ==================== DMA 缓冲区 - SRAM1 ==================== */

__attribute__((section(".adc_dma"), aligned(32)))
static uint16_t adc_dma_buffer[ADC_DMA_BUFFER_SAMPLES * ADC_
```





```
继续
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

```c
/* adc_buffer.c (续) */
#include "adc_buffer.h"
#include <string.h>

/* ==================== DMA 缓冲区 - SRAM1 ==================== */

__attribute__((section(".adc_dma"), aligned(32)))
static uint16_t adc_dma_buffer[ADC_DMA_BUFFER_SAMPLES * ADC_CHANNELS];

/* ==================== 环形缓冲区 - AXI SRAM ==================== */

__attribute__((section(".audio_buffer"), aligned(32)))
static adc_sample_t adc_ring_buffer[ADC_RING_BUFFER_SAMPLES];

/* ==================== 处理缓冲区 - DTCM ==================== */

__attribute__((section(".adc_cache")))
static float adc_process_buffer[ADC_DMA_BUFFER_SAMPLES];

__attribute__((section(".adc_cache")))
static float adc_filter_buffer[ADC_DMA_BUFFER_SAMPLES];

/* ==================== 状态结构 - DTCM ==================== */

__attribute__((section(".adc_cache")))
static adc_ringbuf_t adc_ringbuf;

/* ==================== 缓冲区实现 ==================== */

void adc_buffer_init(void)
{
    adc_ringbuf.buffer = adc_ring_buffer;
    adc_ringbuf.size = ADC_RING_BUFFER_SAMPLES;
    adc_ringbuf.head = 0;
    adc_ringbuf.tail = 0;
    adc_ringbuf.count = 0;
    adc_ringbuf.overflow_count = 0;
    
    memset(adc_dma_buffer, 0, sizeof(adc_dma_buffer));
    memset(adc_ring_buffer, 0, sizeof(adc_ring_buffer));
}

uint16_t *adc_get_dma_buffer(void)
{
    return adc_dma_buffer;
}

float *adc_get_process_buffer(void)
{
    return adc_process_buffer;
}

float *adc_get_filter_buffer(void)
{
    return adc_filter_buffer;
}

adc_ringbuf_t *adc_get_ringbuf(void)
{
    return &adc_ringbuf;
}

bool adc_ringbuf_write_block(adc_ringbuf_t *rb, const adc_sample_t *data, uint32_t count)
{
    if (rb->count + count > rb->size) {
        rb->overflow_count++;
        return false;
    }
    
    for (uint32_t i = 0; i < count; i++) {
        rb->buffer[rb->head] = data[i];
        rb->head = (rb->head + 1) % rb->size;
    }
    
    __atomic_fetch_add(&rb->count, count, __ATOMIC_SEQ_CST);
    return true;
}

bool adc_ringbuf_read_block(adc_ringbuf_t *rb, adc_sample_t *data, uint32_t count)
{
    if (rb->count < count) {
        return false;
    }
    
    for (uint32_t i = 0; i < count; i++) {
        data[i] = rb->buffer[rb->tail];
        rb->tail = (rb->tail + 1) % rb->size;
    }
    
    __atomic_fetch_sub(&rb->count, count, __ATOMIC_SEQ_CST);
    return true;
}

uint32_t adc_ringbuf_available(adc_ringbuf_t *rb)
{
    return rb->count;
}
```

### 3. 高速ADC驱动配置

```c
/* adc_driver.c - 高速ADC驱动 */
#include "stm32h7xx_hal.h"
#include "adc_buffer.h"

/* ADC 和 DMA 句柄 */
static ADC_HandleTypeDef hadc1;
static ADC_HandleTypeDef hadc2;
static DMA_HandleTypeDef hdma_adc1;
static TIM_HandleTypeDef htim1;

/* 回调函数 */
static void (*adc_half_callback)(void) = NULL;
static void (*adc_full_callback)(void) = NULL;

/*
 * 定时器触发配置 - 控制采样率
 */
static void ADC_Timer_Init(uint32_t sample_rate)
{
    uint32_t timer_clock = HAL_RCC_GetPCLK2Freq() * 2;  /* APB2 定时器时钟 */
    uint32_t prescaler = 0;
    uint32_t period = timer_clock / sample_rate - 1;
    
    /* 如果周期太大，使用预分频 */
    while (period > 65535) {
        prescaler++;
        period = timer_clock / ((prescaler + 1) * sample_rate) - 1;
    }
    
    htim1.Instance = TIM1;
    htim1.Init.Prescaler = prescaler;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = period;
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&htim1);
    
    /* 配置 TRGO 输出 */
    TIM_MasterConfigTypeDef master_config = {0};
    master_config.MasterOutputTrigger = TIM_TRGO_UPDATE;
    master_config.MasterOutputTrigger2 = TIM_TRGO2_RESET;
    master_config.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim1, &master_config);
}

/*
 * ADC1 初始化 - 主ADC
 */
static void ADC1_Init(void)
{
    ADC_MultiModeTypeDef multimode = {0};
    ADC_ChannelConfTypeDef channel_config = {0};
    
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV2;
    hadc1.Init.Resolution = ADC_RESOLUTION_16B;
    hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.NbrOfConversion = ADC_CHANNELS;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIG_T1_TRGO;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR;
    hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    hadc1.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
    hadc1.Init.OversamplingMode = DISABLE;
    
    HAL_ADC_Init(&hadc1);
    
    /* 配置多ADC模式 (如果使用双ADC) */
    multimode.Mode = ADC_MODE_INDEPENDENT;
    HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode);
    
    /* 配置通道 */
    channel_config.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
    channel_config.SingleDiff = ADC_SINGLE_ENDED;
    channel_config.OffsetNumber = ADC_OFFSET_NONE;
    channel_config.Offset = 0;
    
    /* 通道 0 - IN0 */
    channel_config.Channel = ADC_CHANNEL_0;
    channel_config.Rank = ADC_REGULAR_RANK_1;
    HAL_ADC_ConfigChannel(&hadc1, &channel_config);
    
    /* 通道 1 - IN1 */
    channel_config.Channel = ADC_CHANNEL_1;
    channel_config.Rank = ADC_REGULAR_RANK_2;
    HAL_ADC_ConfigChannel(&hadc1, &channel_config);
    
    /* 通道 2 - IN2 */
    channel_config.Channel = ADC_CHANNEL_2;
    channel_config.Rank = ADC_REGULAR_RANK_3;
    HAL_ADC_ConfigChannel(&hadc1, &channel_config);
    
    /* 通道 3 - IN3 */
    channel_config.Channel = ADC_CHANNEL_3;
    channel_config.Rank = ADC_REGULAR_RANK_4;
    HAL_ADC_ConfigChannel(&hadc1, &channel_config);
    
    /* ADC 校准 */
    HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
}

/*
 * DMA 配置
 */
static void ADC_DMA_Init(void)
{
    hdma_adc1.Instance = DMA1_Stream2;
    hdma_adc1.Init.Request = DMA_REQUEST_ADC1;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode = DMA_CIRCULAR;
    hdma_adc1.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    hdma_adc1.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    hdma_adc1.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    hdma_adc1.Init.MemBurst = DMA_MBURST_INC8;
    hdma_adc1.Init.PeriphBurst = DMA_PBURST_SINGLE;
    
    HAL_DMA_Init(&hdma_adc1);
    __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);
    
    /* DMA 中断 */
    HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);
}

/*
 * 完整初始化
 */
void ADC_HighSpeed_Init(uint32_t sample_rate)
{
    /* 初始化缓冲区 */
    adc_buffer_init();
    
    /* 初始化定时器 */
    ADC_Timer_Init(sample_rate);
    
    /* 初始化 ADC */
    ADC1_Init();
    
    /* 初始化 DMA */
    ADC_DMA_Init();
}

/*
 * 注册回调
 */
void ADC_HighSpeed_RegisterCallbacks(void (*half_cb)(void), void (*full_cb)(void))
{
    adc_half_callback = half_cb;
    adc_full_callback = full_cb;
}

/*
 * 启动采集
 */
void ADC_HighSpeed_Start(void)
{
    uint16_t *dma_buffer = adc_get_dma_buffer();
    
    /* 启动 DMA */
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)dma_buffer, 
                      ADC_DMA_BUFFER_SAMPLES * ADC_CHANNELS);
    
    /* 启动定时器触发 */
    HAL_TIM_Base_Start(&htim1);
}

/*
 * 停止采集
 */
void ADC_HighSpeed_Stop(void)
{
    HAL_TIM_Base_Stop(&htim1);
    HAL_ADC_Stop_DMA(&hadc1);
}

/*
 * 获取缓冲区指针
 */
uint16_t *ADC_HighSpeed_GetBuffer(uint8_t half)
{
    uint16_t *buffer = adc_get_dma_buffer();
    if (half == 0) {
        return buffer;
    } else {
        return buffer + (ADC_DMA_BUFFER_SAMPLES * ADC_CHANNELS / 2);
    }
}

/* ==================== 中断处理 ==================== */

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc == &hadc1 && adc_half_callback) {
        adc_half_callback();
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc == &hadc1 && adc_full_callback) {
        adc_full_callback();
    }
}

void DMA1_Stream2_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_adc1);
}
```

### 4. ADC数据处理 (ITCM优化)

```c
/* adc_process.c - ADC数据处理 */
#include "adc_buffer.h"
#include "arm_math.h"

/* ==================== 滤波器状态 - DTCM ==================== */

__attribute__((section(".adc_cache")))
static arm_biquad_casd_df1_inst_f32 adc_filter[ADC_CHANNELS];

__attribute__((section(".adc_cache")))
static float filter_state[ADC_CHANNELS][4 * 2];

/* ==================== FFT 实例 ==================== */

__attribute__((section(".adc_cache")))
static arm_rfft_fast_instance_f32 adc_fft;

/* ==================== 滤波器系数 ==================== */

__attribute__((section(".audio_coef")))
static const float adc_filter_coeffs[5 * 2] = {
    /* 低通滤波器系数 */
    0.0201f, 0.0402f, 0.0201f, -1.5610f, 0.6414f,
    0.0201f, 0.0402f, 0.0201f, -1.5610f, 0.6414f,
};

/* ==================== 统计数据 - DTCM ==================== */

__attribute__((section(".adc_cache")))
static struct {
    float min[ADC_CHANNELS];
    float max[ADC_CHANNELS];
    float mean[ADC_CHANNELS];
    float rms[ADC_CHANNELS];
    float peak_freq[ADC_CHANNELS];
} adc_stats;

/* ==================== 初始化 ==================== */

void adc_process_init(void)
{
    /* 初始化滤波器 */
    for (int i = 0; i < ADC_CHANNELS; i++) {
        arm_biquad_cascade_df1_init_f32(&adc_filter[i], 2,
                                        (float *)adc_filter_coeffs,
                                        filter_state[i]);
    }
    
    /* 初始化 FFT */
    arm_rfft_fast_init_f32(&adc_fft, ADC_DMA_BUFFER_SAMPLES / ADC_CHANNELS);
}

/* ==================== 处理函数 - ITCM ==================== */

/*
 * 将 ADC 原始值转换为电压
 */
__attribute__((section(".itcm.adc")))
static inline float adc_to_voltage(uint16_t raw)
{
    /* 假设 3.3V 参考电压，16位分辨率 */
    return (float)raw * 3.3f / 65535.0f;
}

/*
 * 解交织多通道数据
 */
__attribute__((section(".itcm.adc")))
void adc_deinterleave(const uint16_t *input, float *output[ADC_CHANNELS], 
                      uint32_t samples_per_channel)
{
    for (uint32_t i = 0; i < samples_per_channel; i++) {
        for (int ch = 0; ch < ADC_CHANNELS; ch++) {
            output[ch][i] = adc_to_voltage(input[i * ADC_CHANNELS + ch]);
        }
    }
}

/*
 * 应用数字滤波器
 */
__attribute__((section(".itcm.adc")))
void adc_apply_filter(float *data, uint32_t channel, uint32_t samples)
{
    arm_biquad_cascade_df1_f32(&adc_filter[channel], data, data, samples);
}

/*
 * 计算统计数据
 */
__attribute__((section(".itcm.adc")))
void adc_compute_stats(const float *data, uint32_t channel, uint32_t samples)
{
    float min_val, max_val, mean_val;
    uint32_t min_idx, max_idx;
    
    /* 最小值 */
    arm_min_f32(data, samples, &min_val, &min_idx);
    adc_stats.min[channel] = min_val;
    
    /* 最大值 */
    arm_max_f32(data, samples, &max_val, &max_idx);
    adc_stats.max[channel] = max_val;
    
    /* 平均值 */
    arm_mean_f32(data, samples, &mean_val);
    adc_stats.mean[channel] = mean_val;
    
    /* RMS */
    arm_rms_f32(data, samples, &adc_stats.rms[channel]);
}

/*
 * 计算 FFT 并找峰值频率
 */
__attribute__((section(".itcm.adc")))
void adc_compute_fft(float *data, uint32_t channel, uint32_t samples)
{
    extern float fft_output[];
    extern float fft_magnitude[];
    
    /* 执行 FFT */
    arm_rfft_fast_f32(&adc_fft, data, fft_output, 0);
    
    /* 计算幅度 */
    arm_cmplx_mag_f32(fft_output, fft_magnitude, samples / 2);
    
    /* 找峰值 */
    float max_val;
    uint32_t max_idx;
    arm_max_f32(fft_magnitude + 1, samples / 2 - 1, &max_val, &max_idx);  /* 跳过DC */
    
    /* 计算峰值频率 */
    float freq_resolution = (float)ADC_SAMPLE_RATE / samples;
    adc_stats.peak_freq[channel] = (max_idx + 1) * freq_resolution;
}

/*
 * 完整的 ADC 数据处理流水线
 */
__attribute__((section(".itcm.adc")))
void adc_process_block(const uint16_t *input, uint32_t total_samples)
{
    uint32_t samples_per_channel = total_samples / ADC_CHANNELS;
    float *process_buf = adc_get_process_buffer();
    float *filter_buf = adc_get_filter_buffer();
    
    /* 处理每个通道 */
    for (int ch = 0; ch < ADC_CHANNELS; ch++) {
        /* 提取单通道数据并转换 */
        for (uint32_t i = 0; i < samples_per_channel; i++) {
            process_buf[i] = adc_to_voltage(input[i * ADC_CHANNELS + ch]);
        }
        
        /* 滤波 */
        adc_apply_filter(process_buf, ch, samples_per_channel);
        
        /* 统计 */
        adc_compute_stats(process_buf, ch, samples_per_channel);
        
        /* FFT (可选) */
        if (ch == 0) {  /* 只对第一通道做 FFT */
            memcpy(filter_buf, process_buf, samples_per_channel * sizeof(float));
            adc_compute_fft(filter_buf, ch, samples_per_channel);
        }
    }
}

/*
 * 获取统计数据
 */
void adc_get_stats(uint32_t channel, float *min, float *max, 
                   float *mean, float *rms, float *peak_freq)
{
    if (channel >= ADC_CHANNELS) return;
    
    if (min) *min = adc_stats.min[channel];
    if (max) *max = adc_stats.max[channel];
    if (mean) *mean = adc_stats.mean[channel];
    if (rms) *rms = adc_stats.rms[channel];
    if (peak_freq) *peak_freq = adc_stats.peak_freq[channel];
}
```

### 5. 双ADC交替采样 (更高采样率)

```c
/* adc_dual.c - 双ADC交替采样配置 */
#include "stm32h7xx_hal.h"
#include "adc_buffer.h"

/* 双ADC模式可达到 7.2 MSPS */
#define DUAL_ADC_SAMPLE_RATE    2000000  /* 2 MSPS per ADC = 4 MSPS total */

static ADC_HandleTypeDef hadc1;
static ADC_HandleTypeDef hadc2;
static DMA_HandleTypeDef hdma_adc;

/* 双ADC DMA缓冲区 - 32位数据 (ADC1高16位 + ADC2低16位) */
__attribute__((section(".adc_dma"), aligned(32)))
static uint32_t dual_adc_buffer[ADC_DMA_BUFFER_SAMPLES];

/*
 * 双ADC初始化
 */
void ADC_Dual_Init(void)
{
    ADC_MultiModeTypeDef multimode = {0};
    ADC_ChannelConfTypeDef channel_config = {0};
    
    /* ==================== ADC1 配置 (主) ==================== */
    
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV2;
    hadc1.Init.Resolution = ADC_RESOLUTION_16B;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIG_T1_TRGO;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR;
    hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    hadc1.Init.OversamplingMode = DISABLE;
    
    HAL_ADC_Init(&hadc1);
    
    /* 配置双ADC模式 - 交替模式 */
    multimode.Mode = ADC_DUALMODE_INTERL;
    multimode.DualModeData = ADC_DUALMODEDATAFORMAT_32_10_BITS;
    multimode.TwoSamplingDelay = ADC_TWOSAMPLINGDELAY_1CYCLE;
    HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode);
    
    /* 配置通道 */
    channel_config.Channel = ADC_CHANNEL_0;
    channel_config.Rank = ADC_REGULAR_RANK_1;
    channel_config.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
    channel_config.SingleDiff = ADC_SINGLE_ENDED;
    channel_config.OffsetNumber = ADC_OFFSET_NONE;
    HAL_ADC_ConfigChannel(&hadc1, &channel_config);
    
    /* ==================== ADC2 配置 (从) ==================== */
    
    hadc2.Instance = ADC2;
    hadc2.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV2;
    hadc2.Init.Resolution = ADC_RESOLUTION_16B;
    hadc2.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc2.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc2.Init.LowPowerAutoWait = DISABLE;
    hadc2.Init.ContinuousConvMode = DISABLE;
    hadc2.Init.NbrOfConversion = 1;
    hadc2.Init.DiscontinuousConvMode = DISABLE;
    hadc2.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
    hadc2.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    hadc2.Init.OversamplingMode = DISABLE;
    
    HAL_ADC_Init(&hadc2);
    
    /* 配置通道 - 与ADC1相同 */
    channel_config.Channel = ADC_CHANNEL_0;
    channel_config.Rank = ADC_REGULAR_RANK_1;
    channel_config.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
    HAL_ADC_ConfigChannel(&hadc2, &channel_config);
    
    /* ==================== DMA 配置 ==================== */
    
    hdma_adc.Instance = DMA1_Stream2;
    hdma_adc.Init.Request = DMA_REQUEST_ADC1;
    hdma_adc.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_adc.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_adc.Init.Mode = DMA_CIRCULAR;
    hdma_adc.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    hdma_adc.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    hdma_adc.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    hdma_adc.Init.MemBurst = DMA_MBURST_INC4;
    hdma_adc.Init.PeriphBurst = DMA_PBURST_SINGLE;
    
    HAL_DMA_Init(&hdma_adc);
    __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc);
    
    /* ADC 校准 */
    HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
    HAL_ADCEx_Calibration_Start(&hadc2, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
}

/*
 * 启动双ADC采集
 */
void ADC_Dual_Start(void)
{
    /* 启动 ADC2 (从) */
    HAL_ADC_Start(&hadc2);
    
    /* 启动 ADC1 (主) + DMA */
    HAL_ADCEx_MultiModeStart_DMA(&hadc1, dual_adc_buffer, ADC_DMA_BUFFER_SAMPLES);
    
    /* 启动定时器触发 */
    HAL_TIM_Base_Start(&htim1);
}

/*
 * 解析双ADC数据
 */
__attribute__((section(".itcm.adc")))
void ADC_Dual_ParseData(const uint32_t *input, uint16_t *adc1_data, 
                        uint16_t *adc2_data, uint32_t samples)
{
    for (uint32_t i = 0; i < samples; i++) {
        /* ADC1 数据在高16位，ADC2 数据在低16位 */
        adc1_data[i * 2] = (input[i] >> 16) & 0xFFFF;
        adc2_data[i * 2 + 1] = input[i] & 0xFFFF;
    }
}
```

------

## 四、USB高速传输配置

### 1. USB系统架构

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                         USB 高速传输系统架构                             │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                      USB OTG HS 控制器                           │   │
│  │  ┌─────────────────────────────────────────────────────────┐    │   │
│  │  │ 速度: High Speed (480 Mbps) / Full Speed (12 Mbps)      │    │   │
│  │  │ PHY:  内部 FS PHY / 外部 ULPI HS PHY                    │    │   │
│  │  │ 端点: 9个 IN + 9个 OUT (可配置)                          │    │   │
│  │  │ FIFO: 4KB 共享 FIFO                                      │    │   │
│  │  └─────────────────────────────────────────────────────────┘    │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                               │                                         │
│                               ▼                                         │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                      DMA 传输 (D2域)                             │   │
│  │  ┌─────────────────────────────────────────────────────────┐    │   │
│  │  │ 缓冲区位置: SRAM2 (0x30020000)                          │    │   │
│  │  │ 对齐要求: 32字节 (Cache line)                           │    │   │
│  │  │ 传输模式: 双缓冲 / 环形缓冲                             │    │   │
│  │  └─────────────────────────────────────────────────────────┘    │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                               │                                         │
│                               ▼                                         │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                      USB 设备类                                  │   │
│  │  ┌───────────┐  ┌───────────┐  ┌───────────┐  ┌───────────┐    │   │
│  │  │   CDC     │  │   MSC     │  │   Audio   │  │  Custom   │    │   │
│  │  │  虚拟串口  │  │  大容量   │  │   音频    │  │  自定义   │    │   │
│  │  │  2 Mbps   │  │  存储     │  │  UAC 2.0  │  │  批量传输 │    │   │
│  │  └───────────┘  └───────────┘  └───────────┘  └───────────┘    │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  最大吞吐量:                                                            │
│  - Bulk 传输: ~40 MB/s (理论 60 MB/s)                                  │
│  - Isochronous: ~24 MB/s (音频/视频)                                   │
│  - Interrupt: ~24 KB/s                                                  │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 2. USB缓冲区定义

```c
/* usb_buffer.h */
#ifndef __USB_BUFFER_H
#define __USB_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

/* USB 配置 */
#define USB_HS_MAX_PACKET_SIZE      512     /* 高速模式最大包大小 */
#define USB_FS_MAX_PACKET_SIZE      64      /* 全速模式最大包大小 */

/* 端点缓冲区大小 */
#define USB_EP_BUFFER_SIZE          (USB_HS_MAX_PACKET_SIZE * 8)  /* 4KB per EP */
#define USB_TX_BUFFER_SIZE          (32 * 1024)  /* 32KB 发送缓冲 */
#define USB_RX_BUFFER_SIZE          (32 * 1024)  /* 32KB 接收缓冲 */

/* USB 环形缓冲区 */
typedef struct {
    uint8_t *buffer;
    uint32_t size;
    volatile uint32_t head;
    volatile uint32_t tail;
    volatile uint32_t count;
} usb_ringbuf_t;

/* 初始化 */
void usb_buffer_init(void);

/* 获取端点缓冲区 */
uint8_t *usb_get_ep_buffer(uint8_t ep_num, uint8_t is_in);

/* 环形缓冲区操作 */
usb_ringbuf_t *usb_get_tx_ringbuf(void);
usb_ringbuf_t *usb_get_rx_ringbuf(void);

bool usb_ringbuf_write(usb_ringbuf_t *rb, const uint8_t *data, uint32_t len);
bool usb_ringbuf_read(usb_ringbuf_t *rb, uint8_t *data, uint32_t len);
uint32_t usb_ringbuf_available(usb_ringbuf_t *rb);
uint32_t usb_ringbuf_free(usb_ringbuf_t *rb);

#endif /* __USB_BUFFER_H */
/* usb_buffer.c */
#include "usb_buffer.h"
#include <string.h>

/* ==================== USB 端点缓冲区 - SRAM2 ==================== */

/* 端点 0 (控制) */
__attribute__((section(".usb_ep_buffer"), aligned(32)))
static uint8_t usb_ep0_buffer[USB_HS_MAX_PACKET_SIZE];

/* 端点 1 IN (Bulk TX) */
__attribute__((section(".usb_ep_buffer"), aligned(32)))
static uint8_t usb_ep1_in_buffer[USB_EP_BUFFER_SIZE];

/* 端点 1 OUT (Bulk RX) */
__attribute__((section(".usb_ep_buffer"), aligned(32)))
static uint8_t usb_ep1_out_buffer[USB_EP_BUFFER_SIZE];

/* 端点 2 IN (Interrupt) */
__attribute__((section(".usb_ep_buffer"), aligned(32)))
static uint8_t usb_ep2_in_buffer[USB_HS_MAX_PACKET_SIZE];

/* 端点 3 IN/OUT (Isochronous - 音频) */
__attribute__((section(".usb_ep_buffer"), aligned(32)))
static uint8_t usb_ep3_in_buffer[USB_EP_BUFFER_SIZE];

__attribute__((section(".usb_ep_buffer"), aligned(32)))
static uint8_t usb_ep3_out_buffer[USB_EP_BUFFER_SIZE];

/* ==================== USB 数据缓冲区 - SRAM2 ==================== */

__attribute__((section(".usb_data"), aligned(32)))
static uint8_t usb_tx_buffer[USB_TX_BUFFER_SIZE];

__attribute__((section(".usb_data"), aligned(32)))
static uint8_t usb_rx_buffer[USB_RX_BUFFER_SIZE];

/* ==================== 环形缓冲区状态 - DTCM ==================== */

__attribute__((section(".usb_ctrl")))
static usb_ringbuf_t tx_ringbuf;

__attribute__((section(".usb_ctrl")))
static usb_ringbuf_t rx_ringbuf;

/* ==================== 实现 ==================== */

void usb_buffer_init(void)
{
    /* 初始化发送环形缓冲区 */
    tx_ringbuf.buffer = usb_tx_buffer;
    tx_ringbuf.size = USB_TX_BUFFER_SIZE;
    tx_ringbuf.head = 0;
    tx_ringbuf.tail = 0;
    tx_ringbuf.count = 0;
    
    /* 初始化接收环形缓冲区 */
    rx_ringbuf.buffer = usb_rx_buffer;
    rx_ringbuf.size = USB_RX_BUFFER_SIZE;
    rx_ringbuf.head = 0;
    rx_ringbuf.tail = 0;
    rx_ringbuf.count = 0;
    
    /* 清零缓冲区 */
    memset(usb_tx_buffer, 0, USB_TX_BUFFER_SIZE);
    memset(usb_rx_buffer, 0, USB_RX_BUFFER_SIZE);
}

uint8_t *usb_get_ep_buffer(uint8_t ep_num, uint8_t is_in)
{
    switch (ep_num) {
        case 0:
            return usb_ep0_buffer;
        case 1:
            return is_in ? usb_ep1_in_buffer : usb_ep1_out_buffer;
        case 2:
            return usb_ep2_in_buffer;
        case 3:
            return is_in ? usb_ep3_in_buffer : usb_ep3_out_buffer;
        default:
            return NULL;
    }
}

usb_ringbuf_t *usb_get_tx_ringbuf(void)
{
    return &tx_ringbuf;
}

usb_ringbuf_t *usb_get_rx_ringbuf(void)
{
    return &rx_ringbuf;
}

bool usb_ringbuf_write(usb_ringbuf_t *rb, const uint8_t *data, uint32_t len)
{
    if (rb->count + len > rb->size) {
        return false;
    }
    
    for (uint32_t i = 0; i < len; i++) {
        rb->buffer[rb->head] = data[i];
        rb->head = (rb->head + 1) % rb->size;
    }
    
    __atomic_fetch_add(&rb->count, len, __ATOMIC_SEQ_CST);
    return true;
}

bool usb_ringbuf_read(usb_ringbuf_t *rb, uint8_t *data, uint32_t len)
{
    if (rb->count < len) {
        return false;
    }
    
    for (uint32_t i = 0; i < len; i++) {
        data[i] = rb->buffer[rb->tail];
        rb->tail = (rb->tail + 1) % rb->size;
    }
    
    __atomic_fetch_sub(&rb->count, len, __ATOMIC_SEQ_CST);
    return true;
}

uint32_t usb_ringbuf_available(usb_ringbuf_t *rb)
{
    return rb->count;
}

uint32_t usb_ringbuf_free(usb_ringbuf_t *rb)
{
    return rb->size - rb->count;
}
```

### 3. USB高速设备配置

```c
/* usb_device.c - USB 高速设备配置 */
#include "stm32h7xx_hal.h"
#include "usb_buffer.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"

/* USB 设备句柄 */
USBD_HandleTypeDef hUsbDeviceHS;
PCD_HandleTypeDef hpcd_USB_OTG_HS;

/* 外部 ULPI PHY 引脚配置 */
static void USB_HS_PHY_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* 使能时钟 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();
    
    /* ULPI 引脚配置 */
    /* CLK: PA5 */
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OTG2_HS;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* D0: PA3 */
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* D1-D7, STP, DIR, NXT 配置... */
    /* (根据实际硬件配置) */
}

/*
 * USB OTG HS 初始化
 */
void USB_Device_Init(void)
{
    /* 初始化缓冲区 */
    usb_buffer_init();
    
    /* 初始化 PHY */
    USB_HS_PHY_Init();
    
    /* 使能 USB 时钟 */
    __HAL_RCC_USB_OTG_HS_CLK_ENABLE();
    __HAL_RCC_USB_OTG_HS_ULPI_CLK_ENABLE();
    
    /* 配置 USB */
    hpcd_USB_OTG_HS.Instance = USB_OTG_HS;
    hpcd_USB_OTG_HS.Init.dev_endpoints = 9;
    hpcd_USB_OTG_HS.Init.speed = PCD_SPEED_HIGH;
    hpcd_USB_OTG_HS.Init.dma_enable = ENABLE;
    hpcd_USB_OTG_HS.Init.phy_itface = USB_OTG_ULPI_PHY;
    hpcd_USB_OTG_HS.Init.Sof_enable = DISABLE;
    hpcd_USB_OTG_HS.Init.low_power_enable = DISABLE;
    hpcd_USB_OTG_HS.Init.lpm_enable = DISABLE;
    hpcd_USB_OTG_HS.Init.vbus_sensing_enable = ENABLE;
    hpcd_USB_OTG_HS.Init.use_dedicated_ep1 = DISABLE;
    hpcd_USB_OTG_HS.Init.use_external_vbus = DISABLE;
    
    HAL_PCD_Init(&hpcd_USB_OTG_HS);
    
    /* 配置 FIFO */
    /* RX FIFO */
    HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_HS, 0x200);  /* 512 words = 2KB */
    
    /* TX FIFOs */
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 0, 0x40);   /* EP0: 64 words */
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 1, 0x100);  /* EP1: 256 words */
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 2, 0x20);   /* EP2: 32 words */
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 3, 0x100);  /* EP3: 256 words */
    
    /* 中断配置 */
    HAL_NVIC_SetPriority(OTG_HS_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(OTG_HS_IRQn);
    
    /* 初始化 USB 设备库 */
    USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS);
    USBD_RegisterClass(&hUsbDeviceHS, &USBD_CDC);
    USBD_CDC_RegisterInterface(&hUsbDeviceHS, &USBD_Interface_fops_HS);
    USBD_Start(&hUsbDeviceHS);
}

/* USB 中断处理 */
void OTG_HS_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&hpcd_USB_OTG_HS);
}
```

### 4. USB高速批量传输实现

```c
/* usb_bulk_transfer.c - USB 高速批量传输 */
#include "usb_buffer.h"
#include "usbd_core.h"
#include "cache_manager.h"

/* 传输状态 */
typedef struct {
    volatile uint8_t tx_busy;
    volatile uint8_t rx_ready;
    volatile uint32_t tx_count;
    volatile uint32_t rx_count;
    volatile uint32_t tx_total;
    volatile uint32_t rx_total;
} usb_transfer_state_t;

__attribute__((section(".usb_ctrl")))
static usb_transfer_state_t transfer_state;

/* 端点号定义 */
#define USB_BULK_IN_EP      0x81
#define USB_BULK_OUT_EP     0x01

/*
 * 初始化批量传输
 */
void USB_Bulk_Init(void)
{
    memset(&transfer_state, 0, sizeof(transfer_state));
}

/*
 * 启动批量发送
 */
__attribute__((section(".itcm.usb")))
int USB_Bulk_Transmit(USBD_HandleTypeDef *pdev, uint8_t *data, uint32_t len)
{
    usb_ringbuf_t *tx_rb = usb_get_tx_ringbuf();
    
    /* 检查空间 */
    if (usb_ringbuf_free(tx_rb) < len) {
        return -1;
    }
    
    /* 写入环形缓冲区 */
    usb_ringbuf_write(tx_rb, data, len);
    
    /* 如果当前没有传输，启动传输 */
    if (!transfer_state.tx_busy) {
        USB_Bulk_StartTx(pdev);
    }
    
    return 0;
}

/*
 * 启动发送 (从环形缓冲区)
 */
__attribute__((section(".itcm.usb")))
void USB_Bulk_StartTx(USBD_HandleTypeDef *pdev)
{
    usb_ringbuf_t *tx_rb = usb_get_tx_ringbuf();
    uint8_t *ep_buffer = usb_get_ep_buffer(1, 1);
    uint32_t available = usb_ringbuf_available(tx_rb);
    
    if (available == 0) {
        transfer_state.tx_busy = 0;
        return;
    }
    
    /* 限制单次传输大小 */
    uint32_t tx_len = (available > USB_HS_MAX_PACKET_SIZE) ? 
                       USB_HS_MAX_PACKET_SIZE : available;
    
    /* 从环形缓冲区读取到端点缓冲区 */
    usb_ringbuf_read(tx_rb, ep_buffer, tx_len);
    
    /* SRAM2 配置为非缓存，无需 Cache 操作 */
    /* 如果配置为可缓存，需要: cache_prepare_dma_tx(ep_buffer, tx_len); */
    
    transfer_state.tx_busy = 1;
    transfer_state.tx_count = tx_len;
    
    /* 启动 USB 传输 */
    USBD_LL_Transmit(pdev, USB_BULK_IN_EP, ep_buffer, tx_len);
}

/*
 * 发送完成回调
 */
__attribute__((section(".itcm.usb")))
void USB_Bulk_TxComplete(USBD_HandleTypeDef *pdev)
{
    transfer_state.tx_total += transfer_state.tx_count;
    transfer_state.tx_busy = 0;
    
    /* 继续发送剩余数据 */
    USB_Bulk_StartTx(pdev);
}

/*
 * 准备接收
 */
__attribute__((section(".itcm.usb")))
void USB_Bulk_PrepareRx(USBD_HandleTypeDef *pdev)
{
    uint8_t *ep_buffer = usb_get_ep_buffer(1, 0);
    
    /* 准备接收 */
    USBD_LL_PrepareReceive(pdev, USB_BULK_OUT_EP, ep_buffer, USB_HS_MAX_PACKET_SIZE);
}

/*
 * 接收完成回调
 */
__attribute__((section(".itcm.usb")))
void USB_Bulk_RxComplete(USBD_HandleTypeDef *pdev, uint32_t len)
{
    usb_ringbuf_t *rx_rb = usb_get_rx_ringbuf();
    uint8_t *ep_buffer = usb_get_ep_buffer(1, 0);
    
    /* SRAM2 配置为非缓存，无需 Cache 操作 */
    /* 如果配置为可缓存，需要: cache_complete_dma_rx(ep_buffer, len); */
    
    /* 写入环形缓冲区 */
    if (usb_ringbuf_free(rx_rb) >= len) {
        usb_ringbuf_write(rx_rb, ep_buffer, len);
        transfer_state.rx_total += len;
    }
    
    transfer_state.rx_ready = 1;
    transfer_state.rx_count = len;
    
    /* 准备下一次接收 */
    USB_Bulk_PrepareRx(pdev);
}

/*
 * 读取接收数据
 */
int USB_Bulk_Read(uint8_t *data, uint32_t max_len)
{
    usb_ringbuf_t *rx_rb = usb_get_rx_ringbuf();
    uint32_t available = usb_ringbuf_available(rx_rb);
    
    if (available == 0) {
        return 0;
    }
    
    uint32_t read_len = (available > max_len) ? max_len : available;
    usb_ringbuf_read(rx_rb, data, read_len);
    
    return read_len;
}

/*
 * 获取传输统计
 */
void USB_Bulk_GetStats(uint32_t *tx_total, uint32_t *rx_total)
{
    if (tx_total) *tx_total = transfer_state.tx_total;
    if (rx_total) *rx_total = transfer_state.rx_total;
}
```

### 5. USB Audio Class (UAC 2.0) 配置

```c
/* usb_audio.c - USB Audio Class 2.0 */
#include "usb_buffer.h"
#include "usbd_audio.h"
#include "audio_buffer.h"

/* UAC 2.0 配置 */
#define USB_AUDIO_SAMPLE_RATE       48000
#define USB_AUDIO_BIT_DEPTH         24
#define USB_AUDIO_CHANNELS          2
#define USB_AUDIO_PACKET_SIZE       ((USB_AUDIO_SAMPLE_RATE / 1000) * \
                                     USB_AUDIO_CHANNELS * \
                                     (USB_AUDIO_BIT_DEPTH / 8))

/* 音频端点缓冲区 */
__attribute__((section(".usb_ep_buffer"), aligned(32)))
static uint8_t usb_audio_out_buffer[USB_AUDIO_PACKET_SIZE * 4];

__attribute__((section(".usb_ep_buffer"), aligned(32)))
static uint8_t usb_audio_in_buffer[USB_AUDIO_PACKET_SIZE * 4];

/* 音频状态 */
__attribute__((section(".usb_ctrl")))
static struct {
    volatile uint8_t playing;
    volatile uint8_t recording;
    volatile uint32_t out_wr_ptr;
    volatile uint32_t out_rd_ptr;
    volatile uint32_t in_wr_ptr;
    volatile uint32_t in_rd_ptr;
    volatile int32_t feedback;
} usb_audio_state;

/*
 * USB 音频初始化
 */
void USB_Audio_Init(void)
{
    memset(&usb_audio_state, 0, sizeof(usb_audio_state));
    
    /* 初始化反馈值 (10.14 格式) */
    usb_audio_state.feedback = (USB_AUDIO_SAMPLE_RATE / 1000) << 14;
}

/*
 * 音频输出数据接收 (主机 -> 设备)
 */
__attribute__((section(".itcm.usb")))
void USB_Audio_DataOut(uint8_t *data, uint32_t len)
{
    audio_ringbuf_t *output_rb = audio_get_output_ringbuf();
    
    /* 转换 USB 数据格式到内部格式 */
    uint32_t samples = len / (USB_AUDIO_CHANNELS * (USB_AUDIO_BIT_DEPTH / 8));
    
    audio_frame_t frame;
    uint8_t *ptr = data;
    
    for (uint32_t i = 0; i < samples; i += AUDIO_FRAME_SIZE) {
        uint32_t frame_samples = (samples - i > AUDIO_FRAME_SIZE) ? 
                                  AUDIO_FRAME_SIZE : (samples - i);
        
        for (uint32_t j = 0; j < frame_samples; j++) {
            /* 24位数据，小端格式 */
            int32_t left = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16);
            if (left & 0x800000) left |= 0xFF000000;  /* 符号扩展 */
            ptr += 3;
            
            int32_t right = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16);
            if (right & 0x800000) right |= 0xFF000000;
            ptr += 3;
            
            frame.samples[j].left = left;
            frame.samples[j].right = right;
        }
        
        frame.timestamp = HAL_GetTick();
        frame.flags = 0;
        
        audio_ringbuf_write(output_rb, &frame);
    }
    
    usb_audio_state.playing = 1;
}

/*
 * 音频输入数据发送 (设备 -> 主机)
 */
__attribute__((section(".itcm.usb")))
uint32_t USB_Audio_DataIn(uint8_t *data, uint32_t max_len)
{
    audio_ringbuf_t *input_rb = audio_get_input_ringbuf();
    
    if (audio_ringbuf_available(input_rb) == 0) {
        /* 无数据，发送静音 */
        memset(data, 0, USB_AUDIO_PACKET_SIZE);
        return USB_AUDIO_PACKET_SIZE;
    }
    
    audio_frame_t frame;
    uint8_t *ptr = data;
    uint32_t total_bytes = 0;
    
    while (audio_ringbuf_read(input_rb, &frame) && 
           total_bytes + AUDIO_FRAME_SIZE * 6 <= max_len) {
        
        for (uint32_t i = 0; i < AUDIO_FRAME_SIZE; i++) {
            /* 转换为 24位小端格式 */
            int32_t left = frame.samples[i].left;
            int32_t right = frame.samples[i].right;
            
            *ptr++ = left & 0xFF;
            *ptr++ = (left >> 8) & 0xFF;
            *ptr++ = (left >> 16) & 0xFF;
            
            *ptr++ = right & 0xFF;
            *ptr++ = (right >> 8) & 0xFF;
            *ptr++ = (right >> 16) & 0xFF;
            
            total_bytes += 6;
        }
    }
    
    return total_bytes;
}

/*
 * 计算反馈值 (用于同步)
 */
__attribute__((section(".itcm.usb")))
uint32_t USB_Audio_GetFeedback(void)
{
    audio_ringbuf_t *output_rb = audio_get_output_ringbuf();
    uint32_t available = audio_ringbuf_available(output_rb);
    uint32_t target = AUDIO_RING_BUFFER_FRAMES / 2;
    
    /* 根据缓冲区填充程度调整反馈值 */
    int32_t error = (int32_t)available - (int32_t)target;
    
    /* PID 控制 */
    static int32_t integral = 0;
    integral += error;
    
    /* 限制积分 */
    if (integral > 1000) integral = 1000;
    if (integral < -1000) integral = -1000;
    
    /* 计算反馈值 */
    int32_t feedback = (USB_AUDIO_SAMPLE_RATE / 1000) << 14;
    feedback += error * 16 + integral;
    
    usb_audio_state.feedback = feedback;
    
    return (uint32_t)feedback;
}

/*
 * 同步回调 (SOF)
 */
void USB_Audio_SOF(void)
{
    /* 每 SOF (1ms) 更新反馈值 */
    USB_Audio_GetFeedback();
}
```

------

## 五、综合系统集成

### 1. MPU 配置 (所有场景)

```c
/* mpu_config.c - 综合 MPU 配置 */
#include "stm32h7xx.h"

void MPU_Config_AudioAdcUsb(void)
{
    MPU_Region_InitTypeDef MPU_InitStruct = {0};
    
    HAL_MPU_Disable();
    
    /* Region 0: Flash - 可缓存，写透 */
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER0;
    MPU
```





```
继续
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

```c
/* mpu_config.c - 综合 MPU 配置 (续) */

void MPU_Config_AudioAdcUsb(void)
{
    MPU_Region_InitTypeDef MPU_InitStruct = {0};
    
    HAL_MPU_Disable();
    
    /* ==================== Region 0: Flash ==================== */
    /* 可缓存，写透，可执行 */
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER0;
    MPU_InitStruct.BaseAddress = 0x08000000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_2MB;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStruct.AccessPermission = MPU_REGION_PRIV_RO_URO;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    
    /* ==================== Region 1: ITCM RAM ==================== */
    /* 不可缓存 (直连CPU)，可执行 - 关键代码 */
    MPU_InitStruct.Number = MPU_REGION_NUMBER1;
    MPU_InitStruct.BaseAddress = 0x00000000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_64KB;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    
    /* ==================== Region 2: DTCM RAM ==================== */
    /* 不可缓存 (直连CPU)，不可执行 - 栈/堆/关键数据 */
    MPU_InitStruct.Number = MPU_REGION_NUMBER2;
    MPU_InitStruct.BaseAddress = 0x20000000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_128KB;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    
    /* ==================== Region 3: AXI SRAM ==================== */
    /* 可缓存，写回+写分配 - 大缓冲区/帧缓冲 */
    MPU_InitStruct.Number = MPU_REGION_NUMBER3;
    MPU_InitStruct.BaseAddress = 0x24000000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    
    /* ==================== Region 4: SRAM1 ==================== */
    /* 不可缓存 - SAI/ADC DMA 缓冲区 */
    MPU_InitStruct.Number = MPU_REGION_NUMBER4;
    MPU_InitStruct.BaseAddress = 0x30000000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_128KB;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    
    /* ==================== Region 5: SRAM2 ==================== */
    /* 不可缓存 - USB DMA 缓冲区 */
    MPU_InitStruct.Number = MPU_REGION_NUMBER5;
    MPU_InitStruct.BaseAddress = 0x30020000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_128KB;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    
    /* ==================== Region 6: SRAM3 ==================== */
    /* 不可缓存 - BDMA 缓冲区 */
    MPU_InitStruct.Number = MPU_REGION_NUMBER6;
    MPU_InitStruct.BaseAddress = 0x30040000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_32KB;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    
    /* ==================== Region 7: SRAM4 + 外设 ==================== */
    /* Device 类型 - 低功耗SRAM和外设 */
    MPU_InitStruct.Number = MPU_REGION_NUMBER7;
    MPU_InitStruct.BaseAddress = 0x38000000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_64KB;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    
    /* 启用 MPU */
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}
```

### 2. 系统初始化和主循环

```c
/* main.c - 综合系统主程序 */
#include "stm32h7xx_hal.h"
#include "audio_buffer.h"
#include "adc_buffer.h"
#include "usb_buffer.h"
#include "cache_manager.h"

/* 系统状态 */
typedef struct {
    uint8_t audio_running;
    uint8_t adc_running;
    uint8_t usb_connected;
    uint32_t audio_underruns;
    uint32_t audio_overruns;
    uint32_t adc_overflows;
    uint32_t usb_errors;
} system_state_t;

static system_state_t sys_state;

/* 外部函数声明 */
extern void SystemClock_Config(void);
extern void MPU_Config_AudioAdcUsb(void);
extern void audio_system_init(void);
extern void audio_system_start(void);
extern void audio_process_task(void);
extern void ADC_HighSpeed_Init(uint32_t sample_rate);
extern void ADC_HighSpeed_Start(void);
extern void USB_Device_Init(void);

/*
 * 系统时钟配置 - 480MHz
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    
    /* 配置电源 */
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
    
    /* 配置 HSE 和 PLL */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 5;   /* 25MHz / 5 = 5MHz */
    RCC_OscInitStruct.PLL.PLLN = 192; /* 5MHz * 192 = 960MHz */
    RCC_OscInitStruct.PLL.PLLP = 2;   /* 960MHz / 2 = 480MHz */
    RCC_OscInitStruct.PLL.PLLQ = 20;  /* 960MHz / 20 = 48MHz (USB) */
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    
    /* 配置系统时钟 */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |
                                  RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
    
    /* 配置外设时钟 */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SAI1 | 
                                               RCC_PERIPHCLK_ADC |
                                               RCC_PERIPHCLK_USB;
    
    /* SAI 时钟 - PLL2 */
    PeriphClkInitStruct.PLL2.PLL2M = 5;
    PeriphClkInitStruct.PLL2.PLL2N = 192;
    PeriphClkInitStruct.PLL2.PLL2P = 5;   /* 192MHz for SAI */
    PeriphClkInitStruct.PLL2.PLL2Q = 2;
    PeriphClkInitStruct.PLL2.PLL2R = 2;
    PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_2;
    PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
    PeriphClkInitStruct.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLL2;
    
    /* ADC 时钟 - PLL2 */
    PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL2;
    
    /* USB 时钟 - PLL1Q (48MHz) */
    PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_PLL;
    
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
}

/*
 * 错误处理
 */
void Error_Handler(void)
{
    __disable_irq();
    while (1) {
        /* 可以添加 LED 闪烁指示 */
    }
}

/*
 * 主函数
 */
int main(void)
{
    /* 复位所有外设 */
    HAL_Init();
    
    /* 配置系统时钟 */
    SystemClock_Config();
    
    /* 配置 MPU */
    MPU_Config_AudioAdcUsb();
    
    /* 启用 Cache */
    SCB_EnableICache();
    SCB_EnableDCache();
    
    /* 初始化系统状态 */
    memset(&sys_state, 0, sizeof(sys_state));
    
    /* ==================== 初始化各子系统 ==================== */
    
    /* 音频系统初始化 */
    audio_system_init();
    
    /* ADC 系统初始化 */
    ADC_HighSpeed_Init(ADC_SAMPLE_RATE);
    
    /* USB 设备初始化 */
    USB_Device_Init();
    
    /* ==================== 启动各子系统 ==================== */
    
    /* 启动音频处理 */
    audio_system_start();
    sys_state.audio_running = 1;
    
    /* 启动 ADC 采集 */
    ADC_HighSpeed_Start();
    sys_state.adc_running = 1;
    
    /* ==================== 主循环 ==================== */
    
    while (1) {
        /* 音频处理任务 */
        audio_process_task();
        
        /* ADC 数据处理任务 */
        adc_process_task();
        
        /* USB 数据处理任务 */
        usb_process_task();
        
        /* 系统监控 */
        system_monitor_task();
    }
}

/*
 * ADC 处理任务
 */
void adc_process_task(void)
{
    extern volatile uint8_t adc_half_ready;
    extern volatile uint8_t adc_full_ready;
    
    if (adc_half_ready) {
        adc_half_ready = 0;
        uint16_t *buffer = ADC_HighSpeed_GetBuffer(0);
        adc_process_block(buffer, ADC_DMA_BUFFER_SAMPLES * ADC_CHANNELS / 2);
    }
    
    if (adc_full_ready) {
        adc_full_ready = 0;
        uint16_t *buffer = ADC_HighSpeed_GetBuffer(1);
        adc_process_block(buffer, ADC_DMA_BUFFER_SAMPLES * ADC_CHANNELS / 2);
    }
}

/*
 * USB 处理任务
 */
void usb_process_task(void)
{
    /* 检查 USB 连接状态 */
    extern USBD_HandleTypeDef hUsbDeviceHS;
    sys_state.usb_connected = (hUsbDeviceHS.dev_state == USBD_STATE_CONFIGURED);
    
    if (sys_state.usb_connected) {
        /* 处理 USB 数据传输 */
        /* 具体实现取决于 USB 设备类 */
    }
}

/*
 * 系统监控任务
 */
void system_monitor_task(void)
{
    static uint32_t last_tick = 0;
    uint32_t current_tick = HAL_GetTick();
    
    /* 每秒执行一次 */
    if (current_tick - last_tick >= 1000) {
        last_tick = current_tick;
        
        /* 检查音频缓冲区状态 */
        audio_ringbuf_t *input_rb = audio_get_input_ringbuf();
        audio_ringbuf_t *output_rb = audio_get_output_ringbuf();
        
        if (audio_ringbuf_available(output_rb) == 0 && sys_state.audio_running) {
            sys_state.audio_underruns++;
        }
        
        if (audio_ringbuf_free(input_rb) == 0) {
            sys_state.audio_overruns++;
        }
        
        /* 检查 ADC 缓冲区状态 */
        adc_ringbuf_t *adc_rb = adc_get_ringbuf();
        if (adc_rb->overflow_count > 0) {
            sys_state.adc_overflows += adc_rb->overflow_count;
            adc_rb->overflow_count = 0;
        }
        
        /* 可以添加调试输出 */
        #ifdef DEBUG
        printf("Audio: underruns=%lu, overruns=%lu\n", 
               sys_state.audio_underruns, sys_state.audio_overruns);
        printf("ADC: overflows=%lu\n", sys_state.adc_overflows);
        printf("USB: connected=%d, errors=%lu\n", 
               sys_state.usb_connected, sys_state.usb_errors);
        #endif
    }
}
```

### 3. FreeRTOS 集成版本

```c
/* main_freertos.c - FreeRTOS 版本 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "stm32h7xx_hal.h"

/* 任务优先级 */
#define AUDIO_TASK_PRIORITY     (configMAX_PRIORITIES - 1)  /* 最高 */
#define ADC_TASK_PRIORITY       (configMAX_PRIORITIES - 2)
#define USB_TASK_PRIORITY       (configMAX_PRIORITIES - 3)
#define MONITOR_TASK_PRIORITY   (tskIDLE_PRIORITY + 1)

/* 任务栈大小 */
#define AUDIO_TASK_STACK_SIZE   1024
#define ADC_TASK_STACK_SIZE     1024
#define USB_TASK_STACK_SIZE     1024
#define MONITOR_TASK_STACK_SIZE 512

/* 任务句柄 */
static TaskHandle_t audioTaskHandle;
static TaskHandle_t adcTaskHandle;
static TaskHandle_t usbTaskHandle;
static TaskHandle_t monitorTaskHandle;

/* 信号量 */
static SemaphoreHandle_t audioSemaphore;
static SemaphoreHandle_t adcSemaphore;

/* 任务栈 - 放在 DTCM */
__attribute__((section(".dtcm_noinit")))
static StackType_t audioTaskStack[AUDIO_TASK_STACK_SIZE];

__attribute__((section(".dtcm_noinit")))
static StackType_t adcTaskStack[ADC_TASK_STACK_SIZE];

__attribute__((section(".dtcm_noinit")))
static StackType_t usbTaskStack[USB_TASK_STACK_SIZE];

__attribute__((section(".dtcm_noinit")))
static StackType_t monitorTaskStack[MONITOR_TASK_STACK_SIZE];

/* 任务控制块 */
static StaticTask_t audioTaskTCB;
static StaticTask_t adcTaskTCB;
static StaticTask_t usbTaskTCB;
static StaticTask_t monitorTaskTCB;

/* 信号量缓冲区 */
static StaticSemaphore_t audioSemaphoreBuffer;
static StaticSemaphore_t adcSemaphoreBuffer;

/*
 * 音频处理任务
 */
void AudioTask(void *pvParameters)
{
    (void)pvParameters;
    
    while (1) {
        /* 等待 DMA 中断信号 */
        if (xSemaphoreTake(audioSemaphore, portMAX_DELAY) == pdTRUE) {
            /* 处理音频数据 */
            audio_process_task();
        }
    }
}

/*
 * ADC 处理任务
 */
void ADCTask(void *pvParameters)
{
    (void)pvParameters;
    
    while (1) {
        /* 等待 DMA 中断信号 */
        if (xSemaphoreTake(adcSemaphore, portMAX_DELAY) == pdTRUE) {
            /* 处理 ADC 数据 */
            adc_process_task();
        }
    }
}

/*
 * USB 处理任务
 */
void USBTask(void *pvParameters)
{
    (void)pvParameters;
    
    while (1) {
        /* USB 数据处理 */
        usb_process_task();
        
        /* 让出 CPU */
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/*
 * 系统监控任务
 */
void MonitorTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (1) {
        /* 每秒执行一次 */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
        
        /* 系统监控 */
        system_monitor_task();
        
        /* 打印任务状态 */
        #ifdef DEBUG
        printf("Audio task: %lu%% CPU\n", 
               (100 * audioTaskHandle->ulRunTimeCounter) / portGET_RUN_TIME_COUNTER_VALUE());
        printf("Free heap: %lu bytes\n", xPortGetFreeHeapSize());
        #endif
    }
}

/*
 * DMA 中断中释放信号量
 */
void Audio_DMA_Callback(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(audioSemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void ADC_DMA_Callback(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(adcSemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*
 * 主函数
 */
int main(void)
{
    /* 硬件初始化 */
    HAL_Init();
    SystemClock_Config();
    MPU_Config_AudioAdcUsb();
    SCB_EnableICache();
    SCB_EnableDCache();
    
    /* 初始化各子系统 */
    audio_system_init();
    ADC_HighSpeed_Init(ADC_SAMPLE_RATE);
    USB_Device_Init();
    
    /* 创建信号量 */
    audioSemaphore = xSemaphoreCreateBinaryStatic(&audioSemaphoreBuffer);
    adcSemaphore = xSemaphoreCreateBinaryStatic(&adcSemaphoreBuffer);
    
    /* 创建任务 */
    audioTaskHandle = xTaskCreateStatic(
        AudioTask,
        "Audio",
        AUDIO_TASK_STACK_SIZE,
        NULL,
        AUDIO_TASK_PRIORITY,
        audioTaskStack,
        &audioTaskTCB
    );
    
    adcTaskHandle = xTaskCreateStatic(
        ADCTask,
        "ADC",
        ADC_TASK_STACK_SIZE,
        NULL,
        ADC_TASK_PRIORITY,
        adcTaskStack,
        &adcTaskTCB
    );
    
    usbTaskHandle = xTaskCreateStatic(
        USBTask,
        "USB",
        USB_TASK_STACK_SIZE,
        NULL,
        USB_TASK_PRIORITY,
        usbTaskStack,
        &usbTaskTCB
    );
    
    monitorTaskHandle = xTaskCreateStatic(
        MonitorTask,
        "Monitor",
        MONITOR_TASK_STACK_SIZE,
        NULL,
        MONITOR_TASK_PRIORITY,
        monitorTaskStack,
        &monitorTaskTCB
    );
    
    /* 启动各子系统 */
    audio_system_start();
    ADC_HighSpeed_Start();
    
    /* 启动调度器 */
    vTaskStartScheduler();
    
    /* 不应该到达这里 */
    while (1) {}
}

/*
 * FreeRTOS 内存分配 - 使用 DTCM
 */
__attribute__((section(".dtcm_noinit")))
static uint8_t ucHeap[configTOTAL_HEAP_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];
    
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];
    
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
```

------

## 六、KEIL 和 IAR 配置

### 1. KEIL 分散加载文件

```text
; stm32h743_audio_adc_usb.sct

LR_IROM1 0x08000000 0x00200000 {

    ; ==================== Flash 代码区 ====================
    ER_IROM1 0x08000000 0x00200000 {
        *.o (RESET, +First)
        *(InRoot$$Sections)
        .ANY (+RO)
    }
    
    ; ==================== ITCM - 关键代码 ====================
    ER_ITCM 0x00000000 0x00010000 {
        ; 音频 DSP 代码
        audio_dsp.o (+RO)
        *arm_*.o (+RO)
        
        ; ADC 处理代码
        adc_process.o (+RO)
        
        ; USB 中断处理
        usb_bulk_transfer.o (+RO)
        
        ; 中断处理
        stm32h7xx_it.o (+RO)
        
        ; 自定义 ITCM 段
        *(ITCM)
        *(.itcm*)
    }
    
    ; ==================== DTCM - 主数据区 ====================
    RW_DTCM 0x20000000 0x00020000 {
        .ANY (+RW +ZI)
        
        ; 音频状态
        *(AUDIO_STATE)
        *(.audio_state)
        *(.biquad_state)
        *(.fir_state)
        
        ; ADC 缓存
        *(ADC_CACHE)
        *(.adc_cache)
        
        ; USB 控制结构
        *(USB_CTRL)
        *(.usb_ctrl)
    }
    
    ; ==================== AXI SRAM - 大缓冲区 ====================
    RW_AXIRAM 0x24000000 0x00080000 {
        ; 音频环形缓冲区
        *(AUDIO_BUFFER)
        *(.audio_buffer)
        *(.audio_ringbuf)
        *(.fft_buffer)
        
        ; 帧缓冲
        *(FRAMEBUFFER)
        *(.framebuffer)
    }
    
    ; ==================== SRAM1 - SAI/ADC DMA ====================
    RW_SRAM1 0x30000000 UNINIT 0x00020000 {
        ; SAI DMA 缓冲区
        *(SAI_DMA)
        *(.sai_dma)
        *(.sai_tx_buffer)
        *(.sai_rx_buffer)
        
        ; ADC DMA 缓冲区
        *(ADC_DMA)
        *(.adc_dma)
        
        ; 通用 DMA
        *(SRAM1)
        *(.sram1)
        *(.dma_buffer)
    }
    
    ; ==================== SRAM2 - USB DMA ====================
    RW_SRAM2 0x30020000 UNINIT 0x00020000 {
        ; USB 端点缓冲区
        *(USB_EP_BUFFER)
        *(.usb_ep_buffer)
        
        ; USB 数据缓冲区
        *(USB_DATA)
        *(.usb_data)
        
        *(SRAM2)
        *(.sram2)
    }
    
    ; ==================== SRAM3 - BDMA ====================
    RW_SRAM3 0x30040000 UNINIT 0x00008000 {
        *(SRAM3)
        *(.sram3)
        *(.bdma_buffer)
    }
    
    ; ==================== SRAM4 - 低功耗 ====================
    RW_SRAM4 0x38000000 UNINIT 0x00010000 {
        *(SRAM4)
        *(.sram4)
        *(.lowpower)
    }
}
```

### 2. IAR ICF 文件

```icf
/*###ICF### STM32H743 音频/ADC/USB 配置 ###*/

/*-Memory Regions-*/
define symbol __ICFEDIT_region_ITCM_start__   = 0x00000000;
define symbol __ICFEDIT_region_ITCM_end__     = 0x0000FFFF;
define symbol __ICFEDIT_region_DTCM_start__   = 0x20000000;
define symbol __ICFEDIT_region_DTCM_end__     = 0x2001FFFF;
define symbol __ICFEDIT_region_FLASH_start__  = 0x08000000;
define symbol __ICFEDIT_region_FLASH_end__    = 0x081FFFFF;
define symbol __ICFEDIT_region_AXIRAM_start__ = 0x24000000;
define symbol __ICFEDIT_region_AXIRAM_end__   = 0x2407FFFF;
define symbol __ICFEDIT_region_SRAM1_start__  = 0x30000000;
define symbol __ICFEDIT_region_SRAM1_end__    = 0x3001FFFF;
define symbol __ICFEDIT_region_SRAM2_start__  = 0x30020000;
define symbol __ICFEDIT_region_SRAM2_end__    = 0x3003FFFF;
define symbol __ICFEDIT_region_SRAM3_start__  = 0x30040000;
define symbol __ICFEDIT_region_SRAM3_end__    = 0x30047FFF;
define symbol __ICFEDIT_region_SRAM4_start__  = 0x38000000;
define symbol __ICFEDIT_region_SRAM4_end__    = 0x3800FFFF;

/*-Sizes-*/
define symbol __ICFEDIT_size_cstack__ = 0x2000;
define symbol __ICFEDIT_size_heap__   = 0x2000;

define memory mem with size = 4G;

define region ITCM_region   = mem:[from __ICFEDIT_region_ITCM_start__ 
                                   to __ICFEDIT_region_ITCM_end__];
define region DTCM_region   = mem:[from __ICFEDIT_region_DTCM_start__ 
                                   to __ICFEDIT_region_DTCM_end__];
define region FLASH_region  = mem:[from __ICFEDIT_region_FLASH_start__ 
                                   to __ICFEDIT_region_FLASH_end__];
define region AXIRAM_region = mem:[from __ICFEDIT_region_AXIRAM_start__ 
                                   to __ICFEDIT_region_AXIRAM_end__];
define region SRAM1_region  = mem:[from __ICFEDIT_region_SRAM1_start__ 
                                   to __ICFEDIT_region_SRAM1_end__];
define region SRAM2_region  = mem:[from __ICFEDIT_region_SRAM2_start__ 
                                   to __ICFEDIT_region_SRAM2_end__];
define region SRAM3_region  = mem:[from __ICFEDIT_region_SRAM3_start__ 
                                   to __ICFEDIT_region_SRAM3_end__];
define region SRAM4_region  = mem:[from __ICFEDIT_region_SRAM4_start__ 
                                   to __ICFEDIT_region_SRAM4_end__];

define block CSTACK with alignment = 8, size = __ICFEDIT_size_cstack__ { };
define block HEAP   with alignment = 8, size = __ICFEDIT_size_heap__   { };

/* 初始化策略 */
initialize by copy { readwrite };
initialize by copy { section .itcm };
initialize by copy { section .axiram };

/* 不初始化的段 */
do not initialize { section .noinit };
do not initialize { section .sai_dma, section .adc_dma };
do not initialize { section .usb_ep_buffer, section .usb_data };
do not initialize { section .sram1, section .sram2, section .sram3, section .sram4 };

/* 放置规则 */
place at address mem:0x08000000 { readonly section .intvec };

place in FLASH_region  { readonly };
place in DTCM_region   { readwrite, block CSTACK, block HEAP };

/* ITCM - 关键代码 */
place in ITCM_region   { section .itcm, section .itcm.audio, 
                         section .itcm.adc, section .itcm.usb };

/* AXI SRAM - 大缓冲区 */
place in AXIRAM_region { section .axiram, section .audio_buffer, 
                         section .audio_ringbuf, section .fft_buffer,
                         section .framebuffer };

/* SRAM1 - SAI/ADC DMA */
place in SRAM1_region  { section .sram1, section .sai_dma, 
                         section .sai_tx_buffer, section .sai_rx_buffer,
                         section .adc_dma, section .dma_buffer };

/* SRAM2 - USB DMA */
place in SRAM2_region  { section .sram2, section .usb_ep_buffer, 
                         section .usb_data };

/* SRAM3 - BDMA */
place in SRAM3_region  { section .sram3, section .bdma_buffer };

/* SRAM4 - 低功耗 */
place in SRAM4_region  { section .sram4, section .lowpower };
```

------

## 七、性能优化总结

### 1. 内存分配最佳实践

|   数据类型   | 推荐内存区域 |          原因           |
| :----------: | :----------: | :---------------------: |
| 关键中断代码 |     ITCM     |  零等待状态，最快执行   |
| DSP算法代码  |     ITCM     |    避免Flash等待状态    |
|    栈/堆     |     DTCM     |   零等待状态，CPU直连   |
|  滤波器状态  |     DTCM     | 频繁访问，需要最快速度  |
| 音频环形缓冲 |   AXI SRAM   |     大容量，可缓存      |
|    帧缓冲    |   AXI SRAM   |   大容量，DMA2D可访问   |
| SAI DMA缓冲  |    SRAM1     | D2域，DMA可访问，非缓存 |
| ADC DMA缓冲  |    SRAM1     | D2域，DMA可访问，非缓存 |
| USB DMA缓冲  |    SRAM2     |   D2域，USB DMA可访问   |
|   BDMA缓冲   |    SRAM3     |        BDMA专用         |
|  低功耗数据  |    SRAM4     |  D3域，低功耗模式保持   |

### 2. 性能指标参考

|     场景     | 典型性能 |    优化后性能    |
| :----------: | :------: | :--------------: |
| 音频处理延迟 | 10-20ms  |      2-5ms       |
|  ADC采样率   |  1 MSPS  | 3.6 MSPS (单ADC) |
|  USB吞吐量   | 20 MB/s  |     40 MB/s      |
|  FFT 1024点  |  500μs   |   100μs (ITCM)   |
|  滤波器处理  | 200μs/帧 |  50μs/帧 (ITCM)  |

### 3. 常见问题排查

|    问题     |     可能原因     |           解决方案           |
| :---------: | :--------------: | :--------------------------: |
|  音频爆音   |    缓冲区欠载    | 增加缓冲区大小，优化处理速度 |
| ADC数据丢失 |     DMA溢出      |  使用双缓冲，提高处理优先级  |
|  USB传输慢  | 缓冲区在错误内存 |    移到SRAM2，配置非缓存     |
|  系统卡顿   | Cache一致性问题  |  正确配置MPU，使用Cache维护  |
| Hard Fault  |      栈溢出      |   增加栈大小，检查递归深度   |

------

如需更详细的特定场景配置或有其他问题，请随时告诉我！





```
如果上述 MCU 改成 STM32U5 需要怎么配置
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

# STM32U5 系列链接脚本与内存配置详解

STM32U5 是 ST 的超低功耗系列，基于 Cortex-M33 内核，具有 TrustZone 安全特性，内存架构与 STM32H7 有显著差异。

------

## 一、STM32U5 vs STM32H7 架构对比

### 1. 核心差异对比

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                    STM32U5 vs STM32H7 架构对比                           │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  特性              │  STM32H7 (Cortex-M7)    │  STM32U5 (Cortex-M33)   │
│  ─────────────────────────────────────────────────────────────────────  │
│  最高主频          │  480 MHz                │  160 MHz                │
│  内核架构          │  ARMv7-M                │  ARMv8-M                │
│  安全特性          │  无                     │  TrustZone              │
│  Cache             │  I-Cache + D-Cache      │  I-Cache only           │
│  TCM               │  ITCM + DTCM            │  无                     │
│  Flash             │  双Bank, 最大2MB        │  双Bank, 最大4MB        │
│  SRAM              │  多域分布               │  统一编址               │
│  DMA               │  DMA1/2 + BDMA          │  GPDMA (通用DMA)        │
│  低功耗            │  一般                   │  超低功耗优化           │
│  功耗模式          │  Stop0/1/2              │  Stop0/1/2/3 + Standby  │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 2. STM32U5 内存架构

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                    STM32U575/585 内存映射                                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                         Flash (最大4MB)                          │   │
│  │  ┌─────────────────────────────────────────────────────────┐    │   │
│  │  │ Bank 1        │ 0x08000000 │ 最大2MB │ 主程序           │    │   │
│  │  │ Bank 2        │ 0x08100000 │ 最大2MB │ OTA/数据         │    │   │
│  │  │ OTP           │ 0x0BFA0000 │ 512B    │ 一次性编程       │    │   │
│  │  │ System Memory │ 0x0BF90000 │ 64KB    │ Bootloader       │    │   │
│  │  └─────────────────────────────────────────────────────────┘    │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                         SRAM (最大786KB)                         │   │
│  │  ┌─────────────────────────────────────────────────────────┐    │   │
│  │  │ SRAM1         │ 0x20000000 │ 192KB   │ 通用RAM          │    │   │
│  │  │ SRAM2         │ 0x20030000 │ 64KB    │ 通用RAM          │    │   │
│  │  │ SRAM3         │ 0x20040000 │ 512KB   │ 通用RAM/图形     │    │   │
│  │  │ SRAM4         │ 0x28000000 │ 16KB    │ 备份域RAM        │    │   │
│  │  │ SRAM5 (U59x)  │ 0x200C0000 │ 832KB   │ 扩展RAM          │    │   │
│  │  └─────────────────────────────────────────────────────────┘    │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                      TrustZone 安全分区                          │   │
│  │  ┌─────────────────────────────────────────────────────────┐    │   │
│  │  │ Secure Flash      │ 可配置大小 │ 安全代码/数据          │    │   │
│  │  │ Non-Secure Flash  │ 可配置大小 │ 非安全代码/数据        │    │   │
│  │  │ Secure SRAM       │ 可配置大小 │ 安全RAM                │    │   │
│  │  │ Non-Secure SRAM   │ 可配置大小 │ 非安全RAM              │    │   │
│  │  └─────────────────────────────────────────────────────────┘    │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  特点:                                                                  │
│  - 无 TCM (紧耦合内存)                                                 │
│  - 只有 I-Cache (无 D-Cache)                                           │
│  - SRAM 统一编址，无域隔离                                             │
│  - 支持 TrustZone 安全/非安全分区                                      │
│  - SRAM4 在备份域，低功耗模式保持                                      │
│  - GPDMA 可访问所有 SRAM                                               │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 3. STM32U5 各型号内存配置

|   型号    | Flash | SRAM1 | SRAM2 | SRAM3 | SRAM4 | SRAM5 | 总SRAM |
| :-------: | :---: | :---: | :---: | :---: | :---: | :---: | :----: |
| STM32U575 |  2MB  | 192KB | 64KB  | 512KB | 16KB  |   -   | 784KB  |
| STM32U585 |  2MB  | 192KB | 64KB  | 512KB | 16KB  |   -   | 784KB  |
| STM32U595 |  4MB  | 192KB | 64KB  | 512KB | 16KB  | 832KB | 1616KB |
| STM32U5A5 |  4MB  | 192KB | 64KB  | 512KB | 16KB  | 832KB | 1616KB |

------

## 二、GCC 链接脚本

### 1. STM32U575/585 基础链接脚本

```ld
/* stm32u575_base.ld - STM32U575/585 基础链接脚本 */
ENTRY(Reset_Handler)

/* 堆栈配置 */
_Min_Heap_Size  = 0x2000;   /* 8KB */
_Min_Stack_Size = 0x2000;   /* 8KB */

/* 内存定义 - STM32U575/585 */
MEMORY
{
    /* Flash - 双Bank */
    FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 2048K
    
    /* SRAM 区域 */
    SRAM1 (xrw)     : ORIGIN = 0x20000000, LENGTH = 192K   /* 通用RAM */
    SRAM2 (xrw)     : ORIGIN = 0x20030000, LENGTH = 64K    /* 通用RAM */
    SRAM3 (xrw)     : ORIGIN = 0x20040000, LENGTH = 512K   /* 通用/图形RAM */
    SRAM4 (xrw)     : ORIGIN = 0x28000000, LENGTH = 16K    /* 备份域RAM */
}

/* 内存边界符号 */
__flash_start = ORIGIN(FLASH);
__flash_end   = ORIGIN(FLASH) + LENGTH(FLASH);
__sram1_start = ORIGIN(SRAM1);
__sram1_end   = ORIGIN(SRAM1) + LENGTH(SRAM1);
__sram2_start = ORIGIN(SRAM2);
__sram2_end   = ORIGIN(SRAM2) + LENGTH(SRAM2);
__sram3_start = ORIGIN(SRAM3);
__sram3_end   = ORIGIN(SRAM3) + LENGTH(SRAM3);
__sram4_start = ORIGIN(SRAM4);
__sram4_end   = ORIGIN(SRAM4) + LENGTH(SRAM4);

SECTIONS
{
    /* ==================== Flash 代码段 ==================== */
    
    /* 中断向量表 */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* 代码段 */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)

        KEEP(*(.init))
        KEEP(*(.fini))

        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* 只读数据 */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* ARM 异常处理 */
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    /* C++ 构造/析构函数 */
    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    /* ==================== SRAM1 - 主数据区 ==================== */
    
    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        *(.RamFunc)
        *(.RamFunc*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM1 AT > FLASH

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > SRAM1

    /* 不初始化区域 */
    .noinit (NOLOAD) :
    {
        . = ALIGN(4);
        *(.noinit)
        *(.noinit*)
        . = ALIGN(4);
    } > SRAM1

    /* 堆和栈 */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        __heap_start = .;
        . = . + _Min_Heap_Size;
        __heap_end = .;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > SRAM1

    _estack = ORIGIN(SRAM1) + LENGTH(SRAM1);

    /* ==================== SRAM2 - DMA 缓冲区 ==================== */
    
    .sram2 (NOLOAD) :
    {
        . = ALIGN(4);
        __sram2_bss_start = .;
        *(.sram2)
        *(.sram2*)
        *(.dma_buffer)
        . = ALIGN(4);
        __sram2_bss_end = .;
    } > SRAM2

    /* ==================== SRAM3 - 大缓冲区/图形 ==================== */
    
    .sram3 (NOLOAD) :
    {
        . = ALIGN(4);
        __sram3_bss_start = .;
        *(.sram3)
        *(.sram3*)
        *(.framebuffer)
        *(.audio_buffer)
        . = ALIGN(4);
        __sram3_bss_end = .;
    } > SRAM3

    /* ==================== SRAM4 - 备份域 ==================== */
    
    .sram4 (NOLOAD) :
    {
        . = ALIGN(4);
        __sram4_bss_start = .;
        *(.sram4)
        *(.sram4*)
        *(.backup_data)
        . = ALIGN(4);
        __sram4_bss_end = .;
    } > SRAM4

    /* 调试信息 */
    .ARM.attributes 0 : { *(.ARM.attributes) }
}

/* 断言检查 */
ASSERT(_estack <= __sram1_end, "SRAM1 overflow!")
ASSERT(__sram2_bss_end <= __sram2_end, "SRAM2 overflow!")
ASSERT(__sram3_bss_end <= __sram3_end, "SRAM3 overflow!")
ASSERT(__sram4_bss_end <= __sram4_end, "SRAM4 overflow!")
```

### 2. STM32U5 音频/ADC/USB 综合链接脚本

```ld
/* stm32u575_audio_adc_usb.ld - 音频/ADC/USB 综合配置 */
ENTRY(Reset_Handler)

_Min_Heap_Size  = 0x4000;   /* 16KB */
_Min_Stack_Size = 0x2000;   /* 8KB */

MEMORY
{
    /* Flash 双Bank */
    FLASH_BANK1 (rx)    : ORIGIN = 0x08000000, LENGTH = 1024K
    FLASH_BANK2 (rx)    : ORIGIN = 0x08100000, LENGTH = 1024K
    
    /* SRAM 区域 */
    SRAM1 (xrw)         : ORIGIN = 0x20000000, LENGTH = 192K
    SRAM2 (xrw)         : ORIGIN = 0x20030000, LENGTH = 64K
    SRAM3 (xrw)         : ORIGIN = 0x20040000, LENGTH = 512K
    SRAM4 (xrw)         : ORIGIN = 0x28000000, LENGTH = 16K
}

/* 内存边界符号 */
__flash_bank1_start = ORIGIN(FLASH_BANK1);
__flash_bank1_end   = ORIGIN(FLASH_BANK1) + LENGTH(FLASH_BANK1);
__flash_bank2_start = ORIGIN(FLASH_BANK2);
__flash_bank2_end   = ORIGIN(FLASH_BANK2) + LENGTH(FLASH_BANK2);

SECTIONS
{
    /* ==================== Flash Bank1 - 主程序 ==================== */
    
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH_BANK1

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        
        KEEP(*(.init))
        KEEP(*(.fini))
        
        . = ALIGN(4);
        _etext = .;
    } > FLASH_BANK1

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        
        /* 音频系数表 */
        . = ALIGN(4);
        __audio_coef_start = .;
        *(.audio_coef)
        *(.audio_coef*)
        __audio_coef_end = .;
        
        . = ALIGN(4);
    } > FLASH_BANK1

    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH_BANK1
    
    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH_BANK1

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH_BANK1

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH_BANK1

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH_BANK1

    /* ==================== Flash Bank2 - OTA/数据 ==================== */
    
    .ota_region (NOLOAD) :
    {
        . = ALIGN(4);
        __ota_start = .;
        . = . + 896K;
        __ota_end = .;
    } > FLASH_BANK2

    .config_data 0x081E0000 :
    {
        . = ALIGN(4);
        __config_start = .;
        KEEP(*(.config_data))
        . = ALIGN(4);
        __config_end = .;
    } > FLASH_BANK2

    /* ==================== SRAM1 - 主数据区 ==================== */
    
    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        *(.RamFunc)
        *(.RamFunc*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM1 AT > FLASH_BANK1

    /* 关键代码放入 RAM 执行 (STM32U5 无 ITCM) */
    _siramfunc = LOADADDR(.ramfunc);
    
    .ramfunc :
    {
        . = ALIGN(4);
        _sramfunc = .;
        
        /* 音频 DSP 代码 */
        *(.ramfunc.audio)
        *(.ramfunc.audio*)
        *audio_dsp.o(.text*)
        
        /* ADC 处理代码 */
        *(.ramfunc.adc)
        *(.ramfunc.adc*)
        
        /* USB 关键代码 */
        *(.ramfunc.usb)
        *(.ramfunc.usb*)
        
        /* 中断处理 */
        *(.ramfunc.isr)
        *(.ramfunc.isr*)
        
        . = ALIGN(4);
        _eramfunc = .;
    } > SRAM1 AT > FLASH_BANK1

    /* 音频处理状态 */
    .audio_state (NOLOAD) :
    {
        . = ALIGN(4);
        __audio_state_start = .;
        *(.audio_state)
        *(.audio_state*)
        *(.biquad_state)
        *(.fir_state)
        . = ALIGN(4);
        __audio_state_end = .;
    } > SRAM1

    /* ADC 处理缓存 */
    .adc_cache (NOLOAD) :
    {
        . = ALIGN(4);
        __adc_cache_start = .;
        *(.adc_cache)
        *(.adc_cache*)
        . = ALIGN(4);
        __adc_cache_end = .;
    } > SRAM1

    /* USB 控制结构 */
    .usb_ctrl (NOLOAD) :
    {
        . = ALIGN(4);
        __usb_ctrl_start = .;
        *(.usb_ctrl)
        *(.usb_ctrl*)
        . = ALIGN(4);
        __usb_ctrl_end = .;
    } > SRAM1

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > SRAM1

    .noinit (NOLOAD) :
    {
        . = ALIGN(4);
        *(.noinit)
        *(.noinit*)
        . = ALIGN(4);
    } > SRAM1

    ._user_heap_stack (NOLOAD) :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        __heap_start = .;
        . = . + _Min_Heap_Size;
        __heap_end = .;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > SRAM1

    _estack = ORIGIN(SRAM1) + LENGTH(SRAM1);

    /* ==================== SRAM2 - DMA 缓冲区 ==================== */
    
    .sram2_dma (NOLOAD) :
    {
        . = ALIGN(4);
        __sram2_start = .;
        
        /* SAI DMA 缓冲区 */
        *(.sai_dma)
        *(.sai_dma*)
        *(.sai_tx_buffer)
        *(.sai_rx_buffer)
        
        /* ADC DMA 缓冲区 */
        *(.adc_dma)
        *(.adc_dma*)
        
        /* USB DMA 缓冲区 */
        *(.usb_dma)
        *(.usb_dma*)
        *(.usb_ep_buffer)
        
        /* 通用 DMA */
        *(.sram2)
        *(.sram2*)
        *(.dma_buffer)
        
        . = ALIGN(4);
        __sram2_end = .;
    } > SRAM2

    /* ==================== SRAM3 - 大缓冲区 ==================== */
    
    .sram3_buffer (NOLOAD) :
    {
        . = ALIGN(4);
        __sram3_start = .;
        
        /* 音频环形缓冲区 */
        *(.audio_buffer)
        *(.audio_buffer*)
        *(.audio_ringbuf)
        
        /* FFT 缓冲区 */
        *(.fft_buffer)
        *(.fft_buffer*)
        
        /* 帧缓冲 */
        *(.framebuffer)
        *(.framebuffer*)
        
        /* USB 大数据缓冲 */
        *(.usb_data)
        *(.usb_data*)
        
        /* 通用大缓冲 */
        *(.sram3)
        *(.sram3*)
        
        . = ALIGN(4);
        __sram3_end = .;
    } > SRAM3

    /* ==================== SRAM4 - 备份域 ==================== */
    
    .sram4_backup (NOLOAD) :
    {
        . = ALIGN(4);
        __sram4_start = .;
        *(.sram4)
        *(.sram4*)
        *(.backup_data)
        *(.lowpower_data)
        . = ALIGN(4);
        __sram4_end = .;
    } > SRAM4

    .ARM.attributes 0 : { *(.ARM.attributes) }
}

/* 断言检查 */
ASSERT(_estack <= ORIGIN(SRAM1) + LENGTH(SRAM1), "SRAM1 overflow!")
ASSERT(__sram2_end <= ORIGIN(SRAM2) + LENGTH(SRAM2), "SRAM2 overflow!")
ASSERT(__sram3_end <= ORIGIN(SRAM3) + LENGTH(SRAM3), "SRAM3 overflow!")
ASSERT(__sram4_end <= ORIGIN(SRAM4) + LENGTH(SRAM4), "SRAM4 overflow!")
```

### 3. STM32U5 TrustZone 安全配置链接脚本

```ld
/* stm32u575_secure.ld - TrustZone 安全区域链接脚本 */
ENTRY(Reset_Handler)

_Min_Heap_Size  = 0x1000;
_Min_Stack_Size = 0x1000;

/* 安全/非安全边界配置 (需与 Option Bytes 一致) */
/* 假设前 512KB Flash 和前 128KB SRAM 为安全区域 */

MEMORY
{
    /* 安全 Flash */
    FLASH_S (rx)    : ORIGIN = 0x0C000000, LENGTH = 512K   /* 安全别名地址 */
    
    /* 非安全可调用区域 (NSC) */
    FLASH_NSC (rx)  : ORIGIN = 0x0C07E000, LENGTH = 8K
    
    /* 安全 SRAM */
    SRAM1_S (xrw)   : ORIGIN = 0x30000000, LENGTH = 128K   /* 安全别名地址 */
    
    /* 安全 SRAM4 (备份域) */
    SRAM4_S (xrw)   : ORIGIN = 0x38000000, LENGTH = 16K
}

SECTIONS
{
    /* ==================== 安全代码区 ==================== */
    
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH_S

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        
        /* 安全函数 */
        *(.secure_text)
        *(.secure_text*)
        
        KEEP(*(.init))
        KEEP(*(.fini))
        
        . = ALIGN(4);
        _etext = .;
    } > FLASH_S

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        
        /* 安全常量 */
        *(.secure_rodata)
        *(.secure_rodata*)
        
        . = ALIGN(4);
    } > FLASH_S

    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH_S
    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH_S

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH_S

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH_S

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH_S

    /* ==================== NSC 区域 (非安全可调用) ==================== */
    
    .gnu.sgstubs :
    {
        . = ALIGN(4);
        *(.gnu.sgstubs*)
        . = ALIGN(4);
    } > FLASH_NSC

    /* ==================== 安全 SRAM ==================== */
    
    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        
        /* 安全数据 */
        *(.secure_data)
        *(.secure_data*)
        
        . = ALIGN(4);
        _edata = .;
    } > SRAM1_S AT > FLASH_S

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        
        /* 安全 BSS */
        *(.secure_bss)
        *(.secure_bss*)
        
        . = ALIGN(4);
        _ebss = .;
    } > SRAM1_S

    ._user_heap_stack (NOLOAD) :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > SRAM1_S

    _estack = ORIGIN(SRAM1_S) + LENGTH(SRAM1_S);

    /* ==================== 安全备份 SRAM ==================== */
    
    .sram4_secure (NOLOAD) :
    {
        . = ALIGN(4);
        *(.secure_backup)
        *(.secure_backup*)
        . = ALIGN(4);
    } > SRAM4_S
}
/* stm32u575_nonsecure.ld - TrustZone 非安全区域链接脚本 */
ENTRY(Reset_Handler)

_Min_Heap_Size  = 0x4000;
_Min_Stack_Size = 0x2000;

MEMORY
{
    /* 非安全 Flash */
    FLASH_NS (rx)   : ORIGIN = 0x08080000, LENGTH = 1536K
    
    /* 非安全 SRAM */
    SRAM1_NS (xrw)  : ORIGIN = 0x20020000, LENGTH = 64K    /* SRAM1 后半部分 */
    SRAM2_NS (xrw)  : ORIGIN = 0x20030000, LENGTH = 64K
    SRAM3_NS (xrw)  : ORIGIN = 0x20040000, LENGTH = 512K
}

SECTIONS
{
    /* ==================== 非安全代码区 ==================== */
    
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH_NS

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH_NS

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH_NS

    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH_NS
    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH_NS

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH_NS

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH_NS

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH_NS

    /* ==================== 非安全 SRAM ==================== */
    
    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        *(.RamFunc)
        . = ALIGN(4);
        _edata = .;
    } > SRAM1_NS AT > FLASH_NS

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > SRAM1_NS

    ._user_heap_stack (NOLOAD) :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > SRAM1_NS

    _estack = ORIGIN(SRAM1_NS) + LENGTH(SRAM1_NS);

    /* ==================== SRAM2 - DMA 缓冲区 ==================== */
    
    .sram2_dma (NOLOAD) :
    {
        . = ALIGN(4);
        *(.sram2)
        *(.dma_buffer)
        . = ALIGN(4);
    } > SRAM2_NS

    /* ==================== SRAM3 - 大缓冲区 ==================== */
    
    .sram3_buffer (NOLOAD) :
    {
        . = ALIGN(4);
        *(.sram3)
        *(.framebuffer)
        *(.audio_buffer)
        . = ALIGN(4);
    } > SRAM3_NS
}
```

------

## 三、STM32U5 特有配置

### 1. ICACHE 配置 (STM32U5 只有 I-Cache)

```c
/* icache_config.c - STM32U5 I-Cache 配置 */
#include "stm32u5xx_hal.h"

/*
 * STM32U5 Cache 特点:
 * - 只有 I-Cache (指令缓存)，无 D-Cache
 * - I-Cache 大小: 16KB
 * - 4路组相联
 * - Cache line: 32 bytes
 * - 无需处理 DMA 数据一致性问题 (无 D-Cache)
 */

/*
 * 启用 I-Cache
 */
void ICACHE_Enable(void)
{
    /* 检查 I-Cache 是否已启用 */
    if ((ICACHE->CR & ICACHE_CR_EN) == 0) {
        /* 使能 I-Cache */
        HAL_ICACHE_Enable();
    }
}

/*
 * 禁用 I-Cache
 */
void ICACHE_Disable(void)
{
    HAL_ICACHE_Disable();
}

/*
 * 无效化 I-Cache
 */
void ICACHE_Invalidate(void)
{
    HAL_ICACHE_Invalidate();
}

/*
 * 配置 I-Cache 区域监控
 */
void ICACHE_ConfigRegionMonitor(uint32_t region, uint32_t start_addr, uint32_t end_addr)
{
    ICACHE_RegionConfigTypeDef region_config = {0};
    
    region_config.BaseAddress = start_addr;
    region_config.RemapAddress = start_addr;
    region_config.Size = ICACHE_REGIONSIZE_2MB;
    region_config.TrafficRoute = ICACHE_MASTER1_PORT;
    region_config.OutputBurstType = ICACHE_OUTPUT_BURST_WRAP;
    
    HAL_ICACHE_EnableRemapRegion(region, &region_config);
}

/*
 * 获取 I-Cache 命中率统计
 */
void ICACHE_GetStats(uint32_t *hit_count, uint32_t *miss_count)
{
    if (hit_count) {
        *hit_count = HAL_ICACHE_Monitor_GetHitValue();
    }
    if (miss_count) {
        *miss_count = HAL_ICACHE_Monitor_GetMissValue();
    }
}

/*
 * 重置 I-Cache 统计
 */
void ICACHE_ResetStats(void)
{
    HAL_ICACHE_Monitor_Reset(ICACHE_MONITOR_HIT_MISS);
}
```

### 2. GPDMA 配置 (替代 STM32H7 的 DMA1/DMA2)

```c
/* gpdma_config.c - STM32U5 GPDMA 配置 */
#include "stm32u5xx_hal.h"

/*
 * STM32U5 GPDMA 特点:
 * - 16个独立通道 (Channel 0-15)
 * - 支持链表模式 (Linked List)
 * - 支持 2D 传输
 * - 所有通道可访问所有 SRAM
 * - 无需考虑内存域限制 (与 H7 不同)
 */

/* DMA 句柄 */
static DMA_HandleTypeDef hdma_sai_rx;
static DMA_HandleTypeDef hdma_sai_tx;
static DMA_HandleTypeDef hdma_adc;
static DMA_HandleTypeDef hdma_usb;

/*
 * SAI RX DMA 配置
 */
void GPDMA_SAI_RX_Init(void)
{
    hdma_sai_rx.Instance = GPDMA1_Channel0;
    hdma_sai_rx.Init.Request = GPDMA1_REQUEST_SAI1_A;
    hdma_sai_rx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    hdma_sai_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_sai_rx.Init.SrcInc = DMA_SINC_FIXED;
    hdma_sai_rx.Init.DestInc = DMA_DINC_INCREMENTED;
    hdma_sai_rx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_WORD;
    hdma_sai_rx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_WORD;
    hdma_sai_rx.Init.Priority = DMA_HIGH_PRIORITY;
    hdma_sai_rx.Init.SrcBurstLength = 4;
    hdma_sai_rx.Init.DestBurstLength = 4;
    hdma_sai_rx.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT1;
    hdma_sai_rx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    hdma_sai_rx.Init.Mode = DMA_NORMAL;
    
    HAL_DMA_Init(&hdma_sai_rx);
    
    /* 配置循环模式 (使用链表) */
    HAL_DMA_ConfigChannelAttributes(&hdma_sai_rx, DMA_CHANNEL_PRIV);
}

/*
 * SAI TX DMA 配置
 */
void GPDMA_SAI_TX_Init(void)
{
    hdma_sai_tx.Instance = GPDMA1_Channel1;
    hdma_sai_tx.Init.Request = GPDMA1_REQUEST_SAI1_B;
    hdma_sai_tx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    hdma_sai_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_sai_tx.Init.SrcInc = DMA_SINC_INCREMENTED;
    hdma_sai_tx.Init.DestInc = DMA_DINC_FIXED;
    hdma_sai_tx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_WORD;
    hdma_sai_tx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_WORD;
    hdma_sai_tx.Init.Priority = DMA_HIGH_PRIORITY;
    hdma_sai_tx.Init.SrcBurstLength = 4;
    hdma_sai_tx.Init.DestBurstLength = 4;
    hdma_sai_tx.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT1;
    hdma_sai_tx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    hdma_sai_tx.Init.Mode = DMA_NORMAL;
    
    HAL_DMA_Init(&hdma_sai_tx);
}

/*
 * ADC DMA 配置
 */
void GPDMA_ADC_Init(void)
{
    hdma_adc.Instance = GPDMA1_Channel2;
    hdma_adc.Init.Request = GPDMA1_REQUEST_ADC1;
    hdma_adc.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    hdma_adc.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc.Init.SrcInc = DMA_SINC_FIXED;
    hdma_adc.Init.DestInc = DMA_DINC_INCREMENTED;
    hdma_adc.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_HALFWORD;
    hdma_adc.Init.DestDataWidth = DMA_DEST_DATAWIDTH_HALFWORD;
    hdma_adc.Init.Priority = DMA_HIGH_PRIORITY;
    hdma_adc.Init.SrcBurstLength = 1;
    hdma_adc.Init.DestBurstLength = 1;
    hdma_adc.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT1;
    hdma_adc.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    hdma_adc.Init.Mode = DMA_NORMAL;
    
    HAL_DMA_Init(&hdma_adc);
}

/*
 * GPDMA 链表模式配置 (用于循环缓冲)
 */
DMA_QListTypeDef sai_rx_queue;
DMA_NodeTypeDef sai_rx_node1, sai_rx_node2;

void GPDMA_LinkedList_Init(uint8_t *buffer1, uint8_t *buffer2, uint32_t size)
{
    DMA_NodeConfTypeDef node_config = {0};
    
    /* 配置节点1 */
    node_config.NodeType = DMA_GPDMA_LINEAR_NODE;
    node_config.Init.Request = GPDMA1_REQUEST_SAI1_A;
    node_config.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    node_config.Init.Direction = DMA_PERIPH_TO_MEMORY;
    node_config.Init.SrcInc = DMA_SINC_FIXED;
    node_config.Init.DestInc = DMA_DINC_INCREMENTED;
    node_config.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_WORD;
    node_config.Init.DestDataWidth = DMA_DEST_DATAWIDTH_WORD;
    node_config.SrcAddress = (uint32_t)&SAI1_Block_A->DR;
    node_config.DstAddress = (uint32_t)buffer1;
    node_config.DataSize = size;
    
    HAL_DMAEx_List_BuildNode(&node_config, &sai_rx_node1);
    
    /* 配置节点2 */
    node_config.DstAddress = (uint32_t)buffer2;
    HAL_DMAEx_List_BuildNode(&node_config, &sai_rx_node2);
    
    /* 创建循环链表 */
    HAL_DMAEx_List_Init(&sai_rx_queue);
    HAL_DMAEx_List_InsertNode_Tail(&sai_rx_queue, &sai_rx_node1);
    HAL_DMAEx_List_InsertNode_Tail(&sai_rx_queue, &sai_rx_node2);
    HAL_DMAEx_List_SetCircularMode(&sai_rx_queue);
    
    /* 链接到 DMA 通道 */
    HAL_DMAEx_List_LinkQ(&hdma_sai_rx, &sai_rx_queue);
}

/*
 * 启动链表 DMA 传输
 */
void GPDMA_LinkedList_Start(void)
{
    HAL_DMAEx_List_Start_IT(&hdma_sai_rx);
}
```

### 3. 低功耗模式下的内存保持

```c
/* lowpower_memory.c - STM32U5 低功耗内存管理 */
#include "stm32u5xx_hal.h"

/*
 * STM32U5 低功耗模式下的 SRAM 保持:
 * 
 * | 模式      | SRAM1 | SRAM2 | SRAM3 | SRAM4 |
 * |-----------|-------|-------|-------|-------|
 * | Run       | ✓     | ✓     | ✓     | ✓     |
 * | Sleep     | ✓     | ✓     | ✓     | ✓     |
 * | Stop0     | ✓     | ✓     | ✓     | ✓     |
 * | Stop1     | ✓     | ✓     | ✓     | ✓     |
 * | Stop2     | 可选  | 可选  | 可选  | ✓     |
 * | Stop3     | ✗     | ✗     | ✗     | ✓     |
 * | Standby   | ✗     | ✗     | ✗     | ✓     |
 * | Shutdown  | ✗     | ✗     | ✗     | ✗     |
 */

/* SRAM4 备份数据 - 在 Stop3/Standby 模式下保持 */
__attribute__((section(".sram4")))
struct {
    uint32_t magic;
    uint32_t wakeup_count;
    uint32_t last_timestamp;
    uint8_t  config[256];
    uint32_t crc;
} backup_data;

/*
 * 配置 Stop2 模式下的 SRAM 保持
 */
void LowPower_ConfigSRAMRetention(void)
{
    /* 配置 SRAM 页保持 */
    /* SRAM1: 192KB = 12 pages (16KB each) */
    /* SRAM2: 64KB = 4 pages */
    /* SRAM3: 512KB = 32 pages */
    
    /* 在 Stop2 模式下保持 SRAM1 前 64KB (4 pages) */
    HAL_PWREx_EnableSRAM1ContentStandbyRetention(PWR_SRAM1_PAGE1_STANDBY_RETENTION |
                                                  PWR_SRAM1_PAGE2_STANDBY_RETENTION |
                                                  PWR_SRAM1_PAGE3_STANDBY_RETENTION |
                                                  PWR_SRAM1_PAGE4_STANDBY_RETENTION);
    
    /* 在 Stop2 模式下保持 SRAM2 全部 */
    HAL_PWREx_EnableSRAM2ContentStandbyRetention(PWR_SRAM2_FULL_STANDBY_RETENTION);
    
    /* SRAM3 在 Stop2 模式下不保持 (节省功耗) */
    HAL_PWREx_DisableSRAM3ContentStandbyRetention();
    
    /* SRAM4 始终保持 (备份域) */
    HAL_PWREx_EnableBkupRAMRetention();
}

/*
 * 进入 Stop2 模式
 */
void LowPower_EnterStop2(void)
{
    /* 保存关键数据到 SRAM4 */
    backup_data.last_timestamp = HAL_GetTick();
    backup_data.crc = calculate_crc32(&backup_data, sizeof(backup_data) - 4);
    
    /* 配置唤醒源 */
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1_HIGH);
    
    /* 清除唤醒标志 */
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF1);
    
    /* 进入 Stop2 模式 */
    HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
}

/*
 * 进入 Stop3 模式 (只有 SRAM4 保持)
 */
void LowPower_EnterStop3(void)
{
    /* 保存所有关键数据到 SRAM4 */
    backup_data.last_timestamp = HAL_GetTick();
    backup_data.wakeup_count++;
    backup_data.crc = calculate_crc32(&backup_data, sizeof(backup_data) - 4);
    
    /* 配置唤醒源 */
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1_HIGH);
    
    /* 清除唤醒标志 */
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF1);
    
    /* 进入 Stop3 模式 */
    HAL_PWREx_EnterSTOP3Mode(PWR_STOPENTRY_WFI);
}

/*
 * 从低功耗模式唤醒后恢复
 */
void LowPower_RestoreAfterWakeup(void)
{
    /* 检查 SRAM4 数据完整性 */
    uint32_t crc = calculate_crc32(&backup_data, sizeof(backup_data) - 4);
    
    if (backup_data.magic == 0xDEADBEEF && crc == backup_data.crc) {
        /* 数据有效，恢复配置 */
        restore_config_from_backup();
    } else {
        /* 数据无效，使用默认配置 */
        init_default_config();
        backup_data.magic = 0xDEADBEEF;
        backup_data.wakeup_count = 0;
    }
}
```

### 4. USB OTG FS 配置 (STM32U5 无 HS PHY)

```c
/* usb_device_u5.c - STM32U5 USB 配置 */
#include "stm32u5xx_hal.h"
#include "usbd_core.h"

/*
 * STM32U5 USB 特点:
 * - 只有 USB OTG FS (Full Speed, 12 Mbps)
 * - 无 USB HS (High Speed)
 * - 内置 FS PHY
 * - 支持 BCD (Battery Charging Detection)
 * - 支持 LPM (Link Power Management)
 */

PCD_HandleTypeDef hpcd_USB_OTG_FS;
USBD_HandleTypeDef hUsbDeviceFS;

/* USB 缓冲区 - 放在 SRAM2 */
__attribute__((section(".sram2"), aligned(4)))
static uint8_t usb_rx_buffer[512];

__attribute__((section(".sram2"), aligned(4)))
static uint8_t usb_tx_buffer[512];

/*
 * USB 初始化
 */
void USB_Device_Init(void)
{
    /* 使能 USB 时钟 */
    __HAL_RCC_USB_CLK_ENABLE();
    
    /* 配置 USB */
    hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
    hpcd_USB_OTG_FS.Init.dev_endpoints = 6;
    hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
    hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
    hpcd_USB_OTG_FS.Init.Sof_enable = DISABLE;
    hpcd_USB_OTG_FS.Init.low_power_enable = ENABLE;  /* 低功耗优化 */
    hpcd_USB_OTG_FS.Init.lpm_enable = ENABLE;        /* LPM 支持 */
    hpcd_USB_OTG_FS.Init.battery_charging_enable = ENABLE;
    hpcd_USB_OTG_FS.Init.vbus_sensing_enable = ENABLE;
    hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
    
    HAL_PCD_Init(&hpcd_USB_OTG_FS);
    
    /* 配置 FIFO */
    HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_FS, 0x80);   /* 128 words */
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 0, 0x40); /* EP0: 64 words */
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 1, 0x40); /* EP1: 64 words */
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 2, 0x20); /* EP2: 32 words */
    
    /* 中断配置 */
    HAL_NVIC_SetPriority(OTG_FS_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
    
    /* 初始化 USB 设备库 */
    USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS);
    USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC);
    USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS);
    USBD_Start(&hUsbDeviceFS);
}

/*
 * USB 低功耗模式处理
 */
void USB_EnterLowPowerMode(void)
{
    /* 使能 USB LPM */
    HAL_PCD_SetLPMCallback(&hpcd_USB_OTG_FS, USB_LPM_Callback);
}

void USB_LPM_Callback(PCD_HandleTypeDef *hpcd, PCD_LPM_MsgTypeDef msg)
{
    switch (msg) {
        case PCD_LPM_L0_ACTIVE:
            /* 恢复正常模式 */
            SystemClock_Config();
            break;
            
        case PCD_LPM_L1_ACTIVE:
            /* 进入 L1 睡眠模式 */
            HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
            break;
            
        default:
            break;
    }
}

void OTG_FS_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
}
```

------

## 四、音频/ADC/USB 缓冲区配置

### 1. STM32U5 缓冲区分配策略

```c
/* buffer_allocation.h - STM32U5 缓冲区分配 */
#ifndef __BUFFER_ALLOCATION_H
#define __BUFFER_ALLOCATION_H

/*
 * STM32U5 内存分配策略:
 * 
 * | 数据类型           | 推荐区域 | 原因                           |
 * |--------------------|----------|--------------------------------|
 * | 栈/堆              | SRAM1    | 主RAM，最快访问                |
 * | 关键代码 (RAM执行) | SRAM1    | 无TCM，使用RAM执行             |
 * | 滤波器状态         | SRAM1    | 频繁访问                       |
 * | DMA 缓冲区         | SRAM2    | 专用DMA区域                    |
 * | 音频环形缓冲       | SRAM3    | 大容量                         |
 * | 帧缓冲             | SRAM3    | 大容量，LTDC可访问             |
 * | 低功耗保持数据     | SRAM4    | 备份域，Stop3/Standby保持      |
 * 
 * 注意: STM32U5 无 D-Cache，无需处理 Cache 一致性问题
 */

/* 音频配置 */
#define AUDIO_SAMPLE_RATE       48000
#define AUDIO_BIT_DEPTH         24
#define AUDIO_CHANNELS          2
#define AUDIO_FRAME_SIZE        256
#define AUDIO_DMA_BUFFER_FRAMES 4
#define AUDIO_RING_BUFFER_FRAMES 16

/* ADC 配置 */
#define ADC_SAMPLE_RATE         1000000
#define ADC_CHANNELS            4
#define ADC_DMA_BUFFER_SAMPLES  4096

/* USB 配置 */
#define USB_FS_MAX_PACKET_SIZE  64
#define USB_BUFFER_SIZE         4096

#endif /* __BUFFER_ALLOCATION_H */
```

### 2. 音频缓冲区定义

```c
/* audio_buffer_u5.c - STM32U5 音频缓冲区 */
#include "buffer_allocation.h"
#include <stdint.h>
#include <string.h>

/* 计算缓冲区大小 */
#define AUDIO
```





```
继续
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

```c
/* audio_buffer_u5.c - STM32U5 音频缓冲区 (续) */
#include "buffer_allocation.h"
#include <stdint.h>
#include <string.h>

/* 计算缓冲区大小 */
#define AUDIO_SAMPLE_SIZE       (AUDIO_BIT_DEPTH / 8)
#define AUDIO_FRAME_BYTES       (AUDIO_FRAME_SIZE * AUDIO_CHANNELS * AUDIO_SAMPLE_SIZE)
#define AUDIO_DMA_BUFFER_SIZE   (AUDIO_FRAME_BYTES * AUDIO_DMA_BUFFER_FRAMES)
#define AUDIO_RING_BUFFER_SIZE  (AUDIO_FRAME_BYTES * AUDIO_RING_BUFFER_FRAMES)

/* FFT 配置 */
#define FFT_SIZE                1024
#define FFT_BUFFER_SIZE         (FFT_SIZE * sizeof(float) * 2)

/* 音频帧结构 */
typedef struct {
    int32_t left;
    int32_t right;
} audio_sample_t;

typedef struct {
    audio_sample_t samples[AUDIO_FRAME_SIZE];
    uint32_t timestamp;
    uint8_t  flags;
} audio_frame_t;

/* ==================== SAI DMA 缓冲区 - SRAM2 ==================== */
/* STM32U5 无 D-Cache，无需特殊对齐，但保持 4 字节对齐 */

__attribute__((section(".sram2"), aligned(4)))
uint8_t sai_rx_buffer[AUDIO_DMA_BUFFER_SIZE];

__attribute__((section(".sram2"), aligned(4)))
uint8_t sai_tx_buffer[AUDIO_DMA_BUFFER_SIZE];

/* ==================== 环形缓冲区 - SRAM3 ==================== */

__attribute__((section(".sram3"), aligned(4)))
audio_frame_t audio_input_buffer[AUDIO_RING_BUFFER_FRAMES];

__attribute__((section(".sram3"), aligned(4)))
audio_frame_t audio_output_buffer[AUDIO_RING_BUFFER_FRAMES];

/* ==================== FFT 工作区 - SRAM3 ==================== */

__attribute__((section(".sram3"), aligned(4)))
float fft_input[FFT_SIZE * 2];

__attribute__((section(".sram3"), aligned(4)))
float fft_output[FFT_SIZE * 2];

__attribute__((section(".sram3"), aligned(4)))
float fft_magnitude[FFT_SIZE / 2];

/* ==================== 音频处理状态 - SRAM1 ==================== */

__attribute__((section(".audio_state")))
static struct {
    /* 环形缓冲区控制 */
    struct {
        audio_frame_t *buffer;
        uint32_t size;
        volatile uint32_t head;
        volatile uint32_t tail;
        volatile uint32_t count;
    } input_rb, output_rb;
    
    /* 统计信息 */
    uint32_t underrun_count;
    uint32_t overrun_count;
    uint32_t frame_count;
} audio_state;

/* ==================== 环形缓冲区实现 ==================== */

void audio_buffer_init(void)
{
    /* 初始化输入环形缓冲区 */
    audio_state.input_rb.buffer = audio_input_buffer;
    audio_state.input_rb.size = AUDIO_RING_BUFFER_FRAMES;
    audio_state.input_rb.head = 0;
    audio_state.input_rb.tail = 0;
    audio_state.input_rb.count = 0;
    
    /* 初始化输出环形缓冲区 */
    audio_state.output_rb.buffer = audio_output_buffer;
    audio_state.output_rb.size = AUDIO_RING_BUFFER_FRAMES;
    audio_state.output_rb.head = 0;
    audio_state.output_rb.tail = 0;
    audio_state.output_rb.count = 0;
    
    /* 清零统计 */
    audio_state.underrun_count = 0;
    audio_state.overrun_count = 0;
    audio_state.frame_count = 0;
    
    /* 清零缓冲区 */
    memset(sai_rx_buffer, 0, sizeof(sai_rx_buffer));
    memset(sai_tx_buffer, 0, sizeof(sai_tx_buffer));
}

/* 写入帧到输入缓冲区 */
bool audio_input_write(const audio_frame_t *frame)
{
    if (audio_state.input_rb.count >= audio_state.input_rb.size) {
        audio_state.overrun_count++;
        return false;
    }
    
    memcpy(&audio_state.input_rb.buffer[audio_state.input_rb.head], 
           frame, sizeof(audio_frame_t));
    audio_state.input_rb.head = (audio_state.input_rb.head + 1) % 
                                 audio_state.input_rb.size;
    __atomic_fetch_add(&audio_state.input_rb.count, 1, __ATOMIC_SEQ_CST);
    
    return true;
}

/* 从输出缓冲区读取帧 */
bool audio_output_read(audio_frame_t *frame)
{
    if (audio_state.output_rb.count == 0) {
        audio_state.underrun_count++;
        return false;
    }
    
    memcpy(frame, &audio_state.output_rb.buffer[audio_state.output_rb.tail],
           sizeof(audio_frame_t));
    audio_state.output_rb.tail = (audio_state.output_rb.tail + 1) % 
                                  audio_state.output_rb.size;
    __atomic_fetch_sub(&audio_state.output_rb.count, 1, __ATOMIC_SEQ_CST);
    
    return true;
}

/* 获取缓冲区状态 */
uint32_t audio_input_available(void) { return audio_state.input_rb.count; }
uint32_t audio_output_available(void) { return audio_state.output_rb.count; }
uint32_t audio_get_underruns(void) { return audio_state.underrun_count; }
uint32_t audio_get_overruns(void) { return audio_state.overrun_count; }
```

### 3. ADC 缓冲区定义

```c
/* adc_buffer_u5.c - STM32U5 ADC 缓冲区 */
#include "buffer_allocation.h"
#include <stdint.h>
#include <string.h>

/* ADC 缓冲区大小 */
#define ADC_DMA_BUFFER_SIZE     (ADC_DMA_BUFFER_SAMPLES * ADC_CHANNELS * sizeof(uint16_t))
#define ADC_RING_BUFFER_SAMPLES 16384

/* ADC 数据结构 */
typedef struct {
    uint16_t ch[ADC_CHANNELS];
} adc_sample_t;

/* ==================== DMA 缓冲区 - SRAM2 ==================== */

__attribute__((section(".sram2"), aligned(4)))
static uint16_t adc_dma_buffer[ADC_DMA_BUFFER_SAMPLES * ADC_CHANNELS];

/* ==================== 环形缓冲区 - SRAM3 ==================== */

__attribute__((section(".sram3"), aligned(4)))
static adc_sample_t adc_ring_buffer[ADC_RING_BUFFER_SAMPLES];

/* ==================== 处理缓冲区 - SRAM1 ==================== */

__attribute__((section(".adc_cache")))
static float adc_process_buffer[ADC_DMA_BUFFER_SAMPLES];

__attribute__((section(".adc_cache")))
static float adc_filter_buffer[ADC_DMA_BUFFER_SAMPLES];

/* ==================== 状态结构 - SRAM1 ==================== */

__attribute__((section(".adc_cache")))
static struct {
    adc_sample_t *buffer;
    uint32_t size;
    volatile uint32_t head;
    volatile uint32_t tail;
    volatile uint32_t count;
    volatile uint32_t overflow_count;
} adc_ringbuf;

/* ==================== 实现 ==================== */

void adc_buffer_init(void)
{
    adc_ringbuf.buffer = adc_ring_buffer;
    adc_ringbuf.size = ADC_RING_BUFFER_SAMPLES;
    adc_ringbuf.head = 0;
    adc_ringbuf.tail = 0;
    adc_ringbuf.count = 0;
    adc_ringbuf.overflow_count = 0;
    
    memset(adc_dma_buffer, 0, sizeof(adc_dma_buffer));
}

uint16_t *adc_get_dma_buffer(void)
{
    return adc_dma_buffer;
}

float *adc_get_process_buffer(void)
{
    return adc_process_buffer;
}

float *adc_get_filter_buffer(void)
{
    return adc_filter_buffer;
}

bool adc_ringbuf_write_block(const adc_sample_t *data, uint32_t count)
{
    if (adc_ringbuf.count + count > adc_ringbuf.size) {
        adc_ringbuf.overflow_count++;
        return false;
    }
    
    for (uint32_t i = 0; i < count; i++) {
        adc_ringbuf.buffer[adc_ringbuf.head] = data[i];
        adc_ringbuf.head = (adc_ringbuf.head + 1) % adc_ringbuf.size;
    }
    
    __atomic_fetch_add(&adc_ringbuf.count, count, __ATOMIC_SEQ_CST);
    return true;
}

bool adc_ringbuf_read_block(adc_sample_t *data, uint32_t count)
{
    if (adc_ringbuf.count < count) {
        return false;
    }
    
    for (uint32_t i = 0; i < count; i++) {
        data[i] = adc_ringbuf.buffer[adc_ringbuf.tail];
        adc_ringbuf.tail = (adc_ringbuf.tail + 1) % adc_ringbuf.size;
    }
    
    __atomic_fetch_sub(&adc_ringbuf.count, count, __ATOMIC_SEQ_CST);
    return true;
}

uint32_t adc_ringbuf_available(void)
{
    return adc_ringbuf.count;
}

uint32_t adc_get_overflow_count(void)
{
    return adc_ringbuf.overflow_count;
}
```

### 4. USB 缓冲区定义

```c
/* usb_buffer_u5.c - STM32U5 USB 缓冲区 */
#include "buffer_allocation.h"
#include <stdint.h>
#include <string.h>

/* USB 缓冲区大小 */
#define USB_EP_BUFFER_SIZE      (USB_FS_MAX_PACKET_SIZE * 4)
#define USB_TX_BUFFER_SIZE      USB_BUFFER_SIZE
#define USB_RX_BUFFER_SIZE      USB_BUFFER_SIZE

/* ==================== 端点缓冲区 - SRAM2 ==================== */

__attribute__((section(".sram2"), aligned(4)))
static uint8_t usb_ep0_buffer[USB_FS_MAX_PACKET_SIZE];

__attribute__((section(".sram2"), aligned(4)))
static uint8_t usb_ep1_in_buffer[USB_EP_BUFFER_SIZE];

__attribute__((section(".sram2"), aligned(4)))
static uint8_t usb_ep1_out_buffer[USB_EP_BUFFER_SIZE];

__attribute__((section(".sram2"), aligned(4)))
static uint8_t usb_ep2_in_buffer[USB_FS_MAX_PACKET_SIZE];

/* ==================== 数据缓冲区 - SRAM3 ==================== */

__attribute__((section(".sram3"), aligned(4)))
static uint8_t usb_tx_buffer[USB_TX_BUFFER_SIZE];

__attribute__((section(".sram3"), aligned(4)))
static uint8_t usb_rx_buffer[USB_RX_BUFFER_SIZE];

/* ==================== 环形缓冲区状态 - SRAM1 ==================== */

typedef struct {
    uint8_t *buffer;
    uint32_t size;
    volatile uint32_t head;
    volatile uint32_t tail;
    volatile uint32_t count;
} usb_ringbuf_t;

__attribute__((section(".usb_ctrl")))
static usb_ringbuf_t tx_ringbuf;

__attribute__((section(".usb_ctrl")))
static usb_ringbuf_t rx_ringbuf;

/* ==================== 实现 ==================== */

void usb_buffer_init(void)
{
    tx_ringbuf.buffer = usb_tx_buffer;
    tx_ringbuf.size = USB_TX_BUFFER_SIZE;
    tx_ringbuf.head = 0;
    tx_ringbuf.tail = 0;
    tx_ringbuf.count = 0;
    
    rx_ringbuf.buffer = usb_rx_buffer;
    rx_ringbuf.size = USB_RX_BUFFER_SIZE;
    rx_ringbuf.head = 0;
    rx_ringbuf.tail = 0;
    rx_ringbuf.count = 0;
}

uint8_t *usb_get_ep_buffer(uint8_t ep_num, uint8_t is_in)
{
    switch (ep_num) {
        case 0: return usb_ep0_buffer;
        case 1: return is_in ? usb_ep1_in_buffer : usb_ep1_out_buffer;
        case 2: return usb_ep2_in_buffer;
        default: return NULL;
    }
}

usb_ringbuf_t *usb_get_tx_ringbuf(void) { return &tx_ringbuf; }
usb_ringbuf_t *usb_get_rx_ringbuf(void) { return &rx_ringbuf; }

bool usb_ringbuf_write(usb_ringbuf_t *rb, const uint8_t *data, uint32_t len)
{
    if (rb->count + len > rb->size) return false;
    
    for (uint32_t i = 0; i < len; i++) {
        rb->buffer[rb->head] = data[i];
        rb->head = (rb->head + 1) % rb->size;
    }
    
    __atomic_fetch_add(&rb->count, len, __ATOMIC_SEQ_CST);
    return true;
}

bool usb_ringbuf_read(usb_ringbuf_t *rb, uint8_t *data, uint32_t len)
{
    if (rb->count < len) return false;
    
    for (uint32_t i = 0; i < len; i++) {
        data[i] = rb->buffer[rb->tail];
        rb->tail = (rb->tail + 1) % rb->size;
    }
    
    __atomic_fetch_sub(&rb->count, len, __ATOMIC_SEQ_CST);
    return true;
}

uint32_t usb_ringbuf_available(usb_ringbuf_t *rb) { return rb->count; }
uint32_t usb_ringbuf_free(usb_ringbuf_t *rb) { return rb->size - rb->count; }
```

------

## 五、SAI 音频驱动 (STM32U5)

```c
/* sai_audio_u5.c - STM32U5 SAI 音频驱动 */
#include "stm32u5xx_hal.h"
#include "buffer_allocation.h"

/* 外部缓冲区 */
extern uint8_t sai_rx_buffer[];
extern uint8_t sai_tx_buffer[];

/* SAI 和 DMA 句柄 */
static SAI_HandleTypeDef hsai_rx;
static SAI_HandleTypeDef hsai_tx;
static DMA_HandleTypeDef hdma_sai_rx;
static DMA_HandleTypeDef hdma_sai_tx;

/* 回调函数 */
static void (*audio_rx_half_cb)(void) = NULL;
static void (*audio_rx_full_cb)(void) = NULL;
static void (*audio_tx_half_cb)(void) = NULL;
static void (*audio_tx_full_cb)(void) = NULL;

/*
 * SAI 时钟配置
 */
static void SAI_ClockConfig(void)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    
    /* 配置 SAI1 时钟源 - 使用 PLL2 */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_SAI1;
    PeriphClkInit.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLL2;
    
    /* PLL2 配置: 产生精确的音频时钟 */
    /* 假设 HSE = 16MHz */
    /* PLL2: 16MHz / 2 * 48 / 4 = 96MHz */
    /* SAI MCLK = 96MHz / 2 = 48MHz -> 可产生 48kHz 采样率 */
    PeriphClkInit.PLL2.PLL2Source = RCC_PLLSOURCE_HSE;
    PeriphClkInit.PLL2.PLL2M = 2;
    PeriphClkInit.PLL2.PLL2N = 48;
    PeriphClkInit.PLL2.PLL2P = 4;
    PeriphClkInit.PLL2.PLL2Q = 2;
    PeriphClkInit.PLL2.PLL2R = 2;
    PeriphClkInit.PLL2.PLL2RGE = RCC_PLLVCIRANGE_1;
    PeriphClkInit.PLL2.PLL2FRACN = 0;
    PeriphClkInit.PLL2.PLL2ClockOut = RCC_PLL2_DIVP;
    
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
}

/*
 * SAI 初始化
 */
void SAI_Audio_Init(void)
{
    /* 配置时钟 */
    SAI_ClockConfig();
    
    /* 使能 SAI 时钟 */
    __HAL_RCC_SAI1_CLK_ENABLE();
    
    /* ==================== SAI RX 配置 (SAI1_A) ==================== */
    
    hsai_rx.Instance = SAI1_Block_A;
    hsai_rx.Init.AudioMode = SAI_MODEMASTER_RX;
    hsai_rx.Init.Synchro = SAI_ASYNCHRONOUS;
    hsai_rx.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;
    hsai_rx.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
    hsai_rx.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_1QF;
    hsai_rx.Init.AudioFrequency = AUDIO_SAMPLE_RATE;
    hsai_rx.Init.SynchroExt = SAI_SYNCEXT_DISABLE;
    hsai_rx.Init.MckOutput = SAI_MCK_OUTPUT_ENABLE;
    hsai_rx.Init.MonoStereoMode = SAI_STEREOMODE;
    hsai_rx.Init.CompandingMode = SAI_NOCOMPANDING;
    hsai_rx.Init.Protocol = SAI_FREE_PROTOCOL;
    hsai_rx.Init.DataSize = SAI_DATASIZE_24;
    hsai_rx.Init.FirstBit = SAI_FIRSTBIT_MSB;
    hsai_rx.Init.ClockStrobing = SAI_CLOCKSTROBING_FALLINGEDGE;
    
    /* 帧配置 - I2S 格式 */
    hsai_rx.FrameInit.FrameLength = 64;
    hsai_rx.FrameInit.ActiveFrameLength = 32;
    hsai_rx.FrameInit.FSDefinition = SAI_FS_CHANNEL_IDENTIFICATION;
    hsai_rx.FrameInit.FSPolarity = SAI_FS_ACTIVE_LOW;
    hsai_rx.FrameInit.FSOffset = SAI_FS_BEFOREFIRSTBIT;
    
    /* 时隙配置 */
    hsai_rx.SlotInit.FirstBitOffset = 0;
    hsai_rx.SlotInit.SlotSize = SAI_SLOTSIZE_32B;
    hsai_rx.SlotInit.SlotNumber = 2;
    hsai_rx.SlotInit.SlotActive = SAI_SLOTACTIVE_0 | SAI_SLOTACTIVE_1;
    
    HAL_SAI_Init(&hsai_rx);
    
    /* ==================== SAI TX 配置 (SAI1_B) ==================== */
    
    hsai_tx.Instance = SAI1_Block_B;
    hsai_tx.Init.AudioMode = SAI_MODESLAVE_TX;
    hsai_tx.Init.Synchro = SAI_SYNCHRONOUS;
    hsai_tx.Init.OutputDrive = SAI_OUTPUTDRIVE_ENABLE;
    hsai_tx.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_1QF;
    hsai_tx.Init.SynchroExt = SAI_SYNCEXT_DISABLE;
    hsai_tx.Init.MckOutput = SAI_MCK_OUTPUT_DISABLE;
    hsai_tx.Init.MonoStereoMode = SAI_STEREOMODE;
    hsai_tx.Init.CompandingMode = SAI_NOCOMPANDING;
    hsai_tx.Init.Protocol = SAI_FREE_PROTOCOL;
    hsai_tx.Init.DataSize = SAI_DATASIZE_24;
    hsai_tx.Init.FirstBit = SAI_FIRSTBIT_MSB;
    hsai_tx.Init.ClockStrobing = SAI_CLOCKSTROBING_FALLINGEDGE;
    
    hsai_tx.FrameInit.FrameLength = 64;
    hsai_tx.FrameInit.ActiveFrameLength = 32;
    hsai_tx.FrameInit.FSDefinition = SAI_FS_CHANNEL_IDENTIFICATION;
    hsai_tx.FrameInit.FSPolarity = SAI_FS_ACTIVE_LOW;
    hsai_tx.FrameInit.FSOffset = SAI_FS_BEFOREFIRSTBIT;
    
    hsai_tx.SlotInit.FirstBitOffset = 0;
    hsai_tx.SlotInit.SlotSize = SAI_SLOTSIZE_32B;
    hsai_tx.SlotInit.SlotNumber = 2;
    hsai_tx.SlotInit.SlotActive = SAI_SLOTACTIVE_0 | SAI_SLOTACTIVE_1;
    
    HAL_SAI_Init(&hsai_tx);
    
    /* ==================== GPDMA RX 配置 ==================== */
    
    __HAL_RCC_GPDMA1_CLK_ENABLE();
    
    hdma_sai_rx.Instance = GPDMA1_Channel0;
    hdma_sai_rx.Init.Request = GPDMA1_REQUEST_SAI1_A;
    hdma_sai_rx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    hdma_sai_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_sai_rx.Init.SrcInc = DMA_SINC_FIXED;
    hdma_sai_rx.Init.DestInc = DMA_DINC_INCREMENTED;
    hdma_sai_rx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_WORD;
    hdma_sai_rx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_WORD;
    hdma_sai_rx.Init.Priority = DMA_HIGH_PRIORITY;
    hdma_sai_rx.Init.SrcBurstLength = 4;
    hdma_sai_rx.Init.DestBurstLength = 4;
    hdma_sai_rx.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | 
                                              DMA_DEST_ALLOCATED_PORT1;
    hdma_sai_rx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    hdma_sai_rx.Init.Mode = DMA_NORMAL;
    
    HAL_DMA_Init(&hdma_sai_rx);
    __HAL_LINKDMA(&hsai_rx, hdmarx, hdma_sai_rx);
    
    /* ==================== GPDMA TX 配置 ==================== */
    
    hdma_sai_tx.Instance = GPDMA1_Channel1;
    hdma_sai_tx.Init.Request = GPDMA1_REQUEST_SAI1_B;
    hdma_sai_tx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    hdma_sai_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_sai_tx.Init.SrcInc = DMA_SINC_INCREMENTED;
    hdma_sai_tx.Init.DestInc = DMA_DINC_FIXED;
    hdma_sai_tx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_WORD;
    hdma_sai_tx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_WORD;
    hdma_sai_tx.Init.Priority = DMA_HIGH_PRIORITY;
    hdma_sai_tx.Init.SrcBurstLength = 4;
    hdma_sai_tx.Init.DestBurstLength = 4;
    hdma_sai_tx.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | 
                                              DMA_DEST_ALLOCATED_PORT1;
    hdma_sai_tx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    hdma_sai_tx.Init.Mode = DMA_NORMAL;
    
    HAL_DMA_Init(&hdma_sai_tx);
    __HAL_LINKDMA(&hsai_tx, hdmatx, hdma_sai_tx);
    
    /* ==================== 中断配置 ==================== */
    
    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);
    
    HAL_NVIC_SetPriority(GPDMA1_Channel1_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel1_IRQn);
}

/*
 * 注册回调
 */
void SAI_Audio_RegisterCallbacks(void (*rx_half)(void), void (*rx_full)(void),
                                  void (*tx_half)(void), void (*tx_full)(void))
{
    audio_rx_half_cb = rx_half;
    audio_rx_full_cb = rx_full;
    audio_tx_half_cb = tx_half;
    audio_tx_full_cb = tx_full;
}

/*
 * 启动音频传输
 */
void SAI_Audio_Start(void)
{
    /* 清零发送缓冲区 */
    memset(sai_tx_buffer, 0, AUDIO_DMA_BUFFER_SIZE);
    
    /* 启动 DMA 传输 */
    HAL_SAI_Receive_DMA(&hsai_rx, sai_rx_buffer, 
                        AUDIO_DMA_BUFFER_SIZE / sizeof(uint32_t));
    HAL_SAI_Transmit_DMA(&hsai_tx, sai_tx_buffer, 
                         AUDIO_DMA_BUFFER_SIZE / sizeof(uint32_t));
}

/*
 * 停止音频传输
 */
void SAI_Audio_Stop(void)
{
    HAL_SAI_DMAStop(&hsai_rx);
    HAL_SAI_DMAStop(&hsai_tx);
}

/*
 * 获取缓冲区指针
 */
uint8_t *SAI_Audio_GetRxBuffer(uint8_t half)
{
    return half ? (sai_rx_buffer + AUDIO_DMA_BUFFER_SIZE / 2) : sai_rx_buffer;
}

uint8_t *SAI_Audio_GetTxBuffer(uint8_t half)
{
    return half ? (sai_tx_buffer + AUDIO_DMA_BUFFER_SIZE / 2) : sai_tx_buffer;
}

/* ==================== 中断回调 ==================== */

void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    if (hsai == &hsai_rx && audio_rx_half_cb) {
        audio_rx_half_cb();
    }
}

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
    if (hsai == &hsai_rx && audio_rx_full_cb) {
        audio_rx_full_cb();
    }
}

void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    if (hsai == &hsai_tx && audio_tx_half_cb) {
        audio_tx_half_cb();
    }
}

void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{
    if (hsai == &hsai_tx && audio_tx_full_cb) {
        audio_tx_full_cb();
    }
}

/* ==================== 中断处理函数 ==================== */

void GPDMA1_Channel0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_sai_rx);
}

void GPDMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_sai_tx);
}
```

------

## 六、ADC 驱动 (STM32U5)

```c
/* adc_driver_u5.c - STM32U5 ADC 驱动 */
#include "stm32u5xx_hal.h"
#include "buffer_allocation.h"

extern uint16_t *adc_get_dma_buffer(void);

static ADC_HandleTypeDef hadc1;
static DMA_HandleTypeDef hdma_adc1;
static TIM_HandleTypeDef htim2;

static void (*adc_half_cb)(void) = NULL;
static void (*adc_full_cb)(void) = NULL;

/*
 * 定时器触发配置
 */
static void ADC_Timer_Init(uint32_t sample_rate)
{
    uint32_t timer_clock = HAL_RCC_GetPCLK1Freq();
    uint32_t period = timer_clock / sample_rate - 1;
    
    __HAL_RCC_TIM2_CLK_ENABLE();
    
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 0;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = period;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&htim2);
    
    TIM_MasterConfigTypeDef master_config = {0};
    master_config.MasterOutputTrigger = TIM_TRGO_UPDATE;
    master_config.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim2, &master_config);
}

/*
 * ADC 初始化
 */
void ADC_HighSpeed_Init(uint32_t sample_rate)
{
    ADC_ChannelConfTypeDef channel_config = {0};
    
    /* 初始化定时器 */
    ADC_Timer_Init(sample_rate);
    
    /* 使能 ADC 时钟 */
    __HAL_RCC_ADC1_CLK_ENABLE();
    
    /* ADC 配置 */
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_14B;  /* STM32U5 支持 14 位 */
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.NbrOfConversion = ADC_CHANNELS;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIG_T2_TRGO;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc1.Init.DMAContinuousRequests = ENABLE;
    hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    hadc1.Init.OversamplingMode = DISABLE;
    
    HAL_ADC_Init(&hadc1);
    
    /* 配置通道 */
    channel_config.SamplingTime = ADC_SAMPLETIME_5CYCLES;
    channel_config.SingleDiff = ADC_SINGLE_ENDED;
    channel_config.OffsetNumber = ADC_OFFSET_NONE;
    channel_config.Offset = 0;
    
    for (int i = 0; i < ADC_CHANNELS; i++) {
        channel_config.Channel = ADC_CHANNEL_0 + i;
        channel_config.Rank = ADC_REGULAR_RANK_1 + i;
        HAL_ADC_ConfigChannel(&hadc1, &channel_config);
    }
    
    /* ADC 校准 */
    HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
    
    /* ==================== GPDMA 配置 ==================== */
    
    __HAL_RCC_GPDMA1_CLK_ENABLE();
    
    hdma_adc1.Instance = GPDMA1_Channel2;
    hdma_adc1.Init.Request = GPDMA1_REQUEST_ADC1;
    hdma_adc1.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.SrcInc = DMA_SINC_FIXED;
    hdma_adc1.Init.DestInc = DMA_DINC_INCREMENTED;
    hdma_adc1.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_HALFWORD;
    hdma_adc1.Init.DestDataWidth = DMA_DEST_DATAWIDTH_HALFWORD;
    hdma_adc1.Init.Priority = DMA_HIGH_PRIORITY;
    hdma_adc1.Init.SrcBurstLength = 1;
    hdma_adc1.Init.DestBurstLength = 1;
    hdma_adc1.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | 
                                            DMA_DEST_ALLOCATED_PORT1;
    hdma_adc1.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    hdma_adc1.Init.Mode = DMA_NORMAL;
    
    HAL_DMA_Init(&hdma_adc1);
    __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);
    
    /* 中断配置 */
    HAL_NVIC_SetPriority(GPDMA1_Channel2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel2_IRQn);
}

/*
 * 注册回调
 */
void ADC_HighSpeed_RegisterCallbacks(void (*half_cb)(void), void (*full_cb)(void))
{
    adc_half_cb = half_cb;
    adc_full_cb = full_cb;
}

/*
 * 启动采集
 */
void ADC_HighSpeed_Start(void)
{
    uint16_t *buffer = adc_get_dma_buffer();
    
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)buffer, 
                      ADC_DMA_BUFFER_SAMPLES * ADC_CHANNELS);
    HAL_TIM_Base_Start(&htim2);
}

/*
 * 停止采集
 */
void ADC_HighSpeed_Stop(void)
{
    HAL_TIM_Base_Stop(&htim2);
    HAL_ADC_Stop_DMA(&hadc1);
}

/*
 * 获取缓冲区指针
 */
uint16_t *ADC_HighSpeed_GetBuffer(uint8_t half)
{
    uint16_t *buffer = adc_get_dma_buffer();
    return half ? (buffer + ADC_DMA_BUFFER_SAMPLES * ADC_CHANNELS / 2) : buffer;
}

/* ==================== 中断回调 ==================== */

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc == &hadc1 && adc_half_cb) {
        adc_half_cb();
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc == &hadc1 && adc_full_cb) {
        adc_full_cb();
    }
}

void GPDMA1_Channel2_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_adc1);
}
```

------

## 七、KEIL 和 IAR 配置

### 1. KEIL 分散加载文件

```text
; stm32u575_audio_adc_usb.sct - STM32U575 分散加载文件

LR_IROM1 0x08000000 0x00200000 {

    ; ==================== Flash Bank1 - 主程序 ====================
    ER_IROM1 0x08000000 0x00100000 {
        *.o (RESET, +First)
        *(InRoot$$Sections)
        .ANY (+RO)
    }
    
    ; ==================== Flash Bank2 - OTA/配置 ====================
    ER_IROM2 0x08100000 0x000E0000 {
        ; OTA 固件区域 (预留)
    }
    
    ER_CONFIG 0x081E0000 0x00020000 {
        *(.config_data)
    }
    
    ; ==================== SRAM1 - 主数据区 ====================
    RW_SRAM1 0x20000000 0x00030000 {
        .ANY (+RW +ZI)
        
        ; RAM 执行代码 (STM32U5 无 ITCM)
        *(RAMFUNC)
        *(.ramfunc*)
        
        ; 音频状态
        *(AUDIO_STATE)
        *(.audio_state)
        *(.biquad_state)
        *(.fir_state)
        
        ; ADC 缓存
        *(ADC_CACHE)
        *(.adc_cache)
        
        ; USB 控制
        *(USB_CTRL)
        *(.usb_ctrl)
    }
    
    ; ==================== SRAM2 - DMA 缓冲区 ====================
    RW_SRAM2 0x20030000 UNINIT 0x00010000 {
        ; SAI DMA
        *(SAI_DMA)
        *(.sai_dma)
        *(.sai_tx_buffer)
        *(.sai_rx_buffer)
        
        ; ADC DMA
        *(ADC_DMA)
        *(.adc_dma)
        
        ; USB DMA
        *(USB_DMA)
        *(.usb_dma)
        *(.usb_ep_buffer)
        
        ; 通用 DMA
        *(SRAM2)
        *(.sram2)
        *(.dma_buffer)
    }
    
    ; ==================== SRAM3 - 大缓冲区 ====================
    RW_SRAM3 0x20040000 UNINIT 0x00080000 {
        ; 音频缓冲
        *(AUDIO_BUFFER)
        *(.audio_buffer)
        *(.audio_ringbuf)
        *(.fft_buffer)
        
        ; USB 数据
        *(USB_DATA)
        *(.usb_data)
        
        ; 帧缓冲
        *(FRAMEBUFFER)
        *(.framebuffer)
        
        ; 通用大缓冲
        *(SRAM3)
        *(.sram3)
    }
    
    ; ==================== SRAM4 - 备份域 ====================
    RW_SRAM4 0x28000000 UNINIT 0x00004000 {
        *(SRAM4)
        *(.sram4)
        *(.backup_data)
        *(.lowpower_data)
    }
}
```

### 2. IAR ICF 文件

```icf
/*###ICF### STM32U575 音频/ADC/USB 配置 ###*/

/*-Memory Regions-*/
define symbol __ICFEDIT_region_FLASH_start__  = 0x08000000;
define symbol __ICFEDIT_region_FLASH_end__    = 0x081FFFFF;
define symbol __ICFEDIT_region_SRAM1_start__  = 0x20000000;
define symbol __ICFEDIT_region_SRAM1_end__    = 0x2002FFFF;
define symbol __ICFEDIT_region_SRAM2_start__  = 0x20030000;
define symbol __ICFEDIT_region_SRAM2_end__    = 0x2003FFFF;
define symbol __ICFEDIT_region_SRAM3_start__  = 0x20040000;
define symbol __ICFEDIT_region_SRAM3_end__    = 0x200BFFFF;
define symbol __ICFEDIT_region_SRAM4_start__  = 0x28000000;
define symbol __ICFEDIT_region_SRAM4_end__    = 0x28003FFF;

/*-Sizes-*/
define symbol __ICFEDIT_size_cstack__ = 0x2000;
define symbol __ICFEDIT_size_heap__   = 0x4000;

define memory mem with size = 4G;

define region FLASH_region = mem:[from __ICFEDIT_region_FLASH_start__ 
                                  to __ICFEDIT_region_FLASH_end__];
define region SRAM1_region = mem:[from __ICFEDIT_region_SRAM1_start__ 
                                  to __ICFEDIT_region_SRAM1_end__];
define region SRAM2_region = mem:[from __ICFEDIT_region_SRAM2_start__ 
                                  to __ICFEDIT_region_SRAM2_end__];
define region SRAM3_region = mem:[from __ICFEDIT_region_SRAM3_start__ 
                                  to __ICFEDIT_region_SRAM3_end__];
define region SRAM4_region = mem:[from __ICFEDIT_region_SRAM4_start__ 
                                  to __ICFEDIT_region_SRAM4_end__];

define block CSTACK with alignment = 8, size = __ICFEDIT_size_cstack__ { };
define block HEAP   with alignment = 8, size = __ICFEDIT_size_heap__   { };

/* 初始化策略 */
initialize by copy { readwrite };
initialize by copy { section .ramfunc };

/* 不初始化的段 */
do not initialize { section .noinit };
do not initialize { section .sram2, section .sai_dma, section .adc_dma };
do not initialize { section .sram3, section .audio_buffer, section .framebuffer };
do not initialize { section .sram4, section .backup_data };

/* 放置规则 */
place at address mem:0x08000000 { readonly section .intvec };

place in FLASH_region { readonly };
place in SRAM1_region { readwrite, section .ramfunc, block CSTACK, block HEAP };

/* SRAM2 - DMA 缓冲区 */
place in SRAM2_region { section .sram2, section .sai_dma, section .sai_tx_buffer,
                        section .sai_rx_buffer, section .adc_dma, 
                        section .usb_ep_buffer, section .dma_buffer };

/* SRAM3 - 大缓冲区 */
place in SRAM3_region { section .sram3, section .audio_buffer, section .audio_ringbuf,
                        section .fft_buffer, section .usb_data, section .framebuffer };

/* SRAM4 - 备份域 */
place in SRAM4_region { section .sram4, section .backup_data, section .lowpower_data };
```

------

## 八、主程序和系统初始化

```c
/* main_u5.c - STM32U5 主程序 */
#include "stm32u5xx_hal.h"

/* 外部函数声明 */
extern void audio_buffer_init(void);
extern void adc_buffer_init(void);
extern void usb_buffer_init(void);
extern void SAI_Audio_Init(void);
extern void SAI_Audio_Start(void);
extern void ADC_HighSpeed_Init(uint32_t sample_rate);
extern void ADC_HighSpeed_Start(void);
extern void USB_Device_Init(void);

/*
 * 系统时钟配置 - 160MHz
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    /* 配置电源 */
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
    
    /* 配置 HSE 和 PLL */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 1;    /* 假设 HSE = 16MHz */
    RCC_OscInitStruct.PLL.PLLN = 20;   /* 16MHz * 20 = 320MHz VCO */
    RCC_OscInitStruct.PLL.PLLP = 2;    /* 320MHz / 2 = 160MHz */
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_1;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    
    /* 配置系统时钟 */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |
                                  RCC_CLOCKTYPE_PCLK3;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
}

/*
 * I-Cache 配置
 */
void ICACHE_Config(void)
{
    /* STM32U5 只有 I-Cache，无 D-Cache */
    HAL_ICACHE_Enable();
}

/*
 * 低功耗配置
 */
void LowPower_Config(void)
{
    /* 配置 SRAM 保持 */
    HAL_PWREx_EnableSRAM2ContentStandbyRetention(PWR_SRAM2_FULL_STANDBY_RETENTION);
    HAL_PWREx_EnableBkupRAMRetention();
    
    /* 配置唤醒源 */
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1_HIGH);
}

/*
 * 主函数
 */
int main(void)
{
    /* HAL 初始化 */
    HAL_Init();
    
    /* 系统时钟配置 */
    SystemClock_Config();
    
    /* I-Cache 配置 */
    ICACHE_Config();
    
    /* 低功耗配置 */
    LowPower_Config();
    
    /* ==================== 初始化缓冲区 ==================== */
    
    audio_buffer_init();
    adc_buffer_init();
    usb_buffer_init();
    
    /* ==================== 初始化外设 ==================== */
    
    SAI_Audio_Init();
    ADC_HighSpeed_Init(ADC_SAMPLE_RATE);
    USB_Device_Init();
    
    /* ==================== 启动系统 ==================== */
    
    SAI_Audio_Start();
    ADC_HighSpeed_Start();
    
    /* ==================== 主循环 ==================== */
    
    while (1) {
        /* 音频处理 */
        audio_process_task();
        
        /* ADC 处理 */
        adc_process_task();
        
        /* USB 处理 */
        usb_process_task();
        
        /* 系统监控 */
        system_monitor_task();
    }
}

/*
 * 错误处理
 */
void Error_Handler(void)
{
    __disable_irq();
    while (1) {
        /* LED 闪烁指示错误 */
    }
}
```

------

## 九、STM32H7 vs STM32U5 配置对比总结

### 1. 关键差异对比表

|    特性    |          STM32H7           |        STM32U5        |      迁移注意事项       |
| :--------: | :------------------------: | :-------------------: | :---------------------: |
|  **内核**  |     Cortex-M7 @ 480MHz     |  Cortex-M33 @ 160MHz  |  性能降低，需优化算法   |
|  **TCM**   | ITCM (64KB) + DTCM (128KB) |          无           |  关键代码放 SRAM1 执行  |
| **Cache**  |     I-Cache + D-Cache      |      仅 I-Cache       | 无需处理 D-Cache 一致性 |
|  **DMA**   |   DMA1/2 + BDMA (域隔离)   |     GPDMA (统一)      |      简化 DMA 配置      |
|  **SRAM**  |          多域分布          |       统一编址        |      简化内存分配       |
|  **USB**   |          FS + HS           |         仅 FS         |     带宽限制 12Mbps     |
| **低功耗** |         Stop0/1/2          | Stop0/1/2/3 + Standby |     更多低功耗选项      |
|  **安全**  |             无             |       TrustZone       |      可选安全分区       |

### 2. 内存分配对比

|  数据类型  |  STM32H7 位置  |   STM32U5 位置   |    说明    |
| :--------: | :------------: | :--------------: | :--------: |
|  关键代码  |      ITCM      | SRAM1 (.ramfunc) | U5 无 TCM  |
|   栈/堆    |      DTCM      |      SRAM1       |   主 RAM   |
| 滤波器状态 |      DTCM      |      SRAM1       |  频繁访问  |
|  DMA 缓冲  | SRAM1/2 (D2域) |      SRAM2       |  DMA 专用  |
|  大缓冲区  |    AXI SRAM    |      SRAM3       |   大容量   |
| 低功耗数据 |  SRAM4 (D3域)  |  SRAM4 (备份域)  | 低功耗保持 |

### 3. 代码迁移检查清单

```text
□ 移除 D-Cache 相关代码 (SCB_CleanDCache, SCB_InvalidateDCache)
□ 将 ITCM 代码改为 .ramfunc 段
□ 将 DTCM 数据改为 SRAM1
□ 更新 DMA 配置 (DMA1/2 -> GPDMA)
□ 更新 USB 配置 (HS -> FS)
□ 更新时钟配置 (480MHz -> 160MHz)
□ 更新低功耗配置 (新增 Stop3)
□ 可选: 添加 TrustZone 配置
□ 更新链接脚本内存定义
□ 更新 MPU 配置 (简化，无需 Cache 策略)
```

------

如需更详细的特定功能迁移指南或有其他问题，请随时告诉我！