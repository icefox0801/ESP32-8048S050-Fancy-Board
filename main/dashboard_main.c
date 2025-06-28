#include "lvgl/lvgl_setup.h"
#include "lvgl/system_monitor_ui.h"
#include "serial/serial_data_handler.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "dashboard";

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

  // Start LVGL task
  lvgl_setup_start_task();

  // Create system monitor UI with thread safety and logging
  lvgl_setup_create_ui_safe(display, system_monitor_ui_create);

  // Initialize serial data handler
  ESP_ERROR_CHECK(serial_data_init());

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
