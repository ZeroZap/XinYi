#include "FreeRTOS.h"
#include "task.h"
#include "task_bank.h"

static TaskHandle_t _bank_task;

static void bank_task_process(void *arg)
{
    for (;;) {
        bank_process();
        // TODO: task delay...
    }
}

int32_t bank_task_init(void)
{
    bank_init();

    if (_bank_task == NULL) {
        if (pdPASS
            != xTaskCreate(
                bank_task_process, "bankTask", 4096, NULL,
                (configMAX_PRIORITIES - 8), &_bank_task)) {
            // LOGE(TAG, "bank task create fail.\n");
        }
    }

    bank_control(bank_ctrl_cmd_period_update_on, 100);

    return 0;
}
