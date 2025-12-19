--- a/Alpha_U3_Base/PMI_Platform/midware/drivers/driver-charger/inc/driver-charger-custom.h
+++ b/Alpha_U3_Base/PMI_Platform/midware/drivers/driver-charger/inc/driver-charger-custom.h
@@ -60,8 +60,8 @@ typedef enum {
 } driver_charger_state_t;
 
 /**
-* @brief List of charge timer setting by driver charger.
-*/
+ * @brief List of charge timer setting by driver charger.
+ */
 typedef enum {
     driver_charger_timer_disable,
     driver_charger_timer_45mins,
@@ -74,17 +74,39 @@ typedef enum {
     driver_charger_timer_12hrs,
     driver_charger_timer_14hrs,
     driver_charger_timer_20hrs,
-    driver_charger_timer_27hrs
+    driver_charger_timer_27hrs,
+    driver_charger_timer_28hrs
 } driver_charger_timer_t;
 
+/**
+ * @brief List of charge source supported by driver charger.
+ */
+typedef enum {
+    driver_charger_source_type_dcp_2050ma = 0x00,
+    driver_charger_source_type_sdp_500ma,
+    driver_charger_source_type_sdp_100ma,
+    driver_charger_source_type_cdp_default_2050ma,
+    driver_charger_source_type_cdp_audio_500ma,
+    driver_charger_source_type_cdp_medium_1500ma,
+    driver_charger_source_type_cdp_high_2050ma,
+    driver_charger_source_type_sdp_scp_dock_2050ma,
+    driver_charger_source_type_non_standard_1000ma,
+    driver_charger_source_type_non_standard_2000ma,
+    driver_charger_source_type_non_standard_2100ma,
+    driver_charger_source_type_non_standard_2400ma,
+    driver_charger_source_type_unknown_500ma,
+    driver_charger_source_type_none
+} driver_charger_source_type_t;