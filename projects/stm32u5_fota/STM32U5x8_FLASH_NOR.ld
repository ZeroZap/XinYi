/* STM32U5x8 链接脚本 - 内部 Flash + 外部 NOR Flash
 * XinYi Framework - FOTA + FS Configuration
 *
 * 内存布局:
 * - 内部 Flash: 2MB (0x08000000 - 0x081FFFFF)
 * - 外部 NOR:   16MB (0x90000000 - 0x90FFFFFF) via OCTOSPI XIP
 * - RAM:       768KB (0x20000000 - 0x200BFFFF)
 */

/* 入口点 */
ENTRY(Reset_Handler)

/* 内存区域定义 */
MEMORY
{
    /* 内部 Flash - 分为两个槽 (FOTA 双槽模式) */
    FLASH_BOOT    (rx)    : ORIGIN = 0x08000000, LENGTH = 128K    /* Bootloader */
    FLASH_SLOT_A  (rx)    : ORIGIN = 0x08020000, LENGTH = 256K    /* App A */
    FLASH_SLOT_B  (rx)    : ORIGIN = 0x08060000, LENGTH = 256K    /* App B (备份) */
    FLASH_CONFIG  (rw)    : ORIGIN = 0x080A0000, LENGTH = 64K     /* 配置区 */

    /* 外部 NOR Flash - XIP 映射 */
    NOR_FOTA_BACKUP (rwx) : ORIGIN = 0x90000000, LENGTH = 2M     /* FOTA 备份区 */
    NOR_FS         (rwx)  : ORIGIN = 0x90400000, LENGTH = 4M     /* 文件系统 */
    NOR_DATA       (rwx)  : ORIGIN = 0x90800000, LENGTH = 4M     /* 数据存储 */
    NOR_RESERVED   (rwx)  : ORIGIN = 0x90C00000, LENGTH = 4M     /* 预留 */

    /* RAM */
    RAM (xrw)             : ORIGIN = 0x20000000, LENGTH = 768K
}

/* 栈和堆大小 */
_Min_Heap_Size = 0x400;        /* 1KB 堆 */
_Min_Stack_Size = 0x1000;      /* 4KB 栈 */

/* 段定义 */
SECTIONS
{
    /* ==================== 内部 Flash 段 ==================== */

    /* 中断向量表 - 必须放在 Flash 起始位置 */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } >FLASH_BOOT

    /* Bootloader 代码段 */
    .boot_text :
    {
        . = ALIGN(4);
        *(.boot_text)
        *(.boot_text*)
        . = ALIGN(4);
        _boot_etext = .;
    } >FLASH_BOOT

    /* App 代码段 (Slot A) */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } >FLASH_SLOT_A

    /* 初始化数据段 - 从 Flash 复制到 RAM */
    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } >RAM AT> FLASH_SLOT_A

    /* 未初始化数据段 */
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
    } >RAM

    /* 只读数据段 */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } >FLASH_SLOT_A

    /* ==================== NOR Flash 段 (XIP) ==================== */

    /* FOTA 备份区 - 可执行 */
    .nor_fota :
    {
        . = ALIGN(4);
        *(.nor_fota)
        *(.nor_fota*)
        . = ALIGN(4);
    } >NOR_FOTA_BACKUP

    /* 文件系统区 */
    .nor_fs :
    {
        . = ALIGN(4);
        *(.nor_fs)
        *(.nor_fs*)
        . = ALIGN(4);
    } >NOR_FS

    /* ==================== 系统段 ==================== */

    /* 栈 */
    ._user_stack :
    {
        . = ALIGN(8);
        _sstack = .;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
        _estack = .;
    } >RAM

    /* 堆 */
    ._user_heap :
    {
        . = ALIGN(8);
        . = . + _Min_Heap_Size;
        . = ALIGN(8);
    } >RAM

    /* 移除不需要的段 */
    /DISCARD/ :
    {
        libc.a(*)
        libm.a(*)
        libgcc.a(*)
    }
}

/* ==================== 符号定义 (供代码使用) ==================== */

/* Flash 地址 */
FLASH_BASE = 0x08000000;
FLASH_BOOT_ADDR = 0x08000000;
FLASH_SLOT_A_ADDR = 0x08020000;
FLASH_SLOT_B_ADDR = 0x08060000;
FLASH_CONFIG_ADDR = 0x080A0000;

/* NOR Flash 地址 */
NOR_BASE = 0x90000000;
NOR_FOTA_BACKUP_ADDR = 0x90000000;
NOR_FS_ADDR = 0x90400000;
NOR_DATA_ADDR = 0x90800000;

/* RAM 地址 */
RAM_BASE = 0x20000000;
RAM_SIZE = 768K;