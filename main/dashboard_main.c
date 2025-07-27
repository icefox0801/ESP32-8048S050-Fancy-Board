#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "lvgl.h"
#include "lvgl/lvgl_setup.h"
#include "serial/serial_data_handler.h"
#include "wifi/wifi_manager.h"
#include "smart/smart_home.h"
#include "smart/ha_status.h"
#include "ui/ui_controls_panel.h"
#include "ui/ui_dashboard.h"
#include "utils/system_debug_utils.h"
#include "esp_log.h"
#include <stdio.h>

// static const char *TAG = "dashboard"; // Removed - unused after debug cleanup

// Display health monitoring - removed redundant watchdog timer
// LVGL task handles all timer processing automatically
static esp_lcd_panel_handle_t global_panel_handle = NULL;

// Removed redundant display watchdog - LVGL task handles timer processing
// No additional display monitoring needed

static void wifi_status_callback(bool is_connected, const char *status_text, wifi_status_t status, const wifi_info_t *info)
{
  status_info_update_wifi_status(status_text, is_connected);
}

static void wifi_connected_callback(void)
{
  smart_home_init();
}

static void serial_connection_status_callback(bool connected)
{
  status_info_update_serial_status(connected);
}

static void serial_data_update_callback(const system_data_t *data)
{
  ui_dashboard_update(data);
}

static void ha_status_change_callback(bool is_ready, bool is_syncing, const char *status_text)
{
  controls_panel_update_ha_status(is_ready, is_syncing, status_text);
}

static void smart_home_states_sync_callback(bool switch_states[3], int state_count)
{
  // Update UI controls based on sync states
  if (state_count >= 3)
  {
    controls_panel_set_switch(0, switch_states[0]); // SWITCH_A = 0
    controls_panel_set_switch(1, switch_states[1]); // SWITCH_B = 1
    controls_panel_set_switch(2, switch_states[2]); // SWITCH_C = 2
  }
}

void app_main(void)
{
  debug_log_startup(DEBUG_TAG_DASHBOARD, "Dashboard");

  // Reduce HTTP client debug noise
  esp_log_level_set("HTTP_CLIENT", ESP_LOG_WARN);
  esp_log_level_set("event", ESP_LOG_WARN);

  // Initialize LVGL
  lvgl_setup_init_backlight();
  lvgl_setup_set_backlight(LCD_BK_LIGHT_OFF_LEVEL);

  esp_lcd_panel_handle_t panel_handle = lvgl_setup_create_lcd_panel();
  global_panel_handle = panel_handle;

  lvgl_setup_set_backlight(LCD_BK_LIGHT_ON_LEVEL);
  lv_display_t *display = lvgl_setup_init(panel_handle);
  lvgl_setup_init_touch();
  lvgl_setup_create_ui_safe(display, ui_dashboard_create);
  lvgl_setup_start_task();

  // Initialize Wi-Fi manager
  ESP_ERROR_CHECK(wifi_manager_init());
  wifi_manager_register_status_callback(wifi_status_callback);
  wifi_manager_register_connected_callback(wifi_connected_callback);

  // Initialize Serial Data
  ESP_ERROR_CHECK(serial_data_init());
  serial_data_register_connection_callback(serial_connection_status_callback);
  serial_data_register_data_callback(serial_data_update_callback);
  serial_data_start_task();

  // Initialize Home Assistant API
  smart_home_register_states_sync_callback(smart_home_states_sync_callback);
  ha_api_init();
  ha_status_init();
  ha_status_register_change_callback(ha_status_change_callback);

  // Register smart home callbacks with UI dashboard for decoupled control
  smart_home_callbacks_t callbacks = {
      .switch_callback = smart_home_control_switch,
      .scene_callback = smart_home_trigger_scene};
  ui_dashboard_register_smart_home_callbacks(&callbacks);

  debug_log_startup(DEBUG_TAG_SYSTEM, "System Monitor - Fully Initialized");

  while (1)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
