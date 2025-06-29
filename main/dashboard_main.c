#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "lvgl/lvgl_setup.h"
#include "lvgl/system_monitor_ui.h"
#include "serial/serial_data_handler.h"
#include "wifi/wifi_manager.h"
#include <stdio.h>

static const char *TAG = "dashboard";

/**
 * @brief WiFi status callback to update connection status in UI
 */
static void wifi_status_callback(wifi_status_t status, const wifi_info_t *info)
{
  switch (status)
  {
  case WIFI_STATUS_DISCONNECTED:
    ESP_LOGI(TAG, "WiFi Status: Disconnected");
    system_monitor_ui_update_wifi_status("Disconnected", false);
    break;

  case WIFI_STATUS_CONNECTING:
    ESP_LOGI(TAG, "WiFi Status: Connecting...");
    system_monitor_ui_update_wifi_status("Connecting...", false);
    break;

  case WIFI_STATUS_CONNECTED:
    if (info)
    {
      ESP_LOGI(TAG, "WiFi Status: Connected to %s (IP: %s, RSSI: %ddBm)",
               info->ssid, info->ip_address, info->rssi);
      char status_msg[128];
      snprintf(status_msg, sizeof(status_msg), "Connected: %s (%s)",
               info->ssid, info->ip_address);
      system_monitor_ui_update_wifi_status(status_msg, true);
    }
    else
    {
      system_monitor_ui_update_wifi_status("Connected", true);
    }
    break;

  case WIFI_STATUS_FAILED:
    ESP_LOGE(TAG, "WiFi Status: Connection Failed");
    system_monitor_ui_update_wifi_status("Connection Failed", false);
    break;

  case WIFI_STATUS_RECONNECTING:
    ESP_LOGI(TAG, "WiFi Status: Reconnecting...");
    system_monitor_ui_update_wifi_status("Reconnecting...", false);
    break;

  default:
    ESP_LOGW(TAG, "WiFi Status: Unknown (%d)", status);
    system_monitor_ui_update_wifi_status("Unknown", false);
    break;
  }
}

void app_main(void)
{
  ESP_LOGI(TAG, "System Monitor Dashboard started!");

  // Initialize and turn off LCD backlight
  lvgl_setup_init_backlight();
  lvgl_setup_set_backlight(LCD_BK_LIGHT_OFF_LEVEL);

  // Create and initialize LCD panel
  esp_lcd_panel_handle_t panel_handle = lvgl_setup_create_lcd_panel();

  // Turn on LCD backlight
  lvgl_setup_set_backlight(LCD_BK_LIGHT_ON_LEVEL);

  // Initialize LVGL
  lv_display_t *display = lvgl_setup_init(panel_handle);

  // Initialize GT911 touch controller
  lv_indev_t *touch_indev = lvgl_setup_init_touch();
  if (touch_indev == NULL)
  {
    ESP_LOGE(TAG, "Failed to initialize touch controller");
  }
  else
  {
    ESP_LOGI(TAG, "Touch controller initialized successfully");
  }

  // Start LVGL task
  lvgl_setup_start_task();

  // Create system monitor UI with thread safety and logging
  lvgl_setup_create_ui_safe(display, system_monitor_ui_create);

  // Initialize serial data handler
  ESP_ERROR_CHECK(serial_data_init());

  // Initialize WiFi manager
  // ESP_LOGI(TAG, "Initializing WiFi...");
  // ESP_ERROR_CHECK(wifi_manager_init());

  // Register WiFi status callback for UI updates
  // ESP_ERROR_CHECK(wifi_manager_register_callback(wifi_status_callback));

  // Start receiving serial data
  serial_data_start_task();

  ESP_LOGI(TAG, "System monitor initialized, waiting for JSON data...");

  // Main loop - dashboard can do other tasks here
  while (1)
  {
    ESP_LOGI(TAG, "System monitor running... (waiting for serial JSON data)");
    vTaskDelay(10000 / portTICK_PERIOD_MS); // Check every 10 seconds
  }
}
