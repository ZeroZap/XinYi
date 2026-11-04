#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"
#include "xy_os.h"

#include <stddef.h>
#include <stdint.h>

#define UART_TIMEOUT_MS 1000U

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the exact source commit"
#endif

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

/* Same RAM-published verdict as the other OSAL smokes: the VCP ring drops and
   reorders bytes, so the debugger reads these globals instead. */
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

/* Shared state, one slice per concurrency scenario. The violation counter is a
   single-flag accumulator: a worker that trips any check sets it and stops, and
   the monitor reads it once the workers have all retired. */
static xy_os_semaphore_id_t g_done;
static xy_os_semaphore_id_t g_sem2;
static volatile uint32_t g_sem_active;
static xy_os_mutex_id_t g_mutex;
static volatile uint32_t g_counter;
static xy_os_msgqueue_id_t g_queue;
static volatile uint32_t g_queue_sum;
static volatile uint32_t g_queue_count;
static volatile uint32_t g_violation;

static void fail_and_done(void) {
    ++g_violation;
    (void)xy_os_semaphore_release(g_done);
    xy_os_thread_exit();
}

/* Scenario 1: three workers contend for a max-count-2 semaphore. At most two may
   hold a token at once; g_sem_active is bumped right after acquire and decremented
   right before release, so any observed value above 2 is a violation. */
static void sem_worker(void* argument) {
    uint32_t iteration;
    (void)argument;
    for (iteration = 0U; iteration < 50U; ++iteration) {
        if (xy_os_semaphore_acquire(g_sem2, XY_OS_WAIT_FOREVER) != XY_OS_OK)
            break;
        ++g_sem_active;
        if (g_sem_active > 2U)
            ++g_violation;
        (void)xy_os_delay(1);
        --g_sem_active;
        (void)xy_os_semaphore_release(g_sem2);
    }
    (void)xy_os_semaphore_release(g_done);
    xy_os_thread_exit();
}

/* Scenario 2: two workers each increment a shared counter 1000 times under the
   mutex, yielding between iterations to force interleaving. Without the mutex
   the increments would lose count; 2000 proves mutual exclusion held. */
static void mutex_worker(void* argument) {
    uint32_t iteration;
    (void)argument;
    for (iteration = 0U; iteration < 1000U; ++iteration) {
        if (xy_os_mutex_acquire(g_mutex, XY_OS_WAIT_FOREVER) != XY_OS_OK) {
            fail_and_done();
            return;
        }
        ++g_counter;
        (void)xy_os_mutex_release(g_mutex);
        (void)xy_os_thread_yield();
    }
    (void)xy_os_semaphore_release(g_done);
    xy_os_thread_exit();
}

/* Scenario 3: two producers each push ten uint32 values, one consumer drains all
   twenty. The sum 1..20 = 210 and the count must be exactly 20. */
static void producer_worker(void* argument) {
    uint32_t base = (uint32_t)(uintptr_t)argument;
    uint32_t index;
    for (index = 1U; index <= 10U; ++index) {
        uint32_t value = base + index;
        if (xy_os_msgqueue_put(g_queue, &value, 0, 100) != XY_OS_OK) {
            fail_and_done();
            return;
        }
    }
    (void)xy_os_semaphore_release(g_done);
    xy_os_thread_exit();
}

static void consumer_worker(void* argument) {
    uint32_t index;
    (void)argument;
    for (index = 0U; index < 20U; ++index) {
        uint32_t value = 0U;
        if (xy_os_msgqueue_get(g_queue, &value, NULL, 100) != XY_OS_OK) {
            fail_and_done();
            return;
        }
        g_queue_sum += value;
        ++g_queue_count;
    }
    (void)xy_os_semaphore_release(g_done);
    xy_os_thread_exit();
}

static xy_os_thread_id_t spawn(xy_os_thread_func_t func, void* argument, const char* name) {
    const xy_os_thread_attr_t attr = {.name = name, .stack_size = 1024,
                                      .priority = XY_OS_PRIORITY_NORMAL};
    return xy_os_thread_new(func, argument, &attr);
}

static void wait_workers(uint32_t count) {
    uint32_t index;
    for (index = 0U; index < count; ++index)
        (void)xy_os_semaphore_acquire(g_done, XY_OS_WAIT_FOREVER);
}

static void monitor(void* argument) {
    (void)argument;
    mark("OPENCH_OSAL_CONCURRENCY_BOOT_" XINYI_FIRMWARE_COMMIT "\r\n");
    g_done = xy_os_semaphore_new(3U, 0U, NULL);
    if (g_done == NULL) {
        mark("OPENCH_OSAL_CONCURRENCY_SETUP_ERROR_A\r\n");
        for (;;) {
        }
    }

    /* Scenario 1: counting semaphore, max count 2. */
    g_sem2 = xy_os_semaphore_new(2U, 2U, NULL);
    g_sem_active = 0U;
    g_violation = 0U;
    if (g_sem2 == NULL || spawn(sem_worker, NULL, "sem1") == NULL ||
        spawn(sem_worker, NULL, "sem2") == NULL || spawn(sem_worker, NULL, "sem3") == NULL) {
        mark("OPENCH_OSAL_CONCURRENCY_SEMAPHORE_ERROR_A\r\n");
        for (;;) {
        }
    }
    wait_workers(3U);
    if (g_violation == 0U && xy_os_semaphore_get_count(g_sem2) == 2U)
        mark("OPENCH_OSAL_CONCURRENCY_SEMAPHORE_OK_A\r\n");
    else
        mark("OPENCH_OSAL_CONCURRENCY_SEMAPHORE_ERROR_A\r\n");
    (void)xy_os_semaphore_delete(g_sem2);

    /* Scenario 2: mutex atomicity. */
    g_mutex = xy_os_mutex_new(NULL);
    g_counter = 0U;
    g_violation = 0U;
    if (g_mutex == NULL || spawn(mutex_worker, NULL, "mut1") == NULL ||
        spawn(mutex_worker, NULL, "mut2") == NULL) {
        mark("OPENCH_OSAL_CONCURRENCY_MUTEX_ERROR_A\r\n");
        for (;;) {
        }
    }
    wait_workers(2U);
    if (g_violation == 0U && g_counter == 2000U)
        mark("OPENCH_OSAL_CONCURRENCY_MUTEX_OK_A\r\n");
    else
        mark("OPENCH_OSAL_CONCURRENCY_MUTEX_ERROR_A\r\n");
    (void)xy_os_mutex_delete(g_mutex);

    /* Scenario 3: multi-producer queue. */
    g_queue = xy_os_msgqueue_new(20U, sizeof(uint32_t), NULL);
    g_queue_sum = 0U;
    g_queue_count = 0U;
    g_violation = 0U;
    if (g_queue == NULL || spawn(producer_worker, (void*)0U, "prod0") == NULL ||
        spawn(producer_worker, (void*)10U, "prod1") == NULL ||
        spawn(consumer_worker, NULL, "cons") == NULL) {
        mark("OPENCH_OSAL_CONCURRENCY_QUEUE_ERROR_A\r\n");
        for (;;) {
        }
    }
    wait_workers(3U);
    if (g_violation == 0U && g_queue_count == 20U && g_queue_sum == 210U)
        mark("OPENCH_OSAL_CONCURRENCY_QUEUE_OK_A\r\n");
    else
        mark("OPENCH_OSAL_CONCURRENCY_QUEUE_ERROR_A\r\n");
    (void)xy_os_msgqueue_delete(g_queue);

    mark("OPENCH_OSAL_CONCURRENCY_PASS_20261103_A\r\n");
    for (;;) {
        (void)xy_os_delay(1000);
    }
}

int main(void) {
    const xy_os_thread_attr_t attr = {.name = "monitor", .stack_size = 1024,
                                      .priority = XY_OS_PRIORITY_NORMAL};
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    uart_init();
    mark("OPENCH_OSAL_CONCURRENCY_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\r\n");
    (void)xy_os_kernel_init();
    if (xy_os_thread_new(monitor, NULL, &attr) == NULL) {
        mark("OPENCH_OSAL_CONCURRENCY_CREATE_ERROR_A\r\n");
        for (;;) {
        }
    }
    (void)xy_os_kernel_start();
    mark("OPENCH_OSAL_CONCURRENCY_SCHED_RETURN_ERROR_A\r\n");
    for (;;) {
    }
}
