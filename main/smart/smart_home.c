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
#include "esp_log.h"

#include <esp_log.h>
#include <esp_err.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "SmartHome";

// ═══════════════════════════════════════════════════════════════════════════════
// PRIVATE VARIABLES
// ═══════════════════════════════════════════════════════════════════════════════

static bool smart_home_initialized = false;
static smart_home_event_callback_t event_callback = NULL;
static void *callback_user_data = NULL;

// ═══════════════════════════════════════════════════════════════════════════════
// PUBLIC FUNCTION IMPLEMENTATIONS
// ═══════════════════════════════════════════════════════════════════════════════

esp_err_t smart_home_init(void)
{
  if (smart_home_initialized)
  {
    ESP_LOGW(TAG, "Smart home already initialized");
    return ESP_OK;
  }

  ESP_LOGI(TAG, "Initializing Smart Home integration");

  // Initialize Home Assistant API
  esp_err_t ret = ha_api_init();
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to initialize HA API: %s", esp_err_to_name(ret));
    return ret;
  }

  smart_home_initialized = true;
  ESP_LOGI(TAG, "Smart Home integration initialized successfully");

  return ESP_OK;
}

esp_err_t smart_home_deinit(void)
{
  if (!smart_home_initialized)
  {
    return ESP_OK;
  }

  ESP_LOGI(TAG, "Deinitializing Smart Home integration");

  // Cleanup Home Assistant API
  ha_api_deinit();

  smart_home_initialized = false;
  event_callback = NULL;
  callback_user_data = NULL;

  ESP_LOGI(TAG, "Smart Home integration deinitialized");
  return ESP_OK;
}

esp_err_t smart_home_get_status(smart_home_status_t *status)
{
  if (!smart_home_initialized || !status)
  {
    return ESP_ERR_INVALID_STATE;
  }

  // Initialize status structure
  memset(status, 0, sizeof(smart_home_status_t));

  // Test HA connection
  esp_err_t ret = smart_home_test_connection();
  status->ha_connected = (ret == ESP_OK);

  if (!status->ha_connected)
  {
    snprintf(status->connection_error, sizeof(status->connection_error),
             "Connection failed: %s", esp_err_to_name(ret));
  }

  // For now, set basic device counts (can be extended later)
  status->total_devices = 3; // Switch A, Switch B, Switch C
  status->online_devices = status->ha_connected ? 3 : 0;
  status->offline_devices = status->ha_connected ? 0 : 3;
  status->last_full_update = esp_timer_get_time() / 1000; // Convert to ms

  return ESP_OK;
}

esp_err_t smart_home_toggle_pump(void)
{
  if (!smart_home_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(TAG, "Toggling switch A");
  return ha_api_toggle_switch(HA_ENTITY_A);
}

esp_err_t smart_home_pump_on(void)
{
  if (!smart_home_initialized)
  {
    ESP_LOGE(TAG, "Smart home not initialized");
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(TAG, "🔵 USER ACTION: Turning on %s (Switch A: %s)", UI_LABEL_A, HA_ENTITY_A);
  esp_err_t result = ha_api_turn_on_switch(HA_ENTITY_A);

  if (result == ESP_OK)
  {
    ESP_LOGI(TAG, "✅ %s turned ON successfully", UI_LABEL_A);
  }
  else
  {
    ESP_LOGE(TAG, "❌ Failed to turn ON %s: %s", UI_LABEL_A, esp_err_to_name(result));
  }

  return result;
}

esp_err_t smart_home_pump_off(void)
{
  if (!smart_home_initialized)
  {
    ESP_LOGE(TAG, "Smart home not initialized");
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(TAG, "🔴 USER ACTION: Turning off %s (Switch A: %s)", UI_LABEL_A, HA_ENTITY_A);
  esp_err_t result = ha_api_turn_off_switch(HA_ENTITY_A);

  if (result == ESP_OK)
  {
    ESP_LOGI(TAG, "✅ %s turned OFF successfully", UI_LABEL_A);
  }
  else
  {
    ESP_LOGE(TAG, "❌ Failed to turn OFF %s: %s", UI_LABEL_A, esp_err_to_name(result));
  }

  return result;
}

esp_err_t smart_home_get_pump_status(bool *is_on)
{
  if (!smart_home_initialized || !is_on)
  {
    return ESP_ERR_INVALID_ARG;
  }

  ha_entity_state_t state;
  esp_err_t ret = ha_api_get_entity_state(HA_ENTITY_A, &state);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to get switch A status: %s", esp_err_to_name(ret));
    return ret;
  }

  *is_on = (strcmp(state.state, "on") == 0);
  return ESP_OK;
}

esp_err_t smart_home_get_temperature(float *temperature)
{
  if (!smart_home_initialized || !temperature)
  {
    return ESP_ERR_INVALID_ARG;
  }

  // TODO: Implement temperature sensor reading
  // For now, return a placeholder value
  *temperature = 25.5f;
  ESP_LOGW(TAG, "Temperature reading not implemented, returning placeholder");

  return ESP_OK;
}

esp_err_t smart_home_get_humidity(float *humidity)
{
  if (!smart_home_initialized || !humidity)
  {
    return ESP_ERR_INVALID_ARG;
  }

  // TODO: Implement humidity sensor reading
  // For now, return a placeholder value
  *humidity = 60.0f;
  ESP_LOGW(TAG, "Humidity reading not implemented, returning placeholder");

  return ESP_OK;
}

esp_err_t smart_home_get_device_status(const char *entity_id, smart_device_status_t *device_status)
{
  if (!smart_home_initialized || !entity_id || !device_status)
  {
    return ESP_ERR_INVALID_ARG;
  }

  ha_entity_state_t ha_state;
  esp_err_t ret = ha_api_get_entity_state(entity_id, &ha_state);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to get device status for %s: %s", entity_id, esp_err_to_name(ret));
    return ret;
  }

  // Convert HA state to smart device status
  memset(device_status, 0, sizeof(smart_device_status_t));
  strncpy(device_status->entity_id, entity_id, sizeof(device_status->entity_id) - 1);
  strncpy(device_status->state, ha_state.state, sizeof(device_status->state) - 1);
  device_status->available = true;
  device_status->last_updated = esp_timer_get_time() / 1000;

  // Set device type and friendly name based on entity
  if (strcmp(entity_id, HA_ENTITY_A) == 0)
  {
    device_status->type = SMART_DEVICE_SWITCH;
    strncpy(device_status->friendly_name, UI_LABEL_A, sizeof(device_status->friendly_name) - 1);
  }
  else if (strcmp(entity_id, HA_ENTITY_B) == 0)
  {
    device_status->type = SMART_DEVICE_SWITCH;
    strncpy(device_status->friendly_name, UI_LABEL_B, sizeof(device_status->friendly_name) - 1);
  }
  else if (strcmp(entity_id, HA_ENTITY_C) == 0)
  {
    device_status->type = SMART_DEVICE_SWITCH;
    strncpy(device_status->friendly_name, UI_LABEL_C, sizeof(device_status->friendly_name) - 1);
  }
  else
  {
    device_status->type = SMART_DEVICE_UNKNOWN;
    strncpy(device_status->friendly_name, "Unknown Device", sizeof(device_status->friendly_name) - 1);
  }

  return ESP_OK;
}

esp_err_t smart_home_get_all_devices(smart_device_status_t *devices, uint32_t max_devices, uint32_t *device_count)
{
  if (!smart_home_initialized || !devices || !device_count)
  {
    return ESP_ERR_INVALID_ARG;
  }

  const char *entity_ids[] = {HA_ENTITY_A, HA_ENTITY_B, HA_ENTITY_C};
  const uint32_t total_devices = sizeof(entity_ids) / sizeof(entity_ids[0]);

  uint32_t count = (max_devices < total_devices) ? max_devices : total_devices;

  for (uint32_t i = 0; i < count; i++)
  {
    esp_err_t ret = smart_home_get_device_status(entity_ids[i], &devices[i]);
    if (ret != ESP_OK)
    {
      ESP_LOGW(TAG, "Failed to get status for device %d: %s", i, esp_err_to_name(ret));
      // Continue with other devices
    }
  }

  *device_count = count;
  return ESP_OK;
}

esp_err_t smart_home_register_callback(smart_home_event_callback_t callback, void *user_data)
{
  if (!smart_home_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  event_callback = callback;
  callback_user_data = user_data;

  ESP_LOGI(TAG, "Event callback registered");
  return ESP_OK;
}

esp_err_t smart_home_unregister_callback(void)
{
  event_callback = NULL;
  callback_user_data = NULL;

  ESP_LOGI(TAG, "Event callback unregistered");
  return ESP_OK;
}

esp_err_t smart_home_refresh_all(void)
{
  if (!smart_home_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(TAG, "Refreshing all device statuses");

  // Use the ha_sync module for immediate synchronization
  return ha_sync_immediate_switches();
}

esp_err_t smart_home_test_connection(void)
{
  if (!smart_home_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  // Test connection by getting state of a known entity
  ha_entity_state_t test_state;
  esp_err_t ret = ha_api_get_entity_state(HA_ENTITY_A, &test_state);

  if (ret == ESP_OK)
  {
    ESP_LOGI(TAG, "Home Assistant connection test successful");
  }
  else
  {
    ESP_LOGE(TAG, "Home Assistant connection test failed: %s", esp_err_to_name(ret));
  }

  return ret;
}

esp_err_t smart_home_control_switch(const char *entity_id, bool turn_on)
{
  if (!smart_home_initialized || !entity_id)
  {
    ESP_LOGE(TAG, "Invalid parameters - initialized: %d, entity_id: %s",
             smart_home_initialized, entity_id ? entity_id : "NULL");
    return ESP_ERR_INVALID_ARG;
  }

  const char *action = turn_on ? "ON" : "OFF";
  const char *emoji = turn_on ? "🔵" : "🔴";

  ESP_LOGI(TAG, "%s SWITCH CONTROL: %s → %s", emoji, entity_id, action);

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
    ESP_LOGI(TAG, "✅ Switch %s turned %s successfully", entity_id, action);
  }
  else
  {
    ESP_LOGE(TAG, "❌ Failed to turn %s switch %s: %s", action, entity_id, esp_err_to_name(result));
  }

  return result;
}

esp_err_t smart_home_update_ui(void)
{
  if (!smart_home_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  // TODO: Implement UI update logic
  // This could trigger UI refresh with current device states
  ESP_LOGW(TAG, "UI update not implemented yet");

  return ESP_OK;
}

// ═══════════════════════════════════════════════════════════════════════════════
// CONVENIENCE FUNCTIONS FOR SPECIFIC DEVICES
// ═══════════════════════════════════════════════════════════════════════════════

esp_err_t smart_home_wave_maker_on(void)
{
  return smart_home_control_switch(HA_ENTITY_B, true);
}

esp_err_t smart_home_wave_maker_off(void)
{
  return smart_home_control_switch(HA_ENTITY_B, false);
}

esp_err_t smart_home_wave_maker_toggle(void)
{
  if (!smart_home_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(TAG, "Toggling switch B");
  return ha_api_toggle_switch(HA_ENTITY_B);
}

esp_err_t smart_home_light_on(void)
{
  return smart_home_control_switch(HA_ENTITY_C, true);
}

esp_err_t smart_home_light_off(void)
{
  return smart_home_control_switch(HA_ENTITY_C, false);
}

esp_err_t smart_home_light_toggle(void)
{
  if (!smart_home_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(TAG, "Toggling switch C");
  return ha_api_toggle_switch(HA_ENTITY_C);
}

esp_err_t smart_home_trigger_scene(void)
{
  if (!smart_home_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(TAG, "Triggering scene button");

  // For scene entities, we use the scene.turn_on service
  ha_service_call_t scene_call = {
      .domain = "scene",
      .service = "turn_on",
      .entity_id = HA_ENTITY_D};

  ha_api_response_t response;
  esp_err_t ret = ha_api_call_service(&scene_call, &response);

  return ret;
}
