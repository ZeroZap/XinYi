diff --git a/Alpha_U3_Base/UnitTest/ChargerTest/inc/controller-charger-test.h b/Alpha_U3_Base/UnitTest/ChargerTest/inc/controller-charger-test.h
new file mode 100644
index 0000000..3f00415
--- /dev/null
+++ b/Alpha_U3_Base/UnitTest/ChargerTest/inc/controller-charger-test.h
@@ -0,0 +1,13 @@
+/*
+ * controller-charger-test.h
+ *
+ *  Created on: Nov 22, 2024
+ *      Author: 11360919
+ */
+#ifndef _CONTROLLER_CHARGER_TEST_H_
+#define _CONTROLLER_CHARGER_TEST_H_
+
+extern void controller_charger_test_init(void);
+extern void controller_charger_test_start(void);
+
+#endif /* _CONTROLLER_CHARGER_TEST_H_ */
diff --git a/Alpha_U3_Base/UnitTest/ChargerTest/src/controller-charger-test.c b/Alpha_U3_Base/UnitTest/ChargerTest/src/controller-charger-test.c
new file mode 100644
index 0000000..97dc9d5
--- /dev/null
+++ b/Alpha_U3_Base/UnitTest/ChargerTest/src/controller-charger-test.c
@@ -0,0 +1,354 @@
+/*
+ * controller-charger-test.c
+ *
+ *  Created on: Nov 22, 2024
+ *      Author: 11360919
+ */
+#include "controller-charger-test.h"
+#include "controller-charger.h"
+#include "controller-log.h"
+#include "stm32u3xx_hal.h"
+#include "target.h"
+#include "freertos_test.h"
+#define CHG_CTLR "CHARGER_CONTROLLER"
+
+#define CONTROLLER_CHARGER_ENABLE 1
+
+#if (CONTROLLER_CHARGER_ENABLE == 0)
+void controller_charger_test_init(void)
+{
+}
+
+#else
+
+typedef enum charge_init_error {
+    charge_init_error_none = 0,
+    charge_init_error_i2c,
+    charge_init_error_gpio,
+    charge_init_error_id,
+} charge_init_error_t;
+
+typedef enum charge_target_voltage_error {
+    charge_target_voltage_error_none = 0,
+    charge_target_voltage_error_setting,
+    charge_target_voltage_error_setting_notmach
+} charge_target_voltage_error_t;
+
+typedef enum charge_target_current_error {
+    charge_target_current_error_none = 0,
+    charge_target_current_error_setting,
+    charge_target_current_error_setting_notmach,
+} charge_target_current_error_t;
+
+typedef enum charge_input_voltage_error {
+    charge_input_voltage_error_none = 0,
+    charge_input_voltage_error_setting,
+    charge_input_voltage_error_lower,
+    charge_input_voltage_error_higher
+} charge_input_voltage_error_t;
+
+typedef enum charge_input_current_error {
+    charge_input_current_error_none = 0,
+    charge_input_current_error_setting,
+    charge_input_current_error_lower,
+    charge_input_current_error_higher
+} charge_input_current_error_t;
+
+typedef enum charge_voltage_error {
+    charge_voltage_error_none = 0,
+    charge_voltage_error_setting,
+    charge_voltage_error_lower,
+    charge_voltage_error_higher
+} charge_voltage_error_t;
+
+typedef enum charge_current_error {
+    charge_current_error_none = 0,
+    charge_current_error_setting,
+    charge_current_error_lower,
+    charge_current_error_higher
+} charge_current_error_t;
+
+typedef enum charge_status_error {
+    charge_status_error_none = 0,
+
+} charge_status_error_t;
+
+typedef struct controller_charge_test_result {
+    uint8_t init;
+    uint8_t target_voltage;
+    uint8_t targee_current;
+    uint8_t input_voltage;
+    uint8_t input_current;
+} controller_charge_test_result_t;
+
+static void controller_charger_test(void *argument);
+static int charger_init_test(void);
+static int charger_target_voltage_test(void);
+static int charger_target_current_test(void);
+static int charger_input_voltage_test(void);
+static int charger_input_current_test(void);
+static int charger_battery_info_test(void);
+static int charger_safety_timer_test(void);
+static int charger_fault_test(void);
+static int charger_pin_test(void);
+static int charger_watchdog_test(uint16_t timeout_sec);
+static void charger_test_report(void);
+
+static void controller_charger_test(void *argument)
+{
+    charger_init_test();
+    cntlr_charge_disable();
+    cntlr_charge_enable();
+    osDelay(1000);
+    LOGD(CHG_CTLR, "\r\n----------- charger_target_voltage_test---------\r\n");
+    charger_target_voltage_test();
+
+    LOGD(CHG_CTLR, "\r\n---------- charger_target_current_test--------\r\n");
+    charger_target_current_test();
+
+    LOGD(CHG_CTLR, "\r\n------------charger_input_voltage_test---------\r\n");
+    charger_input_voltage_test();
+
+    LOGD(CHG_CTLR, "\r\n----------- charger_input_current_test----------\r\n");
+    charger_input_current_test();
+    LOGD(CHG_CTLR, "\r\n------------ charger_battery_info_test ----------\r\n");
+    charger_battery_info_test();
+
+
+    //    // disable charge
+    //    charger_pin_test();
+    //
+    //    // charger task
+    //
+    //    // while (1)
+    //    // {
+    //    //     if (1)
+    //    //     { // get pg ok
+    //    //       // start charging
+    //    //       // get fault;
+    //    //     }
+    //    //     else
+    //    //     {
+    //    //         // if charging
+    //    //     }
+    //    // }
+}
+
+static int charger_init_test(void)
+{
+    // cntlr_chrg_irq_cb_t *irq_cb = NULL;
+    // cntlr_charge_register_cb(irq_cb);
+    if (cntlr_charge_init()) {
+        LOGD(CHG_CTLR, "charge controller initial successful\r\n");
+    } else {
+        LOGD(CHG_CTLR, "charge controller initial failed\r\n");
+        return -1;
+    }
+    return 0;
+}
+
+static int charger_target_voltage_test(void)
+{
+    uint16_t targe_voltage = 0;
+    for (int i = (CHARGE_DRV_CHIP_MINVOL - CHARGE_DRV_CHIP_VOLSTEP);
+         i <= (CHARGE_DRV_CHIP_MAXVOL + CHARGE_DRV_CHIP_VOLSTEP * 5);
+         i += CHARGE_DRV_CHIP_VOLSTEP * 5) {
+
+        if (cntlr_charge_set_target_voltage(i)) {
+            LOGD(CHG_CTLR, "charge controller set target voltage: %dmV\r\n", i);
+        } else {
+            LOGD(CHG_CTLR, "charge controller set target voltage failed\r\n");
+            return 0;
+        }
+        vTaskDelay(100);
+        if (cntlr_charge_target_voltage(&targe_voltage)) {
+            LOGD(
+                CHG_CTLR, "charge controller get target voltage %dmV\r\n",
+                targe_voltage);
+        } else {
+            LOGD(CHG_CTLR, "charge controller get target voltage failed\r\n");
+            return -1;
+        }
+        vTaskDelay(100);
+    }
+    return 0;
+}
+
+static int charger_target_current_test(void)
+{
+    uint16_t tartge_current = 0;
+    for (int i = CHARGE_DRV_CHIP_MINCURR - CHARGE_DRV_CHIP_CURRSTEP;
+         i <= (CHARGE_DRV_CHIP_MAXCURR + CHARGE_DRV_CHIP_CURRSTEP * 2);
+         i += CHARGE_DRV_CHIP_CURRSTEP * 2) {
+        if (cntlr_charge_set_target_current(i)) {
+            LOGD(CHG_CTLR, "charge controller set target current: %dmA\r\n", i);
+        } else {
+            LOGD(CHG_CTLR, "charge controller set target current failed\r\n");
+            return -1;
+        }
+        vTaskDelay(100);
+        if (cntlr_charge_target_current(&tartge_current)) {
+            LOGD(
+                CHG_CTLR, "charge controller get target current: %dmA\r\n",
+                tartge_current);
+        } else {
+            LOGD(CHG_CTLR, "charge controller get target current failed\r\n");
+            return -1;
+        }
+    }
+    return 0;
+}
+
+static int charger_input_voltage_test(void)
+{
+    uint16_t input_voltage = 0;
+
+    for (int i = (CHARGE_DRV_CHIP_INPUT_MINVOL - CHARGE_DRV_CHIP_INPUT_VOLSTEP);
+         i
+         <= (CHARGE_DRV_CHIP_INPUT_MAXVOL + CHARGE_DRV_CHIP_INPUT_VOLSTEP * 4);
+         i += CHARGE_DRV_CHIP_INPUT_VOLSTEP * 4) {
+        if (cntlr_charge_set_input_voltage(i)) {
+            LOGD(CHG_CTLR, "charge controller set intput voltage: %dmV\r\n", i);
+        } else {
+            LOGD(CHG_CTLR, "charge controller set intput voltage failed\r\n");
+            return 0;
+        }
+        vTaskDelay(100);
+    }
+    return 0;
+}
+
+static int charger_input_current_test(void)
+{
+    int16_t intput_current = 0;
+
+    if (cntlr_charge_set_input_current(1567)) {
+        LOGD(CHG_CTLR, "charge controller set input current: %dmA\r\n", 1567);
+    } else {
+        LOGD(CHG_CTLR, "charge controller set input current failed\r\n");
+        return -1;
+    }
+    if (cntlr_charge_input_current(&intput_current)) {
+        LOGD(
+            CHG_CTLR, "charge controller get intput current: %dmA\r\n",
+            intput_current);
+    } else {
+        LOGD(CHG_CTLR, "charge controller get intput current failed\r\n");
+        return -1;
+    }
+    if (cntlr_charge_set_input_current(50)) {
+        LOGD(CHG_CTLR, "charge controller set input current: %dmA\r\n", 50);
+    } else {
+        LOGD(CHG_CTLR, "charge controller set input current failed\r\n");
+        return -1;
+    }
+    if (cntlr_charge_input_current(&intput_current)) {
+        LOGD(
+            CHG_CTLR, "charge controller get intput current: %dmA\r\n",
+            intput_current);
+    } else {
+        LOGD(CHG_CTLR, "charge controller get intput current failed\r\n");
+        return -1;
+    }
+    if (cntlr_charge_set_input_current(5000)) {
+        LOGD(CHG_CTLR, "charge controller set input current: %dmA\r\n", 5000);
+    } else {
+        LOGD(CHG_CTLR, "charge controller set input current failed\r\n");
+        return -1;
+    }
+    if (cntlr_charge_input_current(&intput_current)) {
+        LOGD(
+            CHG_CTLR, "charge controller get intput current: %dmA\r\n",
+            intput_current);
+    } else {
+        LOGD(CHG_CTLR, "charge controller get intput current failed\r\n");
+        return -1;
+    }
+
+    return 0;
+}
+
+static int charger_battery_info_test(void)
+{
+    int16_t batt_current;
+    uint16_t batt_volt;
+    if (cntlr_charge_battery_current(&batt_current)) {
+        LOGD(
+            CHG_CTLR, "charge controller get battery current: %dmA\r\n",
+            batt_current);
+    } else {
+        LOGD(CHG_CTLR, "charge controller get battery current failed\r\n");
+        return -1;
+    }
+
+    if (cntlr_charge_battery_voltage(&batt_volt)) {
+        LOGD(
+            CHG_CTLR, "charge controller get battery batt_volt: %dmV\r\n",
+            batt_volt);
+    } else {
+        LOGD(CHG_CTLR, "charge controller get battery batt_volt failed\r\n");
+        return -1;
+    }
+    return 0;
+}
+
+static int charger_safety_timer_test(void)
+{
+    return 0;
+}
+
+static int charger_fault_test(void)
+{
+    return 0;
+}
+
+static int charger_pin_test(void)
+{
+    return 0;
+}
+
+/**
+ * @brief charger watchdog test
+ * - reset all register
+ * - set any target register with non-default value
+ * - wait until watchdog timeout_sec
+ * - check target register if reset
+ *  - if reset, test successful, else failed
+ * @param timeout_sec
+ * @return int SUCCE 0 , ERROR others
+ */
+static int charger_watchdog_test(uint16_t timeout_sec)
+{
+    return 0;
+}
+
+static void charger_test_report(void)
+{
+    // start charging voltage , duration
+    //
+}
+
+// controller_test_result_t controller_test_result_ctx;
+
+void controller_charger_test_init(void)
+{
+    controller_charger_test(NULL);
+}
+
+void controller_charger_test_start(void)
+{
+}
+
+void start_charge_power_detect(void *arg)
+{
+}
+
+void stop_charge_power_detect(void *arg)
+{
+}
+
+void start_charge_type_detect(void)
+{
+}