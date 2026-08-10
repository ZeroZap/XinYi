#ifndef XY_GUI_SSD1306_ADAPTER_H
#define XY_GUI_SSD1306_ADAPTER_H

#include "xy_gui.h"
#include "xy_oled_ssd1306.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bind an SSD1306 OLED driver instance to the legacy GUI display-driver callback seam.
 *
 * The GUI core callback table has no user-data field, so this adapter uses a small internal slot
 * table to keep multiple OLED instances isolated. RGB565 black maps to a cleared mono pixel;
 * every other color maps to a set mono pixel.
 *
 * @param out_drv GUI display callback table to populate.
 * @param oled Initialized SSD1306 driver instance with a non-NULL framebuffer and page-aligned
 *        dimensions (height must be a non-zero multiple of 8 pixels).
 * @return XY_GUI_OK on success, XY_GUI_INVALID_PARAM for NULL inputs, or XY_GUI_NO_MEM if no
 *         adapter slot is available.
 */
int xy_gui_ssd1306_bind(xy_gui_disp_drv_t *out_drv, xy_oled_ssd1306_t *oled);

/**
 * @brief Clear all SSD1306 adapter slots.
 *
 * Use this before rebuilding a GUI scene or during host tests when previously-bound OLED driver
 * instances are no longer valid. Existing callback tables obtained from xy_gui_ssd1306_bind()
 * become invalid after reset and must be rebound before use.
 */
void xy_gui_ssd1306_adapter_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_GUI_SSD1306_ADAPTER_H */
