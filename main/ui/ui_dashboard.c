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
  {
    debug_log_warning(DEBUG_TAG_UI_DASHBOARD, "⚠️ Dashboard update called with NULL data");
    return;
  }

  debug_log_debug_f(DEBUG_TAG_UI_DASHBOARD, "🔄 Dashboard update - CPU: %s (%d%%), GPU: %s (%d%%), Memory: %.1f/%.1f GB", 
                    data->cpu.name, data->cpu.usage, data->gpu.name, data->gpu.usage, data->mem.used, data->mem.total);

  if (!lvgl_port_lock(100)) // 100ms timeout
  {
    debug_log_warning(DEBUG_TAG_UI_DASHBOARD, "⚠️ Could not acquire LVGL lock for dashboard update (timeout)");
    return;
  }

  update_cpu_panel(&data->cpu);
  update_gpu_panel(&data->gpu);
  update_memory_panel(&data->mem);
  lvgl_port_unlock();
  
  debug_log_debug(DEBUG_TAG_UI_DASHBOARD, "✅ Dashboard panels updated successfully");
}

/**
 * @brief Reset dashboard display to default values when serial connection is lost
 * @note This function is thread-safe and handles LVGL mutex locking
 */
void ui_dashboard_reset_to_defaults(void)
{
  if (!lvgl_port_lock(100)) // 100ms timeout
  {
    debug_log_warning(DEBUG_TAG_UI_DASHBOARD, "⚠️ Could not acquire LVGL lock for dashboard reset (timeout)");
    return;
  }

  // Use individual panel reset functions for cleaner implementation
  reset_cpu_panel();
  reset_gpu_panel();
  reset_memory_panel();
  lvgl_port_unlock();

  debug_log_info(DEBUG_TAG_UI_DASHBOARD, "🔄 Dashboard reset to default values");
}

/**
 * @brief Register smart home control callbacks for UI decoupling
 * @param callbacks Pointer to smart home callback structure
 */
void ui_dashboard_register_smart_home_callbacks(const smart_home_callbacks_t *callbacks)
{
  controls_panel_register_event_callbacks(callbacks);
  debug_log_info(DEBUG_TAG_UI_DASHBOARD, "Smart home callbacks registered with UI dashboard");
}
