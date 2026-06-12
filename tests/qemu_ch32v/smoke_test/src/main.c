#include <stdint.h>

#define USART1_BASE 0x40013800UL
#define USART_STATR (*(volatile uint32_t *)(USART1_BASE + 0x00UL))
#define USART_DATAR (*(volatile uint32_t *)(USART1_BASE + 0x04UL))
#define USART_CTLR1 (*(volatile uint32_t *)(USART1_BASE + 0x0CUL))

#define USART_STATR_TXE (1U << 7)
#define USART_CTLR1_TE (1U << 3)
#define USART_CTLR1_UE (1U << 13)

static void uart_putc(char c)
{
    while ((USART_STATR & USART_STATR_TXE) == 0U) {
    }
    USART_DATAR = (uint32_t)(uint8_t)c;
}

static void uart_puts(const char *s)
{
    while (*s != '\0') {
        uart_putc(*s++);
    }
}

int main(void)
{
    USART_CTLR1 = USART_CTLR1_UE | USART_CTLR1_TE;

    uart_puts("========================================\n");
    uart_puts("  XinYi CH32V307 QEMU smoke test\n");
    uart_puts("  Machine: ch32v307\n");
    uart_puts("========================================\n");
    uart_puts("[CHECK] USART1 output via qemu-system-riscv32\n");
    uart_puts("[RESULT] PASS=1 FAIL=0\n");

    for (;;) {
    }
}
