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
#include "ha_sync.h"
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

  return ESP_OK;
}

esp_err_t smart_home_deinit(void)
{
  if (!smart_home_initialized)
  {
    return ESP_OK;
  }

  debug_log_event(DEBUG_TAG_SMART_HOME, "Deinitializing integration");

  // Cleanup Home Assistant API
  ha_api_deinit();

  smart_home_initialized = false;

  debug_log_event(DEBUG_TAG_SMART_HOME, "Integration deinitialized");
  return ESP_OK;
}

esp_err_t smart_home_test_connection(void)
{
  if (!smart_home_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  // Test connection by getting state of a known entity
  ha_entity_state_t test_state;
  esp_err_t ret = ha_api_get_entity_state(HA_ENTITY_A_ID, &test_state);

  if (ret == ESP_OK)
  {
    debug_log_event(DEBUG_TAG_SMART_HOME, "Home Assistant connection test successful");
  }
  else
  {
    debug_log_error_f(DEBUG_TAG_SMART_HOME, "Home Assistant connection test failed: %s", esp_err_to_name(ret));
  }

  return ret;
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
