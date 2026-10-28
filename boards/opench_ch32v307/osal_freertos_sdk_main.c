#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_timer.h"
#include "xy_hal_uart.h"
#include "FreeRTOS.h"
#include "portmacro.h"
#include "xy_os.h"

#include <stddef.h>

#define UART_TIMEOUT_MS 1000U

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the exact source commit"
#endif

static xy_os_semaphore_id_t irq_sem;
static volatile uint32_t irq_count;
static volatile uint32_t worker_count;

static void uart_init(void) {
    const xy_hal_gpio_config_t tx = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH, 0U};
    const xy_hal_gpio_config_t rx = {XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0U};
    const xy_hal_uart_config_t uart = {115200U,
                                       XY_HAL_UART_WORDLEN_8B,
                                       XY_HAL_UART_STOPBITS_1,
                                       XY_HAL_UART_PARITY_NONE,
                                       XY_HAL_UART_FLOWCTRL_NONE,
                                       XY_HAL_UART_MODE_TX_RX};
    (void)xy_hal_gpio_init(GPIOA, 9U, &tx);
    (void)xy_hal_gpio_init(GPIOA, 10U, &rx);
    (void)xy_hal_uart_init(USART1, &uart);
}

/* The WCH-Link virtual COM port drops and reorders bytes, so a marker is not
   evidence once it leaves this function. Publish the same facts in RAM, where
   the debugger can read them: how many markers were emitted, whether any of them
   was a failure, and which marker came last. "ERROR" in a marker name is this
   board's convention for a check that failed; no marker carries both ERROR and
   OK, so the substring test is unambiguous. */
volatile uint32_t xy_image_steps;
volatile uint32_t xy_image_failed;
volatile const char* xy_image_last;

static void mark(const char* text) {
    size_t length = 0U;
    size_t index;
    while (text[length] != '\0')
        ++length;
    xy_image_last = text;
    ++xy_image_steps;
    for (index = 0U; index + 5U <= length; ++index) {
        if (text[index] == 'E' && text[index + 1U] == 'R' && text[index + 2U] == 'R' &&
            text[index + 3U] == 'O' && text[index + 4U] == 'R')
            xy_image_failed = 1U;
    }
    (void)xy_hal_uart_send(USART1, (const uint8_t*)text, length, UART_TIMEOUT_MS);
}

/* The timer, its update interrupt and the NVIC line are all the HAL's now: it
   used to stop at the peripheral, which is why this image had to program TIM2
   and the NVIC by hand. */
static void timer_update_callback(void* timer, xy_hal_timer_event_t event, void* argument) {
    (void)timer;
    (void)event;
    (void)argument;
    irq_count++;
    if (xy_os_semaphore_release_from_isr(irq_sem) != XY_OS_OK)
        mark("OPENCH_OSAL_FREERTOS_ISR_RELEASE_ERROR_A\r\n");
}

static void timer_init(void) {
    const xy_hal_timer_config_t config = {.prescaler = 959U,
                                          .period = 9999U,
                                          .mode = XY_HAL_TIMER_COUNT_UP,
                                          .clock_div = XY_HAL_TIMER_CKDIV_1,
                                          .auto_reload_preload = 0U};
    (void)xy_hal_timer_init(TIM2, &config);
    (void)xy_hal_timer_register_callback(TIM2, XY_HAL_TIMER_EVENT_UPDATE, timer_update_callback,
                                        NULL);
    (void)xy_hal_timer_enable_irq(TIM2, XY_HAL_TIMER_EVENT_UPDATE);
    (void)xy_hal_timer_start(TIM2);
}

void TIM2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM2_IRQHandler(void) {
    GET_INT_SP();
    xy_hal_timer_irq_handler(TIM2);
    FREE_INT_SP();
}

static void worker_task(void* argument) {
    (void)argument;
    for (;;) {
        worker_count++;
        (void)xy_os_delay(50);
    }
}

static void irq_task(void* argument) {
    uint32_t received = 0;
    (void)argument;
    mark("OPENCH_OSAL_FREERTOS_BOOT_" XINYI_FIRMWARE_COMMIT "\r\n");
    timer_init();
    for (;;) {
        if (xy_os_semaphore_acquire(irq_sem, 500) != XY_OS_OK) {
            mark("OPENCH_OSAL_FREERTOS_TIMEOUT_ERROR_A\r\n");
            continue;
        }
        received++;
        if (received == 5U && irq_count >= 5U && worker_count >= 5U)
            mark("OPENCH_OSAL_FREERTOS_TASK_SYNC_OK_A\r\n");
        if (received == 10U && irq_count >= 10U && worker_count >= 10U)
            mark("OPENCH_OSAL_FREERTOS_FAST_ISR_OK_A\r\n");
    }
}

int main(void) {
    const xy_os_thread_attr_t worker_attr = {
        .name = "worker", .stack_size = 1024, .priority = XY_OS_PRIORITY_LOW};
    const xy_os_thread_attr_t irq_attr = {
        .name = "irq", .stack_size = 1024, .priority = XY_OS_PRIORITY_NORMAL};
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    uart_init();
    mark("OPENCH_OSAL_FREERTOS_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\r\n");
    if (xy_os_kernel_init() != XY_OS_OK) {
        mark("OPENCH_OSAL_FREERTOS_INIT_ERROR_A\r\n");
        for (;;) {
        }
    }
    irq_sem = xy_os_semaphore_new(1, 0, NULL);
    if (irq_sem == NULL || xy_os_thread_new(worker_task, NULL, &worker_attr) == NULL ||
        xy_os_thread_new(irq_task, NULL, &irq_attr) == NULL) {
        mark("OPENCH_OSAL_FREERTOS_CREATE_ERROR_A\r\n");
        for (;;) {
        }
    }
    (void)xy_os_kernel_start();
    mark("OPENCH_OSAL_FREERTOS_SCHED_RETURN_ERROR_A\r\n");
    for (;;) {
    }
}
