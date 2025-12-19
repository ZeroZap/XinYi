diff --git a/Alpha_U3_Base/PMI_Platform/midware/controllers/controller-charger/src/controller-charger.c b/Alpha_U3_Base/PMI_Platform/midware/controllers/controller-charger/src/controller-charger.c
index 4de039c..19a16a9 100644
--- a/Alpha_U3_Base/PMI_Platform/midware/controllers/controller-charger/src/controller-charger.c
+++ b/Alpha_U3_Base/PMI_Platform/midware/controllers/controller-charger/src/controller-charger.c
@@ -120,7 +120,9 @@ static gpio_instance_t gpio_charger_ext_pwr = {
  *
  */
 static const gpio_instance_t *p_gpio_charger[chrg_pins_idx_count] = {
-    &gpio_charger_state, &gpio_charger_detect, &gpio_charger_ext_pwr,
+    &gpio_charger_state,
+    &gpio_charger_detect,
+    &gpio_charger_ext_pwr,
 };
 
 /**
@@ -288,6 +290,12 @@ bool cntlr_charge_set_input_current(uint16_t current)
         == driver_charger.set_input_current(&charger_abs_driver, current);
 }
 
+bool cntlr_charge_input_current(uint16_t *current)
+{
+    return hal_err_none
+        == driver_charger.get_input_current(&charger_abs_driver, current);
+}
+
 bool cntlr_charge_set_input_voltage(uint16_t voltage)
 {
     return hal_err_none
@@ -312,6 +320,18 @@ bool cntlr_charge_target_current(uint16_t *current)
         == driver_charger.get_current(&charger_abs_driver, current);
 }
 
+bool cntlr_charge_battery_voltage(uint16_t *voltage)
+{
+    return hal_err_none
+        == driver_charger.get_battery_voltage(&charger_abs_driver, voltage);
+}
+
+bool cntlr_charge_battery_current(int16_t *current)
+{
+    return hal_err_none
+        == driver_charger.get_battery_current(&charger_abs_driver, current);
+}
+
 void cntlr_charge_register_cb(const cntlr_chrg_irq_cb_t *cb)
 {
     irq_chg_state_cb = cb->irq_chg_state_cb;
@@ -323,21 +343,21 @@ bool cntlr_charge_enable(void)
 {
     return hal_err_none
         == driver_charger.set_control_charge(
-               &charger_abs_driver, cntlr_chrg_req_enable);
+            &charger_abs_driver, cntlr_chrg_req_enable);
 }
 
 bool cntlr_charge_disable(void)
 {
     return hal_err_none
         == driver_charger.set_control_charge(
-               &charger_abs_driver, cntlr_chrg_req_disable);
+            &charger_abs_driver, cntlr_chrg_req_disable);
 }
 
 bool cntlr_charge_disconnect(void)
 {
     return hal_err_none
         == driver_charger.set_sysoff(
-               &charger_abs_driver, cntlr_chrg_req_disconnect);
+            &charger_abs_driver, cntlr_chrg_req_disconnect);
 }
 
 bool cntlr_charge_disable_ts(void)
@@ -380,7 +400,28 @@ cntlr_chrg_state_t cntlr_charge_state(void)
 
 bool cntlr_charge_faults(uint32_t *faults)
 {
-    return driver_charger.get_faults(&charger_abs_driver, faults);
+    return hal_err_none
+        == driver_charger.get_faults(&charger_abs_driver, faults);
+}
+
+bool cntlr_charge_source_type_detection_enable(void)
+{
+    return hal_err_none
+        == driver_charger.source_type_detection_enable(
+            &charger_abs_driver, true);
+}
+
+bool cntlr_charge_get_source_type_detection_status(uint8_t *status)
+{
+    return hal_err_none
+        == driver_charger.get_source_type_detection_status(
+            &charger_abs_driver, status);
+}
+
+bool cntlr_charge_get_source_type(uint8_t *type)
+{
+    return hal_err_none
+        == driver_charger.get_source_type(&charger_abs_driver, type);
 }