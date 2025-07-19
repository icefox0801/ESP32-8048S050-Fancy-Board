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
#include "ui_dashboard.h"
#include "ui_controls_panel.h"
#include "system_debug_utils.h"
#include "esp_task_wdt.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

// Task management
static TaskHandle_t ha_task_handle = NULL;
static bool ha_initialized = false;
static bool immediate_sync_requested = false;
static bool ha_init_requested = false;

// Pre-allocated HTTP response buffer for large HA API responses
static char *http_response_buffer = NULL;
#define HTTP_RESPONSE_BUFFER_SIZE 131072 // 128KB

/**
 * @brief Home Assistant task to sync switch states every 30 seconds using bulk API
 */
static void home_assistant_task(void *pvParameters)
{
  debug_log_startup(DEBUG_TAG_HA_TASK_MGR, "HA Task");

  esp_err_t wdt_err = esp_task_wdt_add(NULL);
  if (wdt_err != ESP_OK)
  {
    debug_log_error(DEBUG_TAG_HA_TASK_MGR, "Watchdog subscription failed");
  }

  int cycle_count = 0;
  const int SYNC_INTERVAL_MS = 30000;
  bool initial_sync_done = false;

  const char *switch_entity_ids[] = {
      HA_ENTITY_A_ID,
      HA_ENTITY_B_ID,
      HA_ENTITY_C_ID,
  };
  const int switch_count = sizeof(switch_entity_ids) / sizeof(switch_entity_ids[0]);

  while (1)
  {
    if (ha_init_requested && !ha_initialized)
    {
      ha_init_requested = false;
      esp_task_wdt_reset();

      esp_err_t ret = ha_api_init();

      if (ret == ESP_OK)
      {
        ha_initialized = true;
        debug_log_event(DEBUG_TAG_HA_TASK_MGR, "HA API initialized");
        controls_panel_update_ha_status("Connected", true);
        immediate_sync_requested = true;
      }
      else
      {
        debug_log_error(DEBUG_TAG_HA_TASK_MGR, "HA API init failed");
        controls_panel_update_ha_status("Failed", false);
      }

      esp_task_wdt_reset();
    }

    if (immediate_sync_requested && ha_initialized)
    {
      immediate_sync_requested = false;
      esp_task_wdt_reset();

      esp_err_t ret = ha_sync_immediate_switches();
      esp_task_wdt_reset();

      if (ret != ESP_OK)
      {
        debug_log_error(DEBUG_TAG_HA_TASK_MGR, "Immediate sync failed");
        controls_panel_update_ha_status("Sync Error", false);
      }
      else
      {
        controls_panel_update_ha_status("Connected", true);
      }

      vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelay(SYNC_INTERVAL_MS / portTICK_PERIOD_MS);

    cycle_count++;
    if (cycle_count % 10 == 0)
    {
      debug_check_task_health(DEBUG_TAG_HA_TASK_MGR);

      if (cycle_count >= 1000)
      {
        cycle_count = 0;
      }
    }

    if (!ha_initialized)
    {
      continue;
    }

    esp_task_wdt_reset();
    controls_panel_update_ha_status("Syncing", true);

    ha_entity_state_t switch_states[switch_count];
    esp_err_t ret = ha_api_get_multiple_entity_states(switch_entity_ids, switch_count, switch_states);

    esp_task_wdt_reset();

    if (ret == ESP_OK)
    {
      bool switch_a_on = (strcmp(switch_states[0].state, "on") == 0);
      bool switch_b_on = (strcmp(switch_states[1].state, "on") == 0);
      bool switch_c_on = (strcmp(switch_states[2].state, "on") == 0);

      controls_panel_set_switch(SWITCH_A, switch_a_on);
      controls_panel_set_switch(SWITCH_B, switch_b_on);
      controls_panel_set_switch(SWITCH_C, switch_c_on);

      controls_panel_update_ha_status("Connected", true);

      if (!initial_sync_done)
      {
        initial_sync_done = true;
        debug_log_event(DEBUG_TAG_HA_TASK_MGR, "Initial sync completed");
      }
    }
    else
    {
      controls_panel_update_ha_status("Sync Error", false);
      debug_log_error(DEBUG_TAG_HA_TASK_MGR, "Bulk sync failed, trying individual requests");

      esp_task_wdt_reset();

      ha_entity_state_t switch_a_state;
      memset(&switch_a_state, 0, sizeof(switch_a_state));
      if (ha_api_get_entity_state(switch_entity_ids[0], &switch_a_state) == ESP_OK)
      {
        bool switch_a_on = (strcmp(switch_a_state.state, "on") == 0);
        controls_panel_set_switch(SWITCH_A, switch_a_on);
      }

      vTaskDelay(pdMS_TO_TICKS(200));
      esp_task_wdt_reset();

      ha_entity_state_t switch_b_state;
      memset(&switch_b_state, 0, sizeof(switch_b_state));
      if (ha_api_get_entity_state(switch_entity_ids[1], &switch_b_state) == ESP_OK)
      {
        bool switch_b_on = (strcmp(switch_b_state.state, "on") == 0);
        controls_panel_set_switch(SWITCH_B, switch_b_on);
      }

      vTaskDelay(pdMS_TO_TICKS(200));
      esp_task_wdt_reset();

      ha_entity_state_t switch_c_state;
      memset(&switch_c_state, 0, sizeof(switch_c_state));
      if (ha_api_get_entity_state(switch_entity_ids[2], &switch_c_state) == ESP_OK)
      {
        bool switch_c_on = (strcmp(switch_c_state.state, "on") == 0);
        controls_panel_set_switch(SWITCH_C, switch_c_on);
      }
    }

    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

esp_err_t ha_task_manager_init(void)
{
  debug_log_startup(DEBUG_TAG_HA_TASK_MGR, "Home Assistant Task Manager");
  debug_log_info(DEBUG_TAG_HA_TASK_MGR, "Initializing Home Assistant task manager");

  ha_initialized = false;
  immediate_sync_requested = false;
  ha_init_requested = false;
  ha_task_handle = NULL;

  // Update UI status
  controls_panel_update_ha_status("Offline", false);

  debug_log_event(DEBUG_TAG_HA_TASK_MGR, "Task manager initialized");
  return ESP_OK;
}

esp_err_t ha_task_manager_deinit(void)
{
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
    debug_log_error(DEBUG_TAG_HA_TASK_MGR, "Task already running");
    return ESP_ERR_INVALID_STATE;
  }

  vTaskDelay(pdMS_TO_TICKS(100));

  if (!debug_check_heap_sufficient(DEBUG_TAG_HA_TASK_MGR, 20000))
  {
    return ESP_ERR_NO_MEM;
  }

  controls_panel_update_ha_status("Starting", false);

  if (http_response_buffer == NULL)
  {
    http_response_buffer = (char *)heap_caps_malloc(HTTP_RESPONSE_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
    if (http_response_buffer == NULL)
    {
      debug_log_error(DEBUG_TAG_HA_TASK_MGR, "Failed to allocate HTTP buffer");
    }
  }

  uint32_t stack_size = 12288;

  BaseType_t result = xTaskCreatePinnedToCore(
      home_assistant_task,
      "ha_task",
      stack_size / sizeof(StackType_t),
      NULL,
      1,
      &ha_task_handle,
      tskNO_AFFINITY);

  if (result != pdPASS)
  {
    debug_log_error(DEBUG_TAG_HA_TASK_MGR, "Failed to create HA task");
    ha_task_handle = NULL;
    controls_panel_update_ha_status("Failed", false);
    return ESP_FAIL;
  }

  debug_log_event(DEBUG_TAG_HA_TASK_MGR, "HA task started");
  controls_panel_update_ha_status("Ready", false);
  return ESP_OK;
}

esp_err_t ha_task_manager_stop_task(void)
{
  if (ha_task_handle == NULL)
  {
    return ESP_ERR_INVALID_STATE;
  }

  controls_panel_update_ha_status("Stopping", false);

  esp_task_wdt_delete(ha_task_handle);
  vTaskDelete(ha_task_handle);
  ha_task_handle = NULL;
  ha_initialized = false;
  ha_init_requested = false;
  immediate_sync_requested = false;

  debug_log_event(DEBUG_TAG_HA_TASK_MGR, "HA task stopped");
  controls_panel_update_ha_status("Offline", false);
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
    return ESP_ERR_INVALID_STATE;
  }

  immediate_sync_requested = true;
  return ESP_OK;
}

esp_err_t ha_task_manager_request_init(void)
{
  if (ha_task_handle == NULL)
  {
    return ESP_ERR_INVALID_STATE;
  }

  ha_init_requested = true;
  return ESP_OK;
}

void ha_task_manager_wifi_callback(bool is_connected)
{
  debug_log_event(DEBUG_TAG_HA_TASK_MGR, is_connected ? "WiFi connected" : "WiFi disconnected");

  if (is_connected)
  {
    if (!ha_task_manager_is_task_running())
    {
      esp_err_t ret = ha_task_manager_start_task();
      if (ret != ESP_OK)
      {
        debug_log_error(DEBUG_TAG_HA_TASK_MGR, "Failed to start HA task after WiFi connection");
        return;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    esp_err_t init_ret = ha_task_manager_request_init();
    if (init_ret != ESP_OK)
    {
      debug_log_error(DEBUG_TAG_HA_TASK_MGR, "Failed to request HA initialization");
    }
  }
  else
  {
    if (ha_task_manager_is_task_running())
    {
      ha_task_manager_stop_task();
    }
  }
}
