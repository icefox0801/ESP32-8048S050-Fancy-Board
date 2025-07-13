#include "esp_log.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "lvgl.h"
#include "lvgl/lvgl_setup.h"
#include "lvgl/system_monitor_ui.h"
#include "serial/serial_data_handler.h"
#include "wifi/wifi_manager.h"
#include "smart/ha_task_manager.h"
#include "smart/smart_config.h"
#include "smart/smart_home.h"
#include <stdio.h>

static const char *TAG = "dashboard";

// Display health monitoring
static TimerHandle_t display_watchdog_timer = NULL;
static esp_lcd_panel_handle_t global_panel_handle = NULL;

/**
 * @brief Display watchdog timer callback to prevent black screen
 * This function periodically refreshes the display to maintain stability
 */
static void display_watchdog_callback(TimerHandle_t xTimer)
{
  if (global_panel_handle != NULL)
  {
    // Force display refresh to prevent drift and black screen
    ESP_LOGD(TAG, "Display watchdog: Refreshing display");
    // The panel refresh happens automatically via LVGL, this just ensures timing stability
    lv_timer_handler(); // Force LVGL to process any pending updates
  }
}

/**
 * @brief Initialize display health monitoring
 */
static void init_display_watchdog(void)
{
  // Create a timer that triggers every 5 seconds to maintain display health
  display_watchdog_timer = xTimerCreate(
      "DisplayWatchdog",        // Timer name
      pdMS_TO_TICKS(5000),      // Timer period (5 seconds)
      pdTRUE,                   // Auto-reload
      NULL,                     // Timer ID
      display_watchdog_callback // Callback function
  );

  if (display_watchdog_timer != NULL)
  {
    xTimerStart(display_watchdog_timer, 0);
    ESP_LOGI(TAG, "Display watchdog initialized");
  }
  else
  {
    ESP_LOGE(TAG, "Failed to create display watchdog timer");
  }
}

/**
 * @brief Simple WiFi status update callback for UI
 */
static void ui_wifi_status_callback(const char *status_text, bool is_connected)
{
  ESP_LOGI(TAG, "WiFi UI update: %s (connected: %s)", status_text, is_connected ? "yes" : "no");
  system_monitor_ui_update_wifi_status(status_text, is_connected);
}

void app_main(void)
{
  ESP_LOGI(TAG, "System Monitor Dashboard started!");

  // Initial memory analysis - before any major allocations
  ESP_LOGI(TAG, "=== INITIAL MEMORY STATE ===");
  ESP_LOGI(TAG, "Total free heap: %zu bytes", esp_get_free_heap_size());
  ESP_LOGI(TAG, "Internal RAM free: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  ESP_LOGI(TAG, "SPIRAM free: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
  ESP_LOGI(TAG, "Internal largest block: %zu bytes", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));

  // Initialize and turn off LCD backlight
  lvgl_setup_init_backlight();
  lvgl_setup_set_backlight(LCD_BK_LIGHT_OFF_LEVEL);

  // Memory check after backlight init
  ESP_LOGI(TAG, "After backlight init - Internal RAM: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

  // Create and initialize LCD panel
  esp_lcd_panel_handle_t panel_handle = lvgl_setup_create_lcd_panel();
  global_panel_handle = panel_handle; // Store for watchdog access

  // Memory check after LCD panel creation
  ESP_LOGI(TAG, "After LCD panel init - Internal RAM: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

  // Turn on LCD backlight
  lvgl_setup_set_backlight(LCD_BK_LIGHT_ON_LEVEL);

  // Initialize LVGL
  lv_display_t *display = lvgl_setup_init(panel_handle);

  // Memory check after LVGL init
  ESP_LOGI(TAG, "After LVGL init - Internal RAM: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

  // Initialize display health monitoring
  init_display_watchdog();

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

  // Memory check after touch init
  ESP_LOGI(TAG, "After touch init - Internal RAM: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

  // Create system monitor UI with thread safety and logging BEFORE starting LVGL task
  lvgl_setup_create_ui_safe(display, system_monitor_ui_create);

  // Memory check after UI creation
  ESP_LOGI(TAG, "After UI creation - Internal RAM: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

  // Start LVGL task on core 1 AFTER UI is created
  ESP_LOGI(TAG, "About to start LVGL task on core 1...");
  lvgl_setup_start_task();
  ESP_LOGI(TAG, "LVGL task started on core 1");

  // Memory check after LVGL task start
  ESP_LOGI(TAG, "After LVGL task start - Internal RAM: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

  // Initialize serial data handler
  ESP_ERROR_CHECK(serial_data_init());

  // Memory check after serial init
  ESP_LOGI(TAG, "After serial init - Internal RAM: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

  // Add small delay before WiFi initialization to let LCD stabilize
  ESP_LOGI(TAG, "Allowing LCD to stabilize before WiFi initialization...");
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  // Initialize WiFi manager
  ESP_LOGI(TAG, "Initializing WiFi...");
  ESP_ERROR_CHECK(wifi_manager_init());

  // Memory check after WiFi init
  ESP_LOGI(TAG, "After WiFi init - Internal RAM: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

  // Register simple UI callback for WiFi status updates
  ESP_ERROR_CHECK(wifi_manager_register_ui_callback(ui_wifi_status_callback));

  // Initialize Home Assistant task manager
  ESP_LOGI(TAG, "Initializing Home Assistant task manager...");
  ESP_ERROR_CHECK(ha_task_manager_init());

  // Register HA WiFi callback for automatic task management
  ESP_ERROR_CHECK(wifi_manager_register_ha_callback(ha_task_manager_wifi_callback));

  // Connect to WiFi if configured
#ifdef DEFAULT_WIFI_SSID
  if (strlen(DEFAULT_WIFI_SSID) > 0)
  {
    ESP_LOGI(TAG, "Connecting to WiFi: %s", DEFAULT_WIFI_SSID);
#ifdef DEFAULT_WIFI_PASSWORD
    wifi_manager_connect(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD);
#else
    wifi_manager_connect(DEFAULT_WIFI_SSID, NULL);
#endif
  }
#endif

  // Start receiving serial data
  serial_data_start_task();

  // Memory check after serial task start
  ESP_LOGI(TAG, "After serial task start - Internal RAM: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

  // Initialize Smart Home integration after WiFi is ready
  ESP_LOGI(TAG, "Initializing Smart Home integration...");
  esp_err_t ret = smart_home_init();
  if (ret == ESP_OK)
  {
    ESP_LOGI(TAG, "Smart Home integration initialized successfully");
  }
  else
  {
    ESP_LOGE(TAG, "Failed to initialize Smart Home integration: %s", esp_err_to_name(ret));
  }

  ESP_LOGI(TAG, "System monitor initialized and running");
  ESP_LOGI(TAG, "UI rendering running on core 1");

  // Print initial memory usage
  ha_task_manager_print_memory_usage();

  // Main loop - simple monitoring
  int loop_count = 0;
  while (1)
  {
    ESP_LOGD(TAG, "System monitor running...");
    loop_count++;

    // Print memory usage every 5 minutes (30 loops * 10 seconds)
    if (loop_count % 30 == 0)
    {
      ESP_LOGI(TAG, "=== Periodic Memory Report (Loop %d) ===", loop_count);
      ha_task_manager_print_memory_usage();
    }

    vTaskDelay(10000 / portTICK_PERIOD_MS); // Check every 10 seconds
  }
}
