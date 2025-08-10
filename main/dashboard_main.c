#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "lvgl.h"
#include "lvgl/lvgl_setup.h"
#include "serial/serial_data_handler.h"
#include "smart/ha_status.h"
#include "smart/smart_home.h"
#include "ui/ui_controls_panel.h"
#include "ui/ui_dashboard.h"
#include "ui/ui_status_info.h"
#include "utils/system_debug_utils.h"
#include "wifi/wifi_manager.h"

// LVGL task handles all timer processing automatically
static esp_lcd_panel_handle_t global_panel_handle = NULL;
static TimerHandle_t runtime_timer = NULL;
static uint32_t runtime_seconds = 0;

// No additional display monitoring needed

static void runtime_timer_callback(TimerHandle_t xTimer)
{
  runtime_seconds++;
  status_info_update_runtime(runtime_seconds);
}

static void init_runtime_timer(void)
{
  // Create and start runtime timer (1 second interval)
  runtime_timer = xTimerCreate(
      "RuntimeTimer",
      pdMS_TO_TICKS(1000), // 1 second interval
      pdTRUE,              // Auto-reload
      NULL,                // Timer ID (not used)
      runtime_timer_callback);

  if (runtime_timer != NULL)
  {
    xTimerStart(runtime_timer, 0);
    debug_log_info(DEBUG_TAG_SYSTEM, "Runtime timer started");
  }
  else
  {
    debug_log_error(DEBUG_TAG_SYSTEM, "Failed to create runtime timer");
  }
}

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

  // Reset dashboard to default values when serial connection is lost
  if (!connected)
  {
    ui_dashboard_reset_to_defaults();
  }
}

static void serial_data_update_callback(const system_data_t *data)
{
  ui_dashboard_update(data);
}

void ha_status_change_callback(bool is_ready, bool is_syncing, const char *status_text)
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

  // Register smart home callbacks BEFORE creating UI
  // This ensures callbacks are available when controls are created
  smart_home_callbacks_t callbacks = {
      .switch_callback = smart_home_control_switch,
      .scene_callback = smart_home_trigger_scene};
  ui_dashboard_register_smart_home_callbacks(&callbacks);

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

  // Register Smart Home callbacks
  smart_home_register_states_sync_callback(smart_home_states_sync_callback);

  // Initialize runtime timer
  init_runtime_timer();

  // Note: smart_home_init() will be called from wifi_connected_callback()
  // and will initialize ha_status_init(). We'll register the callback
  // from within the smart home module to ensure proper timing.

  debug_log_startup(DEBUG_TAG_SYSTEM, "System Monitor - Fully Initialized");

  while (1)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
