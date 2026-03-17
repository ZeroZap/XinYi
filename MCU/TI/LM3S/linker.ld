/* QEMU LM3S6965 链接脚本 */

MEMORY
{
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 256K
    RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 64K
}

/* 栈大小 */
_Min_Stack_Size = 0x400;

SECTIONS
{
    /* 向量表 */
    .isr_vector : {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } >FLASH

    /* 代码段 */
    .text : {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } >FLASH

    /* 初始化数据 */
    .data : {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } >RAM AT> FLASH

    _sidata = LOADADDR(.data);

    /* 未初始化数据 */
    .bss : {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } >RAM

    /* 栈 */
    ._stack : {
        . = ALIGN(8);
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
        _stack_top = .;
    } >RAM

    _stack_limit = ORIGIN(RAM) + LENGTH(RAM);
}
