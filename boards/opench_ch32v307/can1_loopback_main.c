#include "ch32v30x.h"
#include "xy_hal_can.h"
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"
#include <stddef.h>
#include <stdint.h>
static void console_write(const char* s) {
    size_t n = 0U;
    while (s[n] != '\0')
        ++n;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)s, n, 1000U);
}
int main(void) {
    const xy_hal_gpio_config_t tx = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH, 0U};
    const xy_hal_gpio_config_t rx = {XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_UP,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0U};
    const xy_hal_uart_config_t uart = {115200U,
                                       XY_HAL_UART_WORDLEN_8B,
                                       XY_HAL_UART_STOPBITS_1,
                                       XY_HAL_UART_PARITY_NONE,
                                       XY_HAL_UART_FLOWCTRL_NONE,
                                       XY_HAL_UART_MODE_TX_RX};
    const xy_hal_can_config_t can = {
        XY_HAL_CAN_BAUD_125K, XY_HAL_CAN_MODE_LOOPBACK, 1U, 5U, 2U, 1U, 0U, 1U, 0U, 0U};
    const xy_hal_can_filter_config_t filter = {
        0U, 0U, XY_HAL_CAN_FILTERMODE_IDMASK, XY_HAL_CAN_FILTERSCALE_32BIT, 0U, 1U, 0U};
    const xy_hal_can_msg_t sent = {0x123U, XY_HAL_CAN_FRAME_STD,         XY_HAL_CAN_DATA_FRAME,
                                   4U,     {0x58U, 0x59U, 0x49U, 0x21U}, 0U,
                                   0U};
    xy_hal_can_msg_t received = {0};
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap1_CAN1, ENABLE);
    if (xy_hal_gpio_init(GPIOA, 9U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &rx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOB, 9U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOB, 8U, &rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK)
        for (;;) {
        }
    console_write("OPENCH_CAN1_GPIO_READY\r\n");
    if (xy_hal_can_init(CAN1, &can) != XY_HAL_OK) {
        console_write("OPENCH_CAN1_INIT_ERROR\r\n");
        for (;;) {
        }
    }
    console_write("OPENCH_CAN1_PERIPHERAL_READY\r\n");
    if (xy_hal_can_config_filter(CAN1, &filter) != XY_HAL_OK) {
        console_write("OPENCH_CAN1_FILTER_ERROR\r\n");
        for (;;) {
        }
    }
    console_write("OPENCH_CAN1_FILTER_READY\r\n");
    if (xy_hal_can_start(CAN1) != XY_HAL_OK) {
        console_write("OPENCH_CAN1_START_ERROR\r\n");
        for (;;) {
        }
    }
    console_write("OPENCH_CAN1_STARTED\r\n");
    for (;;) {
        if (xy_hal_can_send(CAN1, &sent, 100U) == XY_HAL_OK &&
            xy_hal_can_receive(CAN1, &received, XY_HAL_CAN_FIFO_0, 100U) == XY_HAL_OK &&
            received.id == sent.id && received.dlc == sent.dlc && received.data[0] == sent.data[0])
            console_write("OPENCH_CAN1_PB8_PB9_LOOPBACK_OK\r\n");
        else
            console_write("OPENCH_CAN1_PB8_PB9_LOOPBACK_ERROR\r\n");
        for (volatile uint32_t d = 0; d < 300000U; ++d) {
        }
    }
}
