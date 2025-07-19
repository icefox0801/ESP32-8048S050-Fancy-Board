#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "lvgl.h"
#include "lvgl/lvgl_setup.h"
#include "serial/serial_data_handler.h"
#include "wifi/wifi_manager.h"
#include "smart/ha_task_manager.h"
#include "smart/smart_config.h"
#include "smart/smart_home.h"
#include "ui_dashboard.h"
#include "utils/system_debug_utils.h"
#include <stdio.h>

// static const char *TAG = "dashboard"; // Removed - unused after debug cleanup

// Display health monitoring
static TimerHandle_t display_watchdog_timer = NULL;
static esp_lcd_panel_handle_t global_panel_handle = NULL;

static void display_watchdog_callback(TimerHandle_t xTimer)
{
  if (global_panel_handle != NULL)
  {
    lv_timer_handler();
  }
}

static void init_display_watchdog(void)
{
  display_watchdog_timer = xTimerCreate(
      "DisplayWatchdog",
      pdMS_TO_TICKS(5000),
      pdTRUE,
      NULL,
      display_watchdog_callback);

  if (display_watchdog_timer != NULL)
  {
    xTimerStart(display_watchdog_timer, 0);
  }
  else
  {
    debug_log_error(DEBUG_TAG_DASHBOARD, "Failed to create watchdog timer");
  }
}

static void ui_wifi_status_callback(const char *status_text, bool is_connected)
{
  ui_dashboard_update_wifi_status(status_text, is_connected);
}

static void serial_connection_status_callback(bool connected)
{
  ui_dashboard_set_connection_status(connected);
}

static void serial_data_update_callback(const system_data_t *data)
{
  ui_dashboard_update(data);
}

void app_main(void)
{
  debug_log_startup(DEBUG_TAG_DASHBOARD, "Dashboard");

  lvgl_setup_init_backlight();
  lvgl_setup_set_backlight(LCD_BK_LIGHT_OFF_LEVEL);

  esp_lcd_panel_handle_t panel_handle = lvgl_setup_create_lcd_panel();
  global_panel_handle = panel_handle;

  lvgl_setup_set_backlight(LCD_BK_LIGHT_ON_LEVEL);

  lv_display_t *display = lvgl_setup_init(panel_handle);

  init_display_watchdog();

  lv_indev_t *touch_indev = lvgl_setup_init_touch();
  if (touch_indev == NULL)
  {
    debug_log_error(DEBUG_TAG_GT911_TOUCH, "Failed to initialize touch");
  }

  lvgl_setup_create_ui_safe(display, ui_dashboard_create);

  lvgl_setup_start_task();

  ESP_ERROR_CHECK(serial_data_init());

  debug_log_event(DEBUG_TAG_SYSTEM, "Stabilizing before WiFi init");
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  ESP_ERROR_CHECK(wifi_manager_init());

  ESP_ERROR_CHECK(wifi_manager_register_ui_callback(ui_wifi_status_callback));

  ESP_ERROR_CHECK(ha_task_manager_init());

  ESP_ERROR_CHECK(wifi_manager_register_ha_callback(ha_task_manager_wifi_callback));

  // Connect to WiFi if configured
#ifdef DEFAULT_WIFI_SSID
  if (strlen(DEFAULT_WIFI_SSID) > 0)
  {
#ifdef DEFAULT_WIFI_PASSWORD
    wifi_manager_connect(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD);
#else
    wifi_manager_connect(DEFAULT_WIFI_SSID, NULL);
#endif
  }
#endif

  serial_data_register_connection_callback(serial_connection_status_callback);
  serial_data_register_data_callback(serial_data_update_callback);

  serial_data_start_task();

  esp_err_t ret = smart_home_init();
  if (ret != ESP_OK)
  {
    debug_log_error(DEBUG_TAG_SMART_HOME, "Failed to initialize");
  }

  // Register smart home callbacks with UI dashboard for decoupled control
  smart_home_callbacks_t callbacks = {
      .switch_callback = smart_home_control_switch,
      .scene_callback = smart_home_trigger_scene};
  ui_dashboard_register_smart_home_callbacks(&callbacks);

  debug_log_startup(DEBUG_TAG_SYSTEM, "System Monitor - Fully Initialized");

  while (1)
  {
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}
