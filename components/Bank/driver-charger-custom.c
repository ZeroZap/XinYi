#define DRIVER_BQ2563X_CUSTOM
 #endif
 
+#ifdef SUPPORT_CHARGER_BQ2562X
+#include "driver-bq2562x.h"
+
+#define DRIVER_BQ2562X_CUSTOM
+#endif /* SUPPORT_CHARGER_BQ2562X */
+
 #ifdef DRIVER_BQ2425X_CUSTOM
 
 /**
@@ -47,7 +53,7 @@
  *
  * @return uint32_t Driver charger faults value.
  */
-static uint32_t bq2425x_get_faults_wrapper(uint8_t chrg_faults)
+static uint32_t bq2425x_get_faults_wrapper(uint32_t chrg_faults)
 {
     uint32_t wrapped_faults = driver_charger_fault_none;
 
@@ -146,7 +152,7 @@ static uint8_t bq2425x_get_timer_wrapper(driver_charger_timer_t timer)
 
 static driver_charger_custom_t driver_bq2425x_custom = {
     bq2425x_get_faults_wrapper, bq2425x_get_state_wrapper,
-    bq2425x_get_timer_wrapper,
+    bq2425x_get_timer_wrapper, NULL,
 };
 
 
@@ -161,7 +167,7 @@ static driver_charger_custom_t driver_bq2425x_custom = {
  *
  * @return uint32_t Driver charger faults value.
  */
-static uint32_t bq2589x_get_faults_wrapper(uint8_t chrg_faults)
+static uint32_t bq2589x_get_faults_wrapper(uint32_t chrg_faults)
 {
     uint32_t wrapped_faults = driver_charger_fault_none;
 
@@ -246,7 +252,7 @@ static uint8_t bq2589x_get_timer_wrapper(driver_charger_timer_t timer)
 
 static driver_charger_custom_t driver_bq2589x_custom = {
     bq2589x_get_faults_wrapper, bq2589x_get_state_wrapper,
-    bq2589x_get_timer_wrapper,
+    bq2589x_get_timer_wrapper, NULL,
 };
 
 
@@ -261,7 +267,7 @@ static driver_charger_custom_t driver_bq2589x_custom = {
  *
  * @return uint32_t Driver charger faults value.
  */
-static uint32_t sgm41578_get_faults_wrapper(uint8_t chrg_faults)
+static uint32_t sgm41578_get_faults_wrapper(uint32_t chrg_faults)
 {
     uint32_t wrapped_faults = driver_charger_fault_none;
 
@@ -346,7 +352,7 @@ static uint8_t sgm41578_get_timer_wrapper(driver_charger_timer_t timer)
 
 static driver_charger_custom_t driver_sgm41578_custom = {
     sgm41578_get_faults_wrapper, sgm41578_get_state_wrapper,
-    sgm41578_get_timer_wrapper,
+    sgm41578_get_timer_wrapper, NULL,
 };
 
 #endif
@@ -360,7 +366,7 @@ static driver_charger_custom_t driver_sgm41578_custom = {
  *
  * @return uint32_t Driver charger faults value.
  */
-static uint32_t bq2563x_get_faults_wrapper(uint8_t chrg_faults)
+static uint32_t bq2563x_get_faults_wrapper(uint32_t chrg_faults)
 {
     uint32_t wrapped_faults = driver_charger_fault_none;
 
@@ -431,9 +437,129 @@ static uint8_t bq2563x_get_timer_wrapper(driver_charger_timer_t timer)
 
 static driver_charger_custom_t driver_bq2563x_custom = {
     bq2563x_get_faults_wrapper, bq2563x_get_state_wrapper,
-    bq2563x_get_timer_wrapper,
+    bq2563x_get_timer_wrapper, NULL,
 };
 
+#endif
+
+#ifdef DRIVER_BQ2562X_CUSTOM
+
+/**
+ * @brief Wrapper charger faults to driver charger layer faults.
+ *
+ * @param chrg_faults Faults from charger chip.
+ *
+ * @return uint32_t Driver charger faults value.
+ */
+static uint32_t bq2562X_get_faults_wrapper(uint32_t chrg_faults)
+{
+    uint32_t wrapped_faults = driver_charger_fault_none;
+    if (chrg_faults & BQ2562X_FAULT_BAT_TEMP) {
+        wrapped_faults = driver_charger_fault_batt_temp;
+    }
+    if (chrg_faults & BQ2562X_FAULT_BAT) {
+        wrapped_faults |= driver_charger_fault_batt_ovp;
+    }
+    if (chrg_faults & BQ2562X_FAULT_VBUS) {
+        wrapped_faults |=
+            (driver_charger_fault_input_ovp | driver_charger_fault_sleep);
+    }
+    if (chrg_faults & BQ2562X_FAULT_THERMAL_SHUTDOWN) {
+        wrapped_faults |= driver_charger_fault_thermal_shutdown;
+    }
+    if (chrg_faults & BQ2562X_FAULT_CHRG_TIMER_EXPIRATION) {
+        wrapped_faults |= driver_charger_fault_timer;
+    }
+    if (chrg_faults & BQ2562X_FAULT_BOOST) {
+        wrapped_faults |= driver_charger_fault_boost;
+    }
+    if (chrg_faults & BQ2562X_FAULT_WTD_TIMER_EXPIRATION) {
+        wrapped_faults |= driver_charger_fault_watchdog;
+    }
+
+    return wrapped_faults;
+}
+
+/**
+ * @brief Wrapper charger chip state to driver charger layer state.
+ *
+ * @param state_in_chip  state from charger chip.
+ *
+ * @return uint8_t Driver charger state.
+ */
+static uint8_t bq2562X_get_state_wrapper(uint8_t state_in_chip)
+{
+    switch (state_in_chip) {
+    case BQ2562X_CHG_STAT_DONE_OR_IDLE:
+        return driver_charger_state_idle;
+    case BQ2562X_CHG_STAT_CC:
+    case BQ2562X_CHG_STAT_CV:
+        return driver_charger_state_charging;
+    case BQ2562X_CHG_STAT_TOP_OFF:
+        return driver_charger_state_done;
+    default:
+        return driver_charger_state_fault;
+    }
+}
+
+/**
+ * @brief Wrapper charger timer to driver charger layer timer.
+ *
+ * @param timer Driver charger timer.
+ *
+ * @return Timer from charger chip.
+ */
+static uint8_t bq2562X_get_timer_wrapper(driver_charger_timer_t timer)
+{
+    switch (timer) {
+    case BQ2562X_VBUS_TYPE_NO_ADAPTER:
+        return BQ2562X_CHG_TMR_28H;
+    case BQ2562X_VBUS_TYPE_SDP:
+    default:
+        return BQ2562X_CHG_TMR_14P5H;
+    }
+}
+
+/**
+ * @brief Wrapper charger source type to driver charger layer type.
+ *
+ * @param timer Driver charger timer.
+ *
+ * @return Timer from charger chip.
+ */
+static uint8_t bq2562X_get_source_type_wrapper(uint8_t source_type)
+{
+    switch (source_type) {
+
+    case BQ2562X_VBUS_TYPE_SDP:
+        return driver_charger_source_type_sdp_500ma;
+    case BQ2562X_VBUS_TYPE_CDP:
+        return driver_charger_source_type_cdp_high_2050ma;
+    case BQ2562X_VBUS_TYPE_DCP:
+        return driver_charger_source_type_dcp_2050ma;
+    case BQ2562X_VBUS_TYPE_UNKNOWN:
+        return driver_charger_source_type_unknown_500ma;
+    case BQ2562X_VBUS_TYPE_NSA_1A:
+        return driver_charger_source_type_non_standard_1000ma;
+    case BQ2562X_VBUS_TYPE_NSA_2P1A:
+        return driver_charger_source_type_non_standard_2100ma;
+    case BQ2562X_VBUS_TYPE_NSA_2P4A:
+        return driver_charger_source_type_non_standard_2400ma;
+    case BQ2562X_VBUS_TYPE_HVDCP:
+        return driver_charger_source_type_cdp_high_2050ma;
+    case BQ2562X_VBUS_TYPE_OTG:
+    case BQ2562X_VBUS_TYPE_NO_ADAPTER:
+    default:
+        return driver_charger_source_type_none;
+    }
+}
+
+static driver_charger_custom_t driver_bq2562X_custom = {
+    bq2562X_get_faults_wrapper, bq2562X_get_state_wrapper,
+    bq2562X_get_timer_wrapper, bq2562X_get_source_type_wrapper,
+};
+
+
 #endif
 
 void driver_charger_custom_init()
@@ -450,4 +576,7 @@ void driver_charger_custom_init()
 #ifdef DRIVER_BQ2563X_CUSTOM
     driver_bq2563x.custom = &driver_bq2563x_custom;
 #endif
+#ifdef DRIVER_BQ2562X_CUSTOM
+    driver_bq2562x.custom = &driver_bq2562X_custom;
+#endif
 }
diff --git a/Alpha_U3_Base/PMI_Platform/midware/drivers/driver-charger/src/driver-charger.c b/Alpha_U3_Base/PMI_Platform/midware/drivers/driver-charger/src/driver-charger.c
index 84dab94..a8b8322 100644
--- a/Alpha_U3_Base/PMI_Platform/midware/drivers/driver-charger/src/driver-charger.c
+++ b/Alpha_U3_Base/PMI_Platform/midware/drivers/driver-charger/src/driver-charger.c
@@ -38,6 +38,12 @@ static driver_charger_type0_t
         NULL,
 #endif
 
+#ifdef DRIVER_BQ2562X_SELECT
+        &driver_bq2562x,
+#else
+        NULL,
+#endif /* DRIVER_BQ2562X_SELECT */
+
     };
 
 /**
@@ -403,8 +409,8 @@ int32_t driver_charger_set_sysoff(
 int32_t driver_charger_get_faults(
     drv_chargerabs_instance_t *drv_charger_instance, uint32_t *faults)
 {
-    uint8_t chrg_faults = driver_charger_fault_none;
-    int ret             = hal_err_none;
+    uint32_t chrg_faults = driver_charger_fault_none;
+    int ret              = hal_err_none;
     driver_charger_custom_t *ptr_custom;
 
     if (drv_charger_instance
@@ -423,6 +429,89 @@ int32_t driver_charger_get_faults(
     return ret;
 }
 
+/**
+ * @brief Get battery voltage.
+ *
+ * @param drv_charger_instance The charger instance.
+ * @param voltage The battery voltage data which get from the charger.
+ * @return int charger status.
+ */
+int32_t driver_charger_get_battery_voltage(
+    drv_chargerabs_instance_t *drv_charger_instance, uint16_t *voltage)
+{
+    return drv_charger_instance->ptr_driver_charger_type0->get_battery_voltage(
+        drv_charger_instance->charger_instance, voltage);
+}
+
+/**
+ * @brief Get battery current.
+ *
+ * @param drv_charger_instance The charger instance.
+ * @param current The battery current data which get from the charger.
+ * @return int charger status.
+ */
+int32_t driver_charger_get_battery_current(
+    drv_chargerabs_instance_t *drv_charger_instance, int16_t *current)
+{
+    return drv_charger_instance->ptr_driver_charger_type0->get_battery_current(
+        drv_charger_instance->charger_instance, current);
+}
+
+/**
+ * @brief Enable source type detection.
+ *
+ * @param drv_charger_instance The charger instance.
+ * @param enable Enable or Disable source type detection.
+ * @return int charger status.
+ */
+int32_t set_control_source_detection(
+    drv_chargerabs_instance_t *drv_charger_instance, uint8_t enable)
+{
+    return drv_charger_instance->ptr_driver_charger_type0
+        ->set_control_source_detection(
+            drv_charger_instance->charger_instance, enable);
+}
+
+/**
+ * @brief Get source type detection state
+ *
+ * @param drv_charger_instance The charger instance.
+ * @param state The source detection state data which get from the charger.
+ * @return int charger status.
+ */
+int32_t driver_charger_get_source_detection_status(
+    drv_chargerabs_instance_t *drv_charger_instance, uint8_t *state)
+{
+    return drv_charger_instance->ptr_driver_charger_type0
+        ->get_source_detection_status(
+            drv_charger_instance->charger_instance, state);
+}
+
+/**
+ * @brief Get source type
+ * @param drv_charger_instance The charger instance.
+ * @param source_type The  source source type data which get from the
+ * charger and wrapped to driver defined types
+ * @return int charger status.
+ */
+int32_t driver_charger_get_source_type(
+    drv_chargerabs_instance_t *drv_charger_instance, uint8_t *source_type)
+{
+    uint8_t type = driver_charger_source_type_none;
+    int ret      = hal_err_none;
+    driver_charger_custom_t *ptr_custom =
+        (driver_charger_custom_t *)
+            drv_charger_instance->ptr_driver_charger_type0->custom;
+
+    ret = drv_charger_instance->ptr_driver_charger_type0->get_source_type(
+        drv_charger_instance->charger_instance, &type);
+    if (hal_err_none == ret) {
+        *source_type = ptr_custom->get_source_type_wrapper(type);
+    }
+
+    return ret;
+}
+
 drv_chargerabs_driver_t driver_charger = {
     driver_charger_init,
     driver_charger_get_device_id,
@@ -440,5 +529,10 @@ drv_chargerabs_driver_t driver_charger = {
     driver_charger_set_input_voltage,
     driver_charger_set_termination_current,
     driver_charger_set_sysoff,
-    driver_charger_get_faults
+    driver_charger_get_faults,
+    driver_charger_get_battery_voltage,
+    driver_charger_get_battery_current,
+    set_control_source_detection,
+    driver_charger_get_source_detection_status,
+    driver_charger_get_source_type
 };