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
#include "smart/ha_api.h"
#include "smart/ha_sync.h"
#include "smart/smart_config.h"
#include <stdio.h>

static const char *TAG = "dashboard";

// Display health monitoring
static TimerHandle_t display_watchdog_timer = NULL;
static esp_lcd_panel_handle_t global_panel_handle = NULL;

// Home Assistant integration
static TaskHandle_t ha_task_handle = NULL;
static bool ha_initialized = false;
static bool immediate_sync_requested = false;
static bool ha_init_requested = false;

/**
 * @brief Simple stack monitoring function
 */
static void check_stack_health(void)
{
  UBaseType_t current_stack = uxTaskGetStackHighWaterMark(NULL);
  size_t free_heap = esp_get_free_heap_size();

  ESP_LOGI(TAG, "Stack remaining: %lu bytes, Free heap: %u bytes",
           current_stack, (unsigned)free_heap);

  if (current_stack < 512)
  {
    ESP_LOGW(TAG, "WARNING: Low stack - only %lu bytes remaining!", current_stack);
  }

  if (free_heap < 50000)
  {
    ESP_LOGW(TAG, "WARNING: Low heap - only %u bytes free!", (unsigned)free_heap);
  }
}

/**
 * @brief Home Assistant task to sync switch states every 30 seconds using bulk API
 */
static void home_assistant_task(void *pvParameters)
{
  ESP_LOGI(TAG, "Home Assistant task started with 8KB stack");

  // Subscribe to task watchdog
  ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
  ESP_LOGI(TAG, "Home Assistant task subscribed to watchdog");

  int cycle_count = 0;
  const int SYNC_INTERVAL_MS = 30000; // 30 seconds for switch sync
  bool initial_sync_done = false;

  // Entity IDs from smart config
  const char *switch_entity_ids[] = {
      HA_ENTITY_A, // Water pump
      HA_ENTITY_B, // Wave maker
      HA_ENTITY_C  // Light switch
  };
  const int switch_count = sizeof(switch_entity_ids) / sizeof(switch_entity_ids[0]);

  while (1)
  {
    // Check for HA initialization request first
    if (ha_init_requested && !ha_initialized)
    {
      ESP_LOGI(TAG, "Processing HA API initialization request");
      ha_init_requested = false;

      // Feed watchdog before potentially long operation
      esp_task_wdt_reset();

      esp_err_t ret = ha_api_init();
      if (ret == ESP_OK)
      {
        ha_initialized = true;
        ESP_LOGI(TAG, "Home Assistant API initialized successfully in task");

        // Request immediate sync after successful initialization
        immediate_sync_requested = true;
      }
      else
      {
        ESP_LOGE(TAG, "Failed to initialize Home Assistant API in task: %s", esp_err_to_name(ret));
      }

      // Feed watchdog after operation
      esp_task_wdt_reset();
    }

    // Check for immediate sync request first (non-blocking)
    if (immediate_sync_requested && ha_initialized)
    {
      ESP_LOGI(TAG, "Processing immediate sync request");
      immediate_sync_requested = false;

      // Feed watchdog before potentially long operation
      esp_task_wdt_reset();

      // Perform immediate sync with error handling
      esp_err_t ret = ha_sync_immediate_switches();
      if (ret != ESP_OK)
      {
        ESP_LOGW(TAG, "Immediate sync failed: %s", esp_err_to_name(ret));
      }

      // Feed watchdog after operation
      esp_task_wdt_reset();

      // Small delay after immediate sync
      vTaskDelay(pdMS_TO_TICKS(1000));
    } // Wait for 30 seconds between sync cycles
    vTaskDelay(SYNC_INTERVAL_MS / portTICK_PERIOD_MS);

    // Check stack and memory health every 10 cycles (5 minutes)
    cycle_count++;
    if (cycle_count % 10 == 0)
    {
      check_stack_health();

      // Log free heap memory to monitor for memory leaks
      size_t free_heap = esp_get_free_heap_size();
      size_t min_free_heap = esp_get_minimum_free_heap_size();
      ESP_LOGI(TAG, "Memory status: Free=%zu bytes, Min=%zu bytes", free_heap, min_free_heap);

      // Reset cycle count to prevent overflow
      if (cycle_count >= 1000)
      {
        cycle_count = 0;
      }
    }

    if (!ha_initialized)
    {
      ESP_LOGW(TAG, "HA API not initialized, skipping device state fetch");
      continue;
    }

    ESP_LOGI(TAG, "Syncing switch states from Home Assistant (cycle %d)", cycle_count);

    // Feed watchdog before bulk operation
    esp_task_wdt_reset();

    // Fetch all switch states in one bulk request
    ha_entity_state_t switch_states[switch_count];
    esp_err_t ret = ha_api_get_multiple_entity_states(switch_entity_ids, switch_count, switch_states);

    // Feed watchdog after bulk operation
    esp_task_wdt_reset();

    if (ret == ESP_OK)
    {
      // Update UI with switch states
      bool pump_on = (strcmp(switch_states[0].state, "on") == 0);
      bool wave_on = (strcmp(switch_states[1].state, "on") == 0);
      bool light_on = (strcmp(switch_states[2].state, "on") == 0);

      system_monitor_ui_set_water_pump(pump_on);
      system_monitor_ui_set_wave_maker(wave_on);
      system_monitor_ui_set_light(light_on);

      ESP_LOGI(TAG, "Switch states synced: Pump=%s, Wave=%s, Light=%s",
               switch_states[0].state, switch_states[1].state, switch_states[2].state);

      if (!initial_sync_done)
      {
        initial_sync_done = true;
        ESP_LOGI(TAG, "Initial switch sync completed successfully");
      }
    }
    else
    {
      ESP_LOGW(TAG, "Failed to sync switch states: %s", esp_err_to_name(ret));

      // Fallback to individual requests if bulk fails
      ESP_LOGI(TAG, "Attempting individual entity requests as fallback");

      // Water pump
      ha_entity_state_t pump_state;
      memset(&pump_state, 0, sizeof(pump_state));
      if (ha_api_get_entity_state(switch_entity_ids[0], &pump_state) == ESP_OK)
      {
        bool pump_on = (strcmp(pump_state.state, "on") == 0);
        system_monitor_ui_set_water_pump(pump_on);
        ESP_LOGD(TAG, "Water pump: %s", pump_state.state);
      }

      vTaskDelay(pdMS_TO_TICKS(200));

      // Wave maker
      ha_entity_state_t wave_state;
      memset(&wave_state, 0, sizeof(wave_state));
      if (ha_api_get_entity_state(switch_entity_ids[1], &wave_state) == ESP_OK)
      {
        bool wave_on = (strcmp(wave_state.state, "on") == 0);
        system_monitor_ui_set_wave_maker(wave_on);
        ESP_LOGD(TAG, "Wave maker: %s", wave_state.state);
      }

      vTaskDelay(pdMS_TO_TICKS(200));

      // Aquarium light
      ha_entity_state_t light_state;
      memset(&light_state, 0, sizeof(light_state));
      if (ha_api_get_entity_state(switch_entity_ids[2], &light_state) == ESP_OK)
      {
        bool light_on = (strcmp(light_state.state, "on") == 0);
        system_monitor_ui_set_light(light_on);
        ESP_LOGD(TAG, "Aquarium light: %s", light_state.state);
      }
    }

    // Fetch sensors every other cycle to reduce load
    if (cycle_count % 2 == 0)
    {
      // Small delay before sensor requests
      vTaskDelay(pdMS_TO_TICKS(500));

      // Fetch temperature sensor
      ha_entity_state_t temp_state;
      memset(&temp_state, 0, sizeof(temp_state));
      ret = ha_api_get_entity_state("sensor.aquarium_temperature", &temp_state);
      if (ret == ESP_OK)
      {
        float temperature = atof(temp_state.state);
        ESP_LOGD(TAG, "Temperature: %.1f°C", temperature);
        // TODO: system_monitor_ui_set_temperature(temperature);
      }

      vTaskDelay(pdMS_TO_TICKS(300));

      // Fetch humidity sensor
      ha_entity_state_t humidity_state;
      memset(&humidity_state, 0, sizeof(humidity_state));
      ret = ha_api_get_entity_state("sensor.aquarium_humidity", &humidity_state);
      if (ret == ESP_OK)
      {
        float humidity = atof(humidity_state.state);
        ESP_LOGD(TAG, "Humidity: %.1f%%", humidity);
        // TODO: system_monitor_ui_set_humidity(humidity);
      }
    }

    ESP_LOGI(TAG, "Device state sync completed (cycle %d)", cycle_count);

    // Feed task watchdog to prevent reset
    esp_task_wdt_reset();

    // Small garbage collection pause
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

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
 * @brief WiFi status callback to update UI directly
 */
static void wifi_status_callback(wifi_status_t status, const wifi_info_t *info)
{
  ESP_LOGI(TAG, "WiFi status callback called with status %d", status);

  // Update UI directly within LVGL task context
  // This is safe as long as we're not blocking
  const char *status_text;
  bool connected = false;

  switch (status)
  {
  case WIFI_STATUS_DISCONNECTED:
    ESP_LOGI(TAG, "WiFi Status: Disconnected");
    status_text = "Disconnected";

    // Stop Home Assistant task when WiFi disconnects
    if (ha_task_handle != NULL)
    {
      // Unsubscribe from watchdog before deleting
      esp_task_wdt_delete(ha_task_handle);
      vTaskDelete(ha_task_handle);
      ha_task_handle = NULL;
      ha_initialized = false;
      ha_init_requested = false;
      immediate_sync_requested = false;
      ESP_LOGI(TAG, "Home Assistant task stopped due to WiFi disconnection");
    }
    break;

  case WIFI_STATUS_CONNECTING:
    ESP_LOGI(TAG, "WiFi Status: Connecting...");
    status_text = "Connecting...";
    break;

  case WIFI_STATUS_CONNECTED:
    connected = true;
    if (info)
    {
      ESP_LOGI(TAG, "WiFi Status: Connected to %s (IP: %s, RSSI: %ddBm)",
               info->ssid, info->ip_address, info->rssi);

      // Create a detailed status message
      static char detailed_status[128];
      snprintf(detailed_status, sizeof(detailed_status), "Connected: %s (%s)",
               info->ssid, info->ip_address);
      status_text = detailed_status;

      // Initialize Home Assistant API after WiFi connection
      if (!ha_initialized)
      {
        // Create Home Assistant task first
        if (ha_task_handle == NULL)
        {
          xTaskCreate(home_assistant_task, "ha_task", 8192, NULL, 5, &ha_task_handle);
          ESP_LOGI(TAG, "Home Assistant task created with 8KB stack");
        }

        // Request HA initialization in task context (safer)
        ha_init_requested = true;
        ESP_LOGI(TAG, "HA initialization requested in task context");
      }
      else
      {
        // HA already initialized, just request immediate sync on reconnect
        ESP_LOGI(TAG, "WiFi reconnected - requesting immediate switch sync");
        immediate_sync_requested = true;
      }
    }
    else
    {
      status_text = "Connected";
    }
    break;

  case WIFI_STATUS_FAILED:
    ESP_LOGE(TAG, "WiFi Status: Connection Failed");
    status_text = "Connection Failed";
    break;

  case WIFI_STATUS_RECONNECTING:
    ESP_LOGI(TAG, "WiFi Status: Reconnecting...");
    status_text = "Reconnecting...";
    break;

  default:
    ESP_LOGW(TAG, "WiFi Status: Unknown (%d)", status);
    status_text = "Unknown";
    break;
  }

  // Update UI with WiFi status
  system_monitor_ui_update_wifi_status(status_text, connected);
}
void app_main(void)
{
  ESP_LOGI(TAG, "System Monitor Dashboard started!");

  // Initialize and turn off LCD backlight
  lvgl_setup_init_backlight();
  lvgl_setup_set_backlight(LCD_BK_LIGHT_OFF_LEVEL);

  // Create and initialize LCD panel
  esp_lcd_panel_handle_t panel_handle = lvgl_setup_create_lcd_panel();
  global_panel_handle = panel_handle; // Store for watchdog access

  // Turn on LCD backlight
  lvgl_setup_set_backlight(LCD_BK_LIGHT_ON_LEVEL);

  // Initialize LVGL
  lv_display_t *display = lvgl_setup_init(panel_handle);

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

  // Create system monitor UI with thread safety and logging BEFORE starting LVGL task
  lvgl_setup_create_ui_safe(display, system_monitor_ui_create);

  // Start LVGL task on core 1 AFTER UI is created
  ESP_LOGI(TAG, "About to start LVGL task on core 1...");
  lvgl_setup_start_task();
  ESP_LOGI(TAG, "LVGL task started on core 1");

  // Initialize serial data handler
  ESP_ERROR_CHECK(serial_data_init());

  // Add small delay before WiFi initialization to let LCD stabilize
  ESP_LOGI(TAG, "Allowing LCD to stabilize before WiFi initialization...");
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  // Initialize WiFi manager
  ESP_LOGI(TAG, "Initializing WiFi...");
  ESP_ERROR_CHECK(wifi_manager_init());

  // Register WiFi status callback for direct UI updates
  ESP_ERROR_CHECK(wifi_manager_register_callback(wifi_status_callback));

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

  ESP_LOGI(TAG, "System monitor initialized and running");
  ESP_LOGI(TAG, "UI rendering running on core 1");

  // Main loop - simple monitoring
  while (1)
  {
    ESP_LOGD(TAG, "System monitor running...");
    vTaskDelay(10000 / portTICK_PERIOD_MS); // Check every 10 seconds
  }
}
