/* STM32F407VGTx 链接脚本 */
/* XinYi Framework - STM32 Test Project */

ENTRY(Reset_Handler)

/* 内存定义 */
MEMORY
{
    FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 1024K
    RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 128K
}

/* 栈大小 */
_Min_Heap_Size = 0x200;
_Min_Stack_Size = 0x400;

/* 段定义 */
SECTIONS
{
    /* 中断向量表 */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } >FLASH

    /* 代码段 */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } >FLASH

    /* 初始化数据 */
    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } >RAM AT> FLASH

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
    } >RAM

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

    /DISCARD/ :
    {
        libc.a(*)
        libm.a(*)
        libgcc.a(*)
    }
}
