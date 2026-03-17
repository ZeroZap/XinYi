/**
 * @file xy_hal_lm3s.c
 * @brief XinYi HAL - TI LM3S6965 (QEMU) 实现
 * 
 * 基于 QEMU lm3s6965evb 模拟器的 HAL 驱动
 */

#include "xy_hal_lm3s.h"
#include "lm3s6965.h"

/*============================================================================
 *  系统初始化
 *===========================================================================*/

void xy_hal_system_init(void)
{
    /* 配置系统时钟 - QEMU 默认 50MHz */
    /* LM3S6965 QEMU 无需额外时钟配置 */
    
    /* 使能 GPIO 端口时钟 */
    LM3S_SYSCTL->GCC = 0x1F;  /* Enable GPIOA-GPIOH */
}

/*============================================================================
 *  GPIO 实现
 *===========================================================================*/

static lm3s_gpio_t* _gpio_port_base(uint8_t port)
{
    switch (port) {
        case 0: return LM3S_GPIOA;  /* PORTA */
        case 1: return LM3S_GPIOB;  /* PORTB */
        case 2: return LM3S_GPIOC;  /* PORTC */
        case 3: return LM3S_GPIOD;  /* PORTD */
        case 4: return LM3S_GPIOE;  /* PORTE */
        case 5: return LM3S_GPIOF;  /* PORTF */
        case 6: return LM3S_GPIOG;  /* PORTG */
        case 7: return LM3S_GPIOH;  /* PORTH */
        default: return LM3S_GPIOA;
    }
}

xy_hal_gpio_t xy_hal_gpio_bind(const char *pin_name)
{
    /* 解析引脚名称，如 "GPIOA.5" 或 "PA5" */
    uint8_t port = 0;
    uint8_t pin = 0;
    
    if (pin_name[0] == 'G' && pin_name[1] == 'P' && 
        pin_name[2] == 'I' && pin_name[3] == 'O') {
        /* 格式：GPIOx.n */
        port = pin_name[4] - 'A';
        pin = pin_name[6] - '0';
    } else if (pin_name[0] == 'P') {
        /* 格式：PXn */
        port = pin_name[1] - 'A';
        pin = pin_name[2] - '0';
    }
    
    return (xy_hal_gpio_t)((port << 8) | pin);
}

xy_hal_status_t xy_hal_gpio_configure(xy_hal_gpio_t gpio, 
                                       xy_hal_gpio_pin_t pins,
                                       const xy_hal_gpio_config_t *config)
{
    uint8_t port = (gpio >> 8) & 0xFF;
    lm3s_gpio_t *port_reg = _gpio_port_base(port);
    
    if (config == NULL) {
        return XY_HAL_STATUS_ERROR;
    }
    
    /* 配置方向 */
    if (config->mode == XY_HAL_GPIO_MODE_OUTPUT) {
        port_reg->DIR |= pins;
    } else {
        port_reg->DIR &= ~pins;
    }
    
    /* 配置上下拉 */
    if (config->pull == XY_HAL_GPIO_PULL_UP) {
        port_reg->PUR |= pins;
        port_reg->PDR &= ~pins;
    } else if (config->pull == XY_HAL_GPIO_PULL_DOWN) {
        port_reg->PUR &= ~pins;
        port_reg->PDR |= pins;
    } else {
        port_reg->PUR &= ~pins;
        port_reg->PDR &= ~pins;
    }
    
    /* 配置驱动强度 (默认 8mA) */
    port_reg->DR2R &= ~pins;
    port_reg->DR4R &= ~pins;
    port_reg->DR8R |= pins;
    
    /* 使能数字功能 */
    port_reg->DEN |= pins;
    
    /* 关闭模拟功能 */
    port_reg->AMSEL &= ~pins;
    
    return XY_HAL_STATUS_OK;
}

xy_hal_status_t xy_hal_gpio_write(xy_hal_gpio_t gpio, 
                                   xy_hal_gpio_pin_t pins,
                                   uint8_t value)
{
    uint8_t port = (gpio >> 8) & 0xFF;
    lm3s_gpio_t *port_reg = _gpio_port_base(port);
    
    if (value) {
        port_reg->DATA[pins] = pins;  /* 置位 */
    } else {
        port_reg->DATA[pins] = 0;      /* 清零 */
    }
    
    return XY_HAL_STATUS_OK;
}

xy_hal_status_t xy_hal_gpio_read(xy_hal_gpio_t gpio, 
                                  xy_hal_gpio_pin_t pins,
                                  uint8_t *value)
{
    uint8_t port = (gpio >> 8) & 0xFF;
    lm3s_gpio_t *port_reg = _gpio_port_base(port);
    
    if (value == NULL) {
        return XY_HAL_STATUS_ERROR;
    }
    
    *value = (port_reg->DATA[pins & 0xFF] & pins) ? 1 : 0;
    
    return XY_HAL_STATUS_OK;
}

xy_hal_status_t xy_hal_gpio_toggle(xy_hal_gpio_t gpio, 
                                    xy_hal_gpio_pin_t pins)
{
    uint8_t port = (gpio >> 8) & 0xFF;
    lm3s_gpio_t *port_reg = _gpio_port_base(port);
    
    /* 读取当前值并翻转 */
    uint32_t current = port_reg->DATA[pins & 0xFF] & pins;
    if (current) {
        port_reg->DATA[pins] = 0;
    } else {
        port_reg->DATA[pins] = pins;
    }
    
    return XY_HAL_STATUS_OK;
}

/*============================================================================
 *  UART 实现
 *===========================================================================*/

xy_hal_uart_t xy_hal_uart_bind(const char *uart_name)
{
    if (uart_name == NULL) {
        return XY_HAL_UART_INVALID;
    }
    
    /* 解析 UART 名称，如 "UART0" 或 "UART1" */
    if (uart_name[4] == '0') {
        return XY_HAL_UART_0;
    } else if (uart_name[4] == '1') {
        return XY_HAL_UART_1;
    } else if (uart_name[4] == '2') {
        return XY_HAL_UART_2;
    }
    
    return XY_HAL_UART_INVALID;
}

xy_hal_status_t xy_hal_uart_configure(xy_hal_uart_t uart, 
                                       const xy_hal_uart_config_t *config)
{
    lm3s_uart_t *uart_reg;
    uint32_t baud_div;
    
    if (config == NULL) {
        return XY_HAL_STATUS_ERROR;
    }
    
    /* 选择 UART 端口 */
    switch (uart) {
        case XY_HAL_UART_0:
            uart_reg = LM3S_UART0;
            break;
        case XY_HAL_UART_1:
            uart_reg = LM3S_UART1;
            break;
        case XY_HAL_UART_2:
            uart_reg = LM3S_UART2;
            break;
        default:
            return XY_HAL_STATUS_ERROR;
    }
    
    /* 禁用 UART 进行配置 */
    uart_reg->CTL = 0;
    
    /* 配置波特率 - QEMU LM3S6965 系统时钟 50MHz */
    /* baud_div = sys_clk / (16 * baudrate) */
    baud_div = 50000000 / (16 * config->baudrate);
    uart_reg->IBRD = baud_div;
    uart_reg->FBRD = 0;  /* 简化：忽略小数部分 */
    
    /* 配置数据位、停止位、校验位 */
    uart_reg->LCRH = UART_LCRH_WLEN_8 | UART_LCRH_FEN;
    
    /* 使能 UART、TX、RX */
    uart_reg->CTL = UART_CTL_UARTEN | UART_CTL_TXE | UART_CTL_RXE;
    
    return XY_HAL_STATUS_OK;
}

xy_hal_status_t xy_hal_uart_write(xy_hal_uart_t uart, const uint8_t *data, uint16_t length)
{
    lm3s_uart_t *uart_reg;
    
    /* 选择 UART 端口 */
    switch (uart) {
        case XY_HAL_UART_0:
            uart_reg = LM3S_UART0;
            break;
        case XY_HAL_UART_1:
            uart_reg = LM3S_UART1;
            break;
        case XY_HAL_UART_2:
            uart_reg = LM3S_UART2;
            break;
        default:
            return XY_HAL_STATUS_ERROR;
    }
    
    for (uint16_t i = 0; i < length; i++) {
        /* 等待 TX FIFO 不满 */
        while (uart_reg->FR & UART_FR_TXFF);
        
        /* 发送数据 */
        uart_reg->DR = data[i];
    }
    
    return XY_HAL_STATUS_OK;
}

xy_hal_status_t xy_hal_uart_read(xy_hal_uart_t uart, uint8_t *data, uint16_t length)
{
    lm3s_uart_t *uart_reg;
    
    /* 选择 UART 端口 */
    switch (uart) {
        case XY_HAL_UART_0:
            uart_reg = LM3S_UART0;
            break;
        case XY_HAL_UART_1:
            uart_reg = LM3S_UART1;
            break;
        case XY_HAL_UART_2:
            uart_reg = LM3S_UART2;
            break;
        default:
            return XY_HAL_STATUS_ERROR;
    }
    
    for (uint16_t i = 0; i < length; i++) {
        /* 等待 RX 数据 */
        while (uart_reg->FR & UART_FR_RXFE);
        
        /* 读取数据 */
        data[i] = uart_reg->DR & 0xFF;
    }
    
    return XY_HAL_STATUS_OK;
}

/*============================================================================
 *  延时实现 (使用 SysTick)
 *===========================================================================*/

void xy_hal_delay_ms(uint32_t ms)
{
    /* 配置 SysTick 为 1ms 中断 (50MHz / 1000 = 50000) */
    LM3S_SYSTICK->RELOAD = 49999;
    LM3S_SYSTICK->CURRENT = 0;
    LM3S_SYSTICK->CTRL = SYSTICK_CTRL_ENABLE | SYSTICK_CTRL_CLKSOURCE;
    
    while (ms--) {
        /* 等待计数完成 */
        while (!(LM3S_SYSTICK->CTRL & 0x10000));
    }
    
    /* 禁用 SysTick */
    LM3S_SYSTICK->CTRL = 0;
}

void xy_hal_delay_us(uint32_t us)
{
    /* 简单延时循环 - QEMU 中不精确 */
    volatile uint32_t count = us * 50;  /* 50MHz 近似 */
    while (count--);
}
