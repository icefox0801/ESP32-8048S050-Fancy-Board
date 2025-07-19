/**
 * @file ui_dashboard.c
 * @brief Dashboard UI for ESP32-S3-8048S050
 *
 * Creates a clean, spacious real-time dashboard with:
 * - CPU Panel (Left): Usage, Temperature, Fan Speed
 * - GPU Panel (Center): Usage, Memory, Temperature
 * - Memory Panel (Right): System Memory Usage
 *
 * Features improved layout with:
 * - Larger fonts for better readability
 * - Proper vertical alignment between labels and values
 * - Equal padding for all panels
 * - Increased panel heights for better spacing
 * - Clean design without emoji icons
 */

#include "ui_dashboard.h"

#include "system_debug_utils.h"
#include "lvgl_setup.h"
#include "ui_config.h"
#include "ui_helpers.h"
#include "ui_cpu_panel.h"
#include "ui_gpu_panel.h"
#include "ui_memory_panel.h"
#include "ui_status_info.h"
#include "smart/ha_api.h"
#include "smart/smart_home.h"
#include "smart/smart_config.h"
#include <stdio.h>
#include <time.h>

/**
 * @brief Create the complete dashboard UI
 * @param disp LVGL display handle
 */
void ui_dashboard_create(lv_display_t *disp)
{
  // Initialize LVGL theme with dark mode and blue/red accents
  lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                        LV_THEME_DEFAULT_DARK, font_normal);

  lv_obj_t *screen = lv_display_get_screen_active(disp);

  // Create all UI panels (smart panel at top, status panel at bottom)
  create_controls_panel(screen);
  create_cpu_panel(screen);
  create_gpu_panel(screen);
  create_memory_panel(screen);
  create_status_info_panel(screen);

  debug_log_info(DEBUG_TAG_UI_DASHBOARD, "Dashboard UI created successfully");
}

/**
 * @brief Update all dashboard display elements with new data
 * @param data Pointer to system monitoring data structure
 * @note This function is thread-safe and handles LVGL mutex locking
 */
void ui_dashboard_update(const system_data_t *data)
{
  if (!data)
    return;

  lvgl_lock_acquire();

  status_info_update_timestamp(data->timestamp);
  status_info_update_serial_status("Connected", true);
  update_cpu_panel(&data->cpu);
  update_gpu_panel(&data->gpu);
  update_memory_panel(&data->mem);
  lvgl_lock_release();
}
