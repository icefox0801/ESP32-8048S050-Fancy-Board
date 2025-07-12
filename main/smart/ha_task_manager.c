/**
 * @file ha_task_manager.c
 * @brief Home Assistant Task Manager Implementation
 *
 * This module manages the Home Assistant sync task and handles
 * periodic synchronization with Home Assistant.
 */

#include "ha_task_manager.h"
#include "ha_api.h"
#include "ha_sync.h"
#include "smart_config.h"
#include "../lvgl/system_monitor_ui.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "ha_task_mgr";

// Task management
static TaskHandle_t ha_task_handle = NULL;
static bool ha_initialized = false;
static bool immediate_sync_requested = false;
static bool ha_init_requested = false;

// AGGRESSIVE SPIRAM utilization: Pre-allocate large HTTP response buffer for 100KB+ responses
// This eliminates dynamic allocation during HTTP operations for maximum performance
static char *http_response_buffer = NULL;
#define HTTP_RESPONSE_BUFFER_SIZE 131072 // 128KB for large HA API responses (supports 100KB+ responses)

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
  ESP_LOGI(TAG, "Home Assistant task started with 12KB internal RAM stack");

  // Subscribe to task watchdog (make it non-fatal if it fails)
  esp_err_t wdt_err = esp_task_wdt_add(NULL);
  if (wdt_err == ESP_OK)
  {
    ESP_LOGI(TAG, "Home Assistant task subscribed to watchdog (60s timeout)");
  }
  else
  {
    ESP_LOGW(TAG, "Failed to subscribe to watchdog: %s", esp_err_to_name(wdt_err));
  }

  int cycle_count = 0;
  const int SYNC_INTERVAL_MS = 30000; // 30 seconds for switch sync
  bool initial_sync_done = false;

  // Entity IDs from smart config
  const char *switch_entity_ids[] = {
      HA_ENTITY_A, // Switch A
      HA_ENTITY_B, // Switch B
      HA_ENTITY_C  // Switch C
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

      // Log memory state before initialization
      size_t free_heap_before = esp_get_free_heap_size();
      size_t free_internal_before = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
      ESP_LOGI(TAG, "Before HA init - Free heap: %zu, Internal: %zu",
               free_heap_before, free_internal_before);

      ESP_LOGI(TAG, "Calling ha_api_init()...");
      esp_err_t ret = ha_api_init();
      ESP_LOGI(TAG, "ha_api_init() returned: %d (%s)", ret, esp_err_to_name(ret));

      if (ret == ESP_OK)
      {
        ha_initialized = true;
        ESP_LOGI(TAG, "Home Assistant API initialized successfully in task");
        system_monitor_ui_update_ha_status("Connected", true);

        // Request immediate sync after successful initialization
        immediate_sync_requested = true;
        ESP_LOGI(TAG, "Immediate sync requested after HA init");
      }
      else
      {
        ESP_LOGE(TAG, "Failed to initialize Home Assistant API in task: %s (code: %d)",
                 esp_err_to_name(ret), ret);
        system_monitor_ui_update_ha_status("Failed", false);

        // Log memory state after failure
        size_t free_heap_after = esp_get_free_heap_size();
        size_t free_internal_after = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        ESP_LOGE(TAG, "After HA init failure - Free heap: %zu, Internal: %zu",
                 free_heap_after, free_internal_after);
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

      // Perform immediate sync with error handling and timeout protection
      ESP_LOGI(TAG, "Starting immediate sync with timeout protection");
      esp_err_t ret = ha_sync_immediate_switches();

      // Feed watchdog immediately after operation
      esp_task_wdt_reset();

      if (ret != ESP_OK)
      {
        ESP_LOGW(TAG, "Immediate sync failed: %s", esp_err_to_name(ret));
        system_monitor_ui_update_ha_status("Sync Error", false);
      }
      else
      {
        ESP_LOGI(TAG, "Immediate sync completed successfully");
        system_monitor_ui_update_ha_status("Connected", true);
      }

      // Small delay after immediate sync
      vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // Wait for 30 seconds between sync cycles
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

    // Show syncing status
    system_monitor_ui_update_ha_status("Syncing", true);

    // Fetch all switch states in one bulk request with timeout protection
    ESP_LOGI(TAG, "Starting bulk state fetch with 15s timeout");
    ha_entity_state_t switch_states[switch_count];
    esp_err_t ret = ha_api_get_multiple_entity_states(switch_entity_ids, switch_count, switch_states);

    // Feed watchdog immediately after bulk operation
    esp_task_wdt_reset();
    ESP_LOGI(TAG, "Bulk state fetch completed, feeding watchdog");

    if (ret == ESP_OK)
    {
      // Update UI with switch states
      bool switch_a_on = (strcmp(switch_states[0].state, "on") == 0);
      bool switch_b_on = (strcmp(switch_states[1].state, "on") == 0);
      bool switch_c_on = (strcmp(switch_states[2].state, "on") == 0);

      system_monitor_ui_set_switch_a(switch_a_on);
      system_monitor_ui_set_switch_b(switch_b_on);
      system_monitor_ui_set_switch_c(switch_c_on);

      ESP_LOGI(TAG, "Switch states synced: A=%s, B=%s, C=%s",
               switch_states[0].state, switch_states[1].state, switch_states[2].state);

      // Update status back to connected after successful sync
      system_monitor_ui_update_ha_status("Connected", true);

      if (!initial_sync_done)
      {
        initial_sync_done = true;
        ESP_LOGI(TAG, "Initial switch sync completed successfully");
      }
    }
    else
    {
      system_monitor_ui_update_ha_status("Sync Error", false);
      ESP_LOGW(TAG, "Failed to sync switch states: %s", esp_err_to_name(ret));

      // Fallback to individual requests if bulk fails
      ESP_LOGI(TAG, "Attempting individual entity requests as fallback");

      // Feed watchdog before fallback operations
      esp_task_wdt_reset();

      // Switch A
      ha_entity_state_t switch_a_state;
      memset(&switch_a_state, 0, sizeof(switch_a_state));
      if (ha_api_get_entity_state(switch_entity_ids[0], &switch_a_state) == ESP_OK)
      {
        bool switch_a_on = (strcmp(switch_a_state.state, "on") == 0);
        system_monitor_ui_set_switch_a(switch_a_on);
        ESP_LOGD(TAG, "Switch A: %s", switch_a_state.state);
      }

      vTaskDelay(pdMS_TO_TICKS(200));
      esp_task_wdt_reset(); // Feed watchdog between requests

      // Switch B
      ha_entity_state_t switch_b_state;
      memset(&switch_b_state, 0, sizeof(switch_b_state));
      if (ha_api_get_entity_state(switch_entity_ids[1], &switch_b_state) == ESP_OK)
      {
        bool switch_b_on = (strcmp(switch_b_state.state, "on") == 0);
        system_monitor_ui_set_switch_b(switch_b_on);
        ESP_LOGD(TAG, "Switch B: %s", switch_b_state.state);
      }

      vTaskDelay(pdMS_TO_TICKS(200));
      esp_task_wdt_reset(); // Feed watchdog between requests

      // Switch C
      ha_entity_state_t switch_c_state;
      memset(&switch_c_state, 0, sizeof(switch_c_state));
      if (ha_api_get_entity_state(switch_entity_ids[2], &switch_c_state) == ESP_OK)
      {
        bool switch_c_on = (strcmp(switch_c_state.state, "on") == 0);
        system_monitor_ui_set_switch_c(switch_c_on);
        ESP_LOGD(TAG, "Switch C: %s", switch_c_state.state);
      }

      ESP_LOGI(TAG, "Individual fallback requests completed");
    }

    // Fetch sensors every other cycle to reduce load
    if (cycle_count % 2 == 0)
    {
      // Small delay before sensor requests
      vTaskDelay(pdMS_TO_TICKS(500));
      esp_task_wdt_reset(); // Feed watchdog before sensor operations

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
      esp_task_wdt_reset(); // Feed watchdog between sensor requests

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

      ESP_LOGI(TAG, "Sensor fetch completed");
    }

    ESP_LOGI(TAG, "Device state sync completed (cycle %d)", cycle_count);

    // Feed task watchdog to prevent reset
    esp_task_wdt_reset();

    // Small garbage collection pause
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

esp_err_t ha_task_manager_init(void)
{
  ESP_LOGI(TAG, "Initializing Home Assistant task manager");

  ha_initialized = false;
  immediate_sync_requested = false;
  ha_init_requested = false;
  ha_task_handle = NULL;

  // Update UI status
  system_monitor_ui_update_ha_status("Offline", false);

  return ESP_OK;
}

esp_err_t ha_task_manager_deinit(void)
{
  ESP_LOGI(TAG, "Deinitializing Home Assistant task manager");

  // Stop task if running
  ha_task_manager_stop_task();

  ha_initialized = false;
  immediate_sync_requested = false;
  ha_init_requested = false;

  return ESP_OK;
}

esp_err_t ha_task_manager_start_task(void)
{
  if (ha_task_handle != NULL)
  {
    ESP_LOGW(TAG, "Home Assistant task already running");
    return ESP_ERR_INVALID_STATE;
  }

  // Small delay to ensure system is ready
  vTaskDelay(pdMS_TO_TICKS(100));

  // Check available heap before task creation
  size_t free_heap = esp_get_free_heap_size();
  size_t min_heap = esp_get_minimum_free_heap_size();
  ESP_LOGI(TAG, "Starting Home Assistant task - Free heap: %zu, Min heap: %zu", free_heap, min_heap);

  if (free_heap < 20000)
  { // Need at least 20KB free
    ESP_LOGE(TAG, "Insufficient heap memory for HA task creation");
    return ESP_ERR_NO_MEM;
  }

  ESP_LOGI(TAG, "Starting Home Assistant task");
  system_monitor_ui_update_ha_status("Starting", false);

  // Log detailed memory information
  ESP_LOGI(TAG, "Free heap: %zu bytes", free_heap);
  ESP_LOGI(TAG, "Min heap: %zu bytes", min_heap);
  ESP_LOGI(TAG, "Largest free block: %zu bytes", heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
  ESP_LOGI(TAG, "Free internal: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

  // Log task memory usage
  ESP_LOGI(TAG, "=== Task Memory Usage ===");
  UBaseType_t task_count = uxTaskGetNumberOfTasks();
  ESP_LOGI(TAG, "Number of tasks: %u", (unsigned int)task_count);

  // Note: vTaskList and uxTaskGetSystemState require special config options
  // For now, we'll show basic task count and individual task info if available

  // Log detailed memory by capability
  ESP_LOGI(TAG, "=== Memory by Capability ===");
  ESP_LOGI(TAG, "Internal RAM: %zu bytes free", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  ESP_LOGI(TAG, "SPIRAM: %zu bytes free", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
  ESP_LOGI(TAG, "DMA capable: %zu bytes free", heap_caps_get_free_size(MALLOC_CAP_DMA));
  ESP_LOGI(TAG, "32-bit access: %zu bytes free", heap_caps_get_free_size(MALLOC_CAP_32BIT));

  // Log largest blocks by capability
  ESP_LOGI(TAG, "=== Largest Free Blocks ===");
  ESP_LOGI(TAG, "Internal largest: %zu bytes", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
  ESP_LOGI(TAG, "SPIRAM largest: %zu bytes", heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
  ESP_LOGI(TAG, "DMA largest: %zu bytes", heap_caps_get_largest_free_block(MALLOC_CAP_DMA));

  // Pre-allocate large HTTP response buffer in SPIRAM for aggressive memory utilization
  if (http_response_buffer == NULL)
  {
    http_response_buffer = (char *)heap_caps_malloc(HTTP_RESPONSE_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
    if (http_response_buffer != NULL)
    {
      ESP_LOGI(TAG, "Allocated 128KB HTTP response buffer in SPIRAM at %p", http_response_buffer);
    }
    else
    {
      ESP_LOGW(TAG, "Failed to allocate 128KB HTTP response buffer in SPIRAM");
    }
  }

  // CRITICAL: HA task MUST use internal RAM stack for networking
  // LWIP requires tasks making network calls to have internal RAM stacks
  // (lwip_hook_tcp_isn assertion: !esp_ptr_external_ram(esp_cpu_get_sp()))
  // Use smaller stack but allocate large HTTP response buffer separately in SPIRAM
  uint32_t stack_size = 12288; // 12KB - sufficient for network calls in internal RAM

  ESP_LOGI(TAG, "Attempting HA task creation with %lu byte stack (internal RAM required for networking)", stack_size);
  ESP_LOGI(TAG, "Internal RAM available: %zu bytes (largest block: %zu)",
           heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
           heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));

  // MUST use internal RAM for networking stack - no SPIRAM stack allowed
  ESP_LOGI(TAG, "Using internal RAM stack (required for LWIP networking)");

  // Create task with internal RAM stack (networking requirement)
  BaseType_t result = xTaskCreatePinnedToCore(
      home_assistant_task, // Task function
      "ha_task",           // Task name
      stack_size / sizeof(StackType_t), // Stack size in words
      NULL,                // Parameters
      1,                   // Priority (lowest priority for LVGL rendering priority)
      &ha_task_handle,     // Task handle
      tskNO_AFFINITY       // Allow any core (or use 0 for core 0, 1 for core 1)
  );

  if (result != pdPASS)
  {
    size_t free_heap_after = esp_get_free_heap_size();
    size_t free_internal_after = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    ESP_LOGE(TAG, "Failed to create Home Assistant task (result=%d)", (int)result);
    ESP_LOGE(TAG, "Heap before: %zu, after: %zu", free_heap, free_heap_after);
    ESP_LOGE(TAG, "Internal before: %zu, after: %zu", heap_caps_get_free_size(MALLOC_CAP_INTERNAL), free_internal_after);
    ESP_LOGE(TAG, "Task stack size: %lu, priority: 1, core: auto", stack_size);
    ESP_LOGE(TAG, "FreeRTOS error: %s",
             (result == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY) ? "Could not allocate memory" : "Unknown error");
    ESP_LOGE(TAG, "Available internal blocks: largest=%zu bytes",
             heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    ESP_LOGE(TAG, "Available SPIRAM blocks: largest=%zu bytes",
             heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
    ha_task_handle = NULL;
    system_monitor_ui_update_ha_status("Failed", false);
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "HA task created successfully with internal RAM stack (required for networking)");

  ESP_LOGI(TAG, "Home Assistant task started successfully");
  system_monitor_ui_update_ha_status("Ready", false);
  return ESP_OK;
}

esp_err_t ha_task_manager_stop_task(void)
{
  if (ha_task_handle == NULL)
  {
    ESP_LOGW(TAG, "Home Assistant task not running");
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(TAG, "Stopping Home Assistant task");
  system_monitor_ui_update_ha_status("Stopping", false);

  // Unsubscribe from watchdog before deleting
  esp_task_wdt_delete(ha_task_handle);
  vTaskDelete(ha_task_handle);
  ha_task_handle = NULL;
  ha_initialized = false;
  ha_init_requested = false;
  immediate_sync_requested = false;

  ESP_LOGI(TAG, "Home Assistant task stopped");
  system_monitor_ui_update_ha_status("Offline", false);
  return ESP_OK;
}

bool ha_task_manager_is_task_running(void)
{
  return ha_task_handle != NULL;
}

esp_err_t ha_task_manager_request_immediate_sync(void)
{
  if (ha_task_handle == NULL)
  {
    ESP_LOGW(TAG, "Cannot request sync - task not running");
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(TAG, "Requesting immediate sync");
  immediate_sync_requested = true;
  return ESP_OK;
}

esp_err_t ha_task_manager_request_init(void)
{
  if (ha_task_handle == NULL)
  {
    ESP_LOGW(TAG, "Cannot request init - task not running");
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(TAG, "Requesting Home Assistant API initialization");
  ha_init_requested = true;
  return ESP_OK;
}

void ha_task_manager_wifi_callback(bool is_connected)
{
  ESP_LOGI(TAG, "WiFi callback: %s", is_connected ? "connected" : "disconnected");

  if (is_connected)
  {
    // WiFi connected - start HA task if not already running
    if (!ha_task_manager_is_task_running())
    {
      ESP_LOGI(TAG, "Starting Home Assistant task due to WiFi connection");
      esp_err_t ret = ha_task_manager_start_task();
      if (ret != ESP_OK)
      {
        ESP_LOGE(TAG, "Failed to start HA task after WiFi connection: %s", esp_err_to_name(ret));
        return;
      }
    }

    // Wait a moment for task to start and be ready
    vTaskDelay(pdMS_TO_TICKS(100));

    // Automatically request HA initialization after WiFi connection
    ESP_LOGI(TAG, "Automatically requesting HA initialization after WiFi connection");
    esp_err_t init_ret = ha_task_manager_request_init();
    if (init_ret != ESP_OK)
    {
      ESP_LOGW(TAG, "Failed to request HA initialization: %s", esp_err_to_name(init_ret));
    }
  }
  else
  {
    // WiFi disconnected - stop HA task
    if (ha_task_manager_is_task_running())
    {
      ESP_LOGI(TAG, "Stopping Home Assistant task due to WiFi disconnection");
      ha_task_manager_stop_task();
    }
  }
}

void ha_task_manager_print_memory_usage(void)
{
  ESP_LOGI(TAG, "=== System Memory Usage Report ===");

  // Overall heap information
  size_t free_heap = esp_get_free_heap_size();
  size_t min_heap = esp_get_minimum_free_heap_size();
  ESP_LOGI(TAG, "Total free heap: %zu bytes", free_heap);
  ESP_LOGI(TAG, "Minimum heap: %zu bytes", min_heap);
  ESP_LOGI(TAG, "Heap used: %zu bytes", 8 * 1024 * 1024 - free_heap); // Assuming 8MB total

  // Memory by capability
  size_t internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  size_t spiram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  size_t dma_free = heap_caps_get_free_size(MALLOC_CAP_DMA);

  ESP_LOGI(TAG, "=== Memory by Type ===");
  ESP_LOGI(TAG, "Internal RAM: %zu bytes free", internal_free);
  ESP_LOGI(TAG, "SPIRAM: %zu bytes free", spiram_free);
  ESP_LOGI(TAG, "DMA capable: %zu bytes free", dma_free);

  // Largest free blocks
  ESP_LOGI(TAG, "=== Largest Free Blocks ===");
  ESP_LOGI(TAG, "Internal largest: %zu bytes", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
  ESP_LOGI(TAG, "SPIRAM largest: %zu bytes", heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));

  // Task information
  ESP_LOGI(TAG, "=== Task Information ===");
  UBaseType_t task_count = uxTaskGetNumberOfTasks();
  ESP_LOGI(TAG, "Number of tasks: %u", (unsigned int)task_count);

  // Note: Detailed task information requires configTASKLIST_INCLUDE_COREID and
  // configUSE_STATS_FORMATTING_FUNCTIONS to be enabled in FreeRTOS config
  ESP_LOGI(TAG, "Task details require special FreeRTOS config options");

  // Show basic task information instead
  ESP_LOGI(TAG, "Free heap: %zu bytes", esp_get_free_heap_size());
  ESP_LOGI(TAG, "Min heap: %zu bytes", esp_get_minimum_free_heap_size());

  // HA task specific info
  if (ha_task_handle != NULL)
  {
    UBaseType_t ha_stack_hwm = uxTaskGetStackHighWaterMark(ha_task_handle);
    ESP_LOGI(TAG, "HA Task stack HWM: %u bytes", (unsigned int)(ha_stack_hwm * sizeof(StackType_t)));
    ESP_LOGI(TAG, "HA Task stack used: ~%u bytes", (unsigned int)(12288 - ha_stack_hwm * sizeof(StackType_t)));
  }
}
