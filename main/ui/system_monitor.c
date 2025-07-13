/**
 * @file system_monitor_ui.c
 * @brief System Monitor Dashboard UI for ESP32-S3-8048S050
 *
 * Creates a clean, spacious real-time system monitoring dashboard with:
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

#include "system_monitor.h"

#include "esp_log.h"
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

static const char *TAG = "system_monitor";

// Status and Info Elements
lv_obj_t *timestamp_label = NULL;
lv_obj_t *connection_status_label = NULL;
lv_obj_t *wifi_status_label = NULL;

// CPU Panel Elements
lv_obj_t *cpu_name_label = NULL;
lv_obj_t *cpu_usage_label = NULL;
lv_obj_t *cpu_temp_label = NULL;
lv_obj_t *cpu_fan_label = NULL;

// GPU Panel Elements
lv_obj_t *gpu_name_label = NULL;
lv_obj_t *gpu_usage_label = NULL;
lv_obj_t *gpu_temp_label = NULL;
lv_obj_t *gpu_mem_label = NULL;

// Memory Panel Elements
lv_obj_t *mem_usage_bar = NULL;
lv_obj_t *mem_usage_label = NULL;
lv_obj_t *mem_info_label = NULL;

// Smart Panel Elements
lv_obj_t *switch_a = NULL;
lv_obj_t *switch_b = NULL;
lv_obj_t *switch_c = NULL;
lv_obj_t *scene_button = NULL;
lv_obj_t *ha_status_label = NULL;

/**
 * @brief Create the complete system monitor dashboard UI
 * @param disp LVGL display handle
 */
void system_monitor_ui_create(lv_display_t *disp)
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

  ESP_LOGI(TAG, "System Monitor UI created successfully");
}

/**
 * @brief Update all system monitor display elements with new data
 * @param data Pointer to system monitoring data structure
 * @note This function is thread-safe and handles LVGL mutex locking
 */
void system_monitor_ui_update(const system_data_t *data)
{
  if (!data)
    return;

  lvgl_lock_acquire();

  // ─────────────────────────────────────────────────────────────────
  // Update Timestamp and Status Display
  // ─────────────────────────────────────────────────────────────────

  if (timestamp_label)
  {
    time_t timestamp_sec = data->timestamp / 1000;
    struct tm *timeinfo = localtime(&timestamp_sec);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "Last: %H:%M:%S", timeinfo);
    lv_label_set_text(timestamp_label, time_str);
  }

  // Since we received valid data, update serial status as connected
  status_info_update_serial_status("Connected", true);

  update_cpu_panel(&data->cpu);
  update_gpu_panel(&data->gpu);
  update_memory_panel(&data->mem);
  lvgl_lock_release();
}
