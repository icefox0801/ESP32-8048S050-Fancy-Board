/**
 * @file smart_home.c
 * @brief Smart Home Integration Manager Implementation
 *
 * This module provides a high-level interface for smart home automation
 * including Home Assistant integration, device control, and sensor monitoring.
 *
 * @author System Monitor Dashboard
 * @date 2025-08-15
 */

#include "smart_home.h"
#include "smart_config.h"
#include "ha_api.h"
#include "esp_timer.h"
#include "system_debug_utils.h"
#include <esp_err.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ═══════════════════════════════════════════════════════════════════════════════
// PRIVATE VARIABLES
// ═══════════════════════════════════════════════════════════════════════════════

static bool smart_home_initialized = false;
static TaskHandle_t sync_task_handle = NULL;
static smart_home_status_callback_t status_callback = NULL;
static smart_home_states_sync_callback_t states_sync_callback = NULL;

// ═══════════════════════════════════════════════════════════════════════════════
// PRIVATE FUNCTION DECLARATIONS
// ═══════════════════════════════════════════════════════════════════════════════

static void sync_task_function(void *pvParameters);
static esp_err_t run_sync_states_task(void);

// ═══════════════════════════════════════════════════════════════════════════════
// PRIVATE FUNCTION IMPLEMENTATIONS
// ═══════════════════════════════════════════════════════════════════════════════

static void sync_task_function(void *pvParameters)
{
  while (1)
  {
    debug_log_info(DEBUG_TAG_SMART_HOME, "⏰ Running periodic switch state sync");
    smart_home_sync_switch_states();

    // Wait for 30 seconds before next sync
    vTaskDelay(pdMS_TO_TICKS(30000));
  }
}

static esp_err_t run_sync_states_task(void)
{
  BaseType_t result = xTaskCreate(
      sync_task_function,
      "SyncStatesTask",
      4096, // Stack size - increased for network operations
      NULL,
      2,                // Priority
      &sync_task_handle // Store task handle for cleanup
  );

  if (result != pdPASS)
  {
    debug_log_error(DEBUG_TAG_SMART_HOME, "Failed to create sync states task");
    return ESP_FAIL;
  }

  debug_log_info(DEBUG_TAG_SMART_HOME, "🔄 Started periodic switch sync task (30s interval)");
  return ESP_OK;
}

// ═══════════════════════════════════════════════════════════════════════════════
// PUBLIC FUNCTION IMPLEMENTATIONS
// ═══════════════════════════════════════════════════════════════════════════════

esp_err_t smart_home_init(void)
{
  if (smart_home_initialized)
  {
    debug_log_event(DEBUG_TAG_SMART_HOME, "Already initialized");
    return ESP_OK;
  }

  debug_log_startup(DEBUG_TAG_SMART_HOME, "SmartHome");

  // Initialize Home Assistant API
  esp_err_t ret = ha_api_init();
  if (ret != ESP_OK)
  {
    debug_log_error_f(DEBUG_TAG_SMART_HOME, "Failed to initialize HA API: %s", esp_err_to_name(ret));
    return ret;
  }

  smart_home_initialized = true;
  debug_log_event(DEBUG_TAG_SMART_HOME, "Smart Home integration initialized successfully");

  // Start periodic sync task
  ret = run_sync_states_task();
  if (ret != ESP_OK)
  {
    debug_log_error_f(DEBUG_TAG_SMART_HOME, "Failed to start sync task: %s", esp_err_to_name(ret));
    return ret;
  }

  return ESP_OK;
}

esp_err_t smart_home_deinit(void)
{
  if (!smart_home_initialized)
  {
    return ESP_OK;
  }

  debug_log_event(DEBUG_TAG_SMART_HOME, "Deinitializing integration");

  // Stop and cleanup sync task
  if (sync_task_handle != NULL)
  {
    vTaskDelete(sync_task_handle);
    sync_task_handle = NULL;
    debug_log_info(DEBUG_TAG_SMART_HOME, "🔄 Stopped periodic switch sync task");
  }

  // Cleanup Home Assistant API
  ha_api_deinit();

  smart_home_initialized = false;

  debug_log_event(DEBUG_TAG_SMART_HOME, "Integration deinitialized");
  return ESP_OK;
}

esp_err_t smart_home_control_switch(const char *entity_id, bool turn_on)
{
  if (!smart_home_initialized || !entity_id)
  {
    debug_log_error_f(DEBUG_TAG_SMART_HOME, "Invalid parameters - initialized: %d, entity_id: %s",
                      smart_home_initialized, entity_id ? entity_id : "NULL");
    return ESP_ERR_INVALID_ARG;
  }

  const char *action = turn_on ? "ON" : "OFF";
  const char *emoji = turn_on ? "🔵" : "🔴";

  debug_log_info_f(DEBUG_TAG_SMART_HOME, "%s SWITCH CONTROL: %s → %s", emoji, entity_id, action);

  esp_err_t result;
  if (turn_on)
  {
    result = ha_api_turn_on_switch(entity_id);
  }
  else
  {
    result = ha_api_turn_off_switch(entity_id);
  }

  if (result == ESP_OK)
  {
    debug_log_info_f(DEBUG_TAG_SMART_HOME, "✅ Switch %s turned %s successfully", entity_id, action);
  }
  else
  {
    debug_log_error_f(DEBUG_TAG_SMART_HOME, "❌ Failed to turn %s switch %s: %s", action, entity_id, esp_err_to_name(result));
  }

  return result;
}

esp_err_t smart_home_trigger_scene(void)
{
  if (!smart_home_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  debug_log_event(DEBUG_TAG_SMART_HOME, "Triggering scene button");

  // For scene entities, we use the scene.turn_on service
  ha_service_call_t scene_call = {
      .domain = "scene",
      .service = "turn_on",
      .entity_id = HA_ENTITY_D_ID};

  ha_api_response_t response;
  esp_err_t ret = ha_api_call_service(&scene_call, &response);

  return ret;
}

void smart_home_sync_switch_states(void)
{
  debug_log_info(DEBUG_TAG_HA_SYNC, "Performing immediate switch sync using bulk API");

  // Entity IDs from smart config
  const char *switch_entity_ids[] = {
      HA_ENTITY_A_ID, // Switch A
      HA_ENTITY_B_ID, // Switch B
      HA_ENTITY_C_ID  // Switch C
  };
  const int switch_count = sizeof(switch_entity_ids) / sizeof(switch_entity_ids[0]);

  // Fetch all switch states in one bulk request
  ha_entity_state_t switch_states[switch_count];
  esp_err_t ret = ha_api_get_multiple_entity_states(switch_entity_ids, switch_count, switch_states);

  if (ret == ESP_OK)
  {
    // Update UI with switch states
    bool switch_a_on = (strcmp(switch_states[0].state, "on") == 0);
    bool switch_b_on = (strcmp(switch_states[1].state, "on") == 0);
    bool switch_c_on = (strcmp(switch_states[2].state, "on") == 0);

    // Notify states sync callback if registered
    if (states_sync_callback)
    {
      bool switch_state_array[3] = {switch_a_on, switch_b_on, switch_c_on};
      states_sync_callback(switch_state_array, 3);
    }

    debug_log_info_f(DEBUG_TAG_HA_SYNC, "Immediate sync completed: %s=%s, %s=%s, %s=%s",
                     switch_entity_ids[0], switch_states[0].state,
                     switch_entity_ids[1], switch_states[1].state,
                     switch_entity_ids[2], switch_states[2].state);
  }
  else
  {
    debug_log_warning_f(DEBUG_TAG_HA_SYNC, "Immediate sync failed: %s", esp_err_to_name(ret));
  }
}

void smart_home_update_wifi_status(bool is_connected)
{
  debug_log_info_f(DEBUG_TAG_SMART_HOME, "HA status changed: %s", is_connected ? "offline" : "disconnected");

  // Notify status callback about WiFi status changes
  if (status_callback)
  {
    const char *status_msg = is_connected ? "HA Ready" : "HA Offline";
    status_callback(is_connected && smart_home_initialized, status_msg);
  }
}

void smart_home_register_status_callback(smart_home_status_callback_t callback)
{
  status_callback = callback;
  debug_log_info_f(DEBUG_TAG_SMART_HOME, "Status callback %s", callback ? "registered" : "unregistered");

  // If registering a callback and we're already initialized, notify current status
  if (callback && smart_home_initialized)
  {
    callback(true, "Smart Home Ready");
  }
}

void smart_home_register_states_sync_callback(smart_home_states_sync_callback_t callback)
{
  states_sync_callback = callback;
  debug_log_info_f(DEBUG_TAG_SMART_HOME, "States sync callback %s", callback ? "registered" : "unregistered");
}
