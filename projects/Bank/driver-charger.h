 bool cntlr_charge_pin_level(chrg_pins_index_t pin_idx, uint8_t *level)
diff --git a/Alpha_U3_Base/PMI_Platform/midware/drivers/driver-charger/inc/driver-charger-custom.h b/Alpha_U3_Base/PMI_Platform/midware/drivers/driver-charger/inc/driver-charger-custom.h
index b230382..6208280 100644

+
 /**
  * @brief access structure of the driver charger custom.
  *
  */
 typedef struct driver_charger_custom {
-    uint32_t (*get_faults_wrapper)(uint8_t chrg_faults);
+    uint32_t (*get_faults_wrapper)(uint32_t chrg_faults);
     uint8_t (*get_state_wrapper)(uint8_t state_in_chip);
     uint8_t (*get_timer_wrapper)(driver_charger_timer_t timer);
+    uint8_t (*get_source_type_wrapper)(uint8_t source_type);
 } driver_charger_custom_t;
 
 void driver_charger_custom_init();
@@ -94,4 +116,4 @@ void driver_charger_custom_init();
 }
 #endif
 
 +#ifdef SUPPORT_CHARGER_BQ2562X
+#include "driver-bq2562x.h"
+
+#define DRIVER_BQ2562X_SELECT
+#endif /* SUPPORT_CHARGER_BQ2562X */
+
 /** List of all charger driver ic. */
 typedef enum charger_driver_chipset {
     charger_driver_chipset_first   = 0,
@@ -46,6 +52,7 @@ typedef enum charger_driver_chipset {
     charger_driver_chipset_bq2589x,
     charger_driver_chipset_sgm41578,
     charger_driver_chipset_bq2563x,
+    charger_driver_chipset_bq2562x,
     charger_driver_chipset_unknown,
     charger_driver_chipset_count = charger_driver_chipset_unknown
 } charger_driver_chipset_t;
@@ -261,6 +268,60 @@ typedef struct charger_driver_s {
     int32_t (*get_faults)(
         drv_chargerabs_instance_t *drv_charger_instance, uint32_t *faults);
 
+    /**
+     * @brief Pointer to \ref driver_charger_get_battery_voltage: Get battery
+     * voltage.
+     *
+     * @param drv_charger_instance The charger instance.
+     * @param voltage The battery voltage data which get from the charger.
+     * @return int charger status.
+     */
+    int32_t (*get_battery_voltage)(
+        drv_chargerabs_instance_t *drv_charger_instance, uint16_t *voltage);
+
+    /**
+     * @brief Pointer to \ref driver_charger_get_battery_current: Get battery
+     * current.
+     *
+     * @param drv_charger_instance The charger instance.
+     * @param current The battery current data which get from the charger.
+     * @return int charger status.
+     */
+    int32_t (*get_battery_current)(
+        drv_chargerabs_instance_t *drv_charger_instance, int16_t *current);
+
+    /**
+     * @brief Pointer to \ref driver_charger_source_type_detection_enable:
+     * Enable source type detection.
+     *
+     * @param drv_charger_instance The charger instance.
+     * @param enable Enable or Disable source type detection.
+     * @return int charger status.
+     */
+    int32_t (*source_type_detection_enable)(
+        drv_chargerabs_instance_t *drv_charger_instance, uint8_t enable);
+
+    /**
+     * @brief Pointer to \ref driver_charger_get_source_detection_status: Get
+     * source type detection state
+     *
+     * @param drv_charger_instance The charger instance.
+     * @param state The source detection state data which get from the charger.
+     * @return int charger status.
+     */
+    int32_t (*get_source_type_detection_status)(
+        drv_chargerabs_instance_t *drv_charger_instance, uint8_t *state);
+
+    /**
+     * @brief Pointer to \ref driver_charger_get_source_type: Get source type
+     * @param drv_charger_instance The charger instance.
+     * @param source_type The  source type data which get from the charger
+     * and wrapped to driver defined types
+     * @return int charger status.
+     */
+    int32_t (*get_source_type)(
+        drv_chargerabs_instance_t *drv_charger_instance, uint8_t *source_type);
+
 } drv_chargerabs_driver_t;