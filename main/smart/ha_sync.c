/**
 * @file ha_sync.c
 * @brief Home Assistant Device State Synchronization Implementation
 *
 * This module provides the implementation for synchronizing device states
 * with Home Assistant bool ha_sync_pump_sync(void)
{
  if (!pump_sync.is_enabled)
  {
    ESP_LOGW(TAG, "Cannot sync disabled pump");
    return false;
  }

  ESP_LOGI(TAG, "Synchronizing pump: local=%s",
           ha_device_state_to_string(pump_sync.local_state));

  // Prepare service call structure
  ha_service_call_t service_call = {0};
  strcpy(service_call.domain, "switch");
  strcpy(service_call.entity_id, pump_sync.entity_id);

  if (pump_sync.local_state == HA_DEVICE_STATE_ON)
  {
    strcpy(service_call.service, "turn_on");
  }
  else if (pump_sync.local_state == HA_DEVICE_STATE_OFF)
  {
    strcpy(service_call.service, "turn_off");
  }
  else
  {
    ESP_LOGW(TAG, "Cannot sync pump with unknown state");
    return false;
  }

  // Make service call to Home Assistant
  ha_api_response_t response;
  esp_err_t ret = ha_api_call_service(&service_call, &response);

  if (ret == ESP_OK && response.success)
  {
    ESP_LOGI(TAG, "Successfully sent pump command to Home Assistant");

    // Wait a moment for the device to respond
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Verify the state change took effect
    ha_device_state_t new_remote_state;
    if (ha_sync_pump_get_remote_state(&new_remote_state))
    {
      pump_sync.remote_state = new_remote_state;
      pump_sync.last_sync_time = get_timestamp_ms();

      if (pump_sync.local_state == new_remote_state)
      {
        pump_sync.sync_status = HA_SYNC_STATUS_SYNCED;
        pump_sync.failed_attempts = 0;
        ESP_LOGI(TAG, "Pump sync successful: %s", ha_device_state_to_string(new_remote_state));

        // Clean up response data
        if (response.response_data) {
          free(response.response_data);
        }
        return true;
      }
      else
      {
        pump_sync.sync_status = HA_SYNC_STATUS_OUT_OF_SYNC;
        ESP_LOGW(TAG, "Pump sync failed - state mismatch: local=%s, remote=%s",
                 ha_device_state_to_string(pump_sync.local_state),
                 ha_device_state_to_string(new_remote_state));
      }
    }
  }
  else
  {
    ESP_LOGE(TAG, "Failed to send pump command: %s", response.error_message);
    pump_sync.failed_attempts++;
    pump_sync.sync_status = HA_SYNC_STATUS_FAILED;
  }

  // Clean up response data
  if (response.response_data) {
    free(response.response_data);
  }

  return false;failures.
 */

#include "ha_sync.h"
#include "ha_api.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <string.h>

static const char *TAG = "HA_SYNC";

// ═══════════════════════════════════════════════════════════════════════════════
// GLOBAL SYNC STATE
// ═══════════════════════════════════════════════════════════════════════════════

static ha_device_sync_t pump_sync = {
    .entity_id = HA_ENTITY_A,
    .friendly_name = "Water Pump",
    .local_state = HA_DEVICE_STATE_UNKNOWN,
    .remote_state = HA_DEVICE_STATE_UNKNOWN,
    .sync_status = HA_SYNC_STATUS_UNKNOWN,
    .last_sync_time = 0,
    .last_check_time = 0,
    .failed_attempts = 0,
    .is_enabled = true};

static bool sync_system_initialized = false;

// ═══════════════════════════════════════════════════════════════════════════════
// INTERNAL HELPER FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════

static uint32_t get_timestamp_ms(void)
{
  return (uint32_t)(esp_timer_get_time() / 1000);
}

static __attribute__((unused)) ha_device_state_t parse_ha_state(const char *state_str)
{
  if (state_str == NULL)
  {
    return HA_DEVICE_STATE_UNKNOWN;
  }

  if (strcmp(state_str, "on") == 0)
  {
    return HA_DEVICE_STATE_ON;
  }
  else if (strcmp(state_str, "off") == 0)
  {
    return HA_DEVICE_STATE_OFF;
  }
  else if (strcmp(state_str, "unavailable") == 0)
  {
    return HA_DEVICE_STATE_UNAVAILABLE;
  }

  return HA_DEVICE_STATE_UNKNOWN;
}

// ═══════════════════════════════════════════════════════════════════════════════
// WATER PUMP SYNC IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════════

bool ha_sync_pump_init(void)
{
  ESP_LOGI(TAG, "Initializing water pump sync for entity: %s", pump_sync.entity_id);

  pump_sync.local_state = HA_DEVICE_STATE_UNKNOWN;
  pump_sync.remote_state = HA_DEVICE_STATE_UNKNOWN;
  pump_sync.sync_status = HA_SYNC_STATUS_UNKNOWN;
  pump_sync.last_sync_time = 0;
  pump_sync.last_check_time = 0;
  pump_sync.failed_attempts = 0;
  pump_sync.is_enabled = true;

  // Perform initial state check
  ha_device_state_t initial_state;
  if (ha_sync_pump_get_remote_state(&initial_state))
  {
    pump_sync.remote_state = initial_state;
    pump_sync.local_state = initial_state; // Assume we start in sync
    pump_sync.sync_status = HA_SYNC_STATUS_SYNCED;
    pump_sync.last_sync_time = get_timestamp_ms();
    ESP_LOGI(TAG, "Pump initialized with state: %s", ha_device_state_to_string(initial_state));
  }
  else
  {
    ESP_LOGW(TAG, "Failed to get initial pump state - will retry later");
    pump_sync.sync_status = HA_SYNC_STATUS_FAILED;
    pump_sync.failed_attempts = 1;
  }

  return true;
}

ha_sync_status_t ha_sync_pump_check_status(void)
{
  uint32_t now = get_timestamp_ms();

  // Don't check too frequently
  if (now - pump_sync.last_check_time < HA_SYNC_CHECK_INTERVAL_MS)
  {
    return pump_sync.sync_status;
  }

  pump_sync.last_check_time = now;

  // If device is disabled, don't try to sync
  if (!pump_sync.is_enabled)
  {
    return HA_SYNC_STATUS_DISABLED;
  }

  // Get current remote state
  ha_device_state_t remote_state;
  if (!ha_sync_pump_get_remote_state(&remote_state))
  {
    pump_sync.failed_attempts++;
    ESP_LOGW(TAG, "Failed to get pump state (attempt %d/%d)",
             pump_sync.failed_attempts, HA_SYNC_RETRY_COUNT);

    if (pump_sync.failed_attempts >= HA_SYNC_RETRY_COUNT)
    {
      pump_sync.sync_status = HA_SYNC_STATUS_DISABLED;
      pump_sync.is_enabled = false;
      ESP_LOGE(TAG, "Pump disabled due to sync failures");
    }
    else
    {
      pump_sync.sync_status = HA_SYNC_STATUS_FAILED;
    }

    return pump_sync.sync_status;
  }

  // Update remote state and reset failure count
  pump_sync.remote_state = remote_state;
  pump_sync.failed_attempts = 0;

  // Check if states match
  if (pump_sync.local_state == pump_sync.remote_state)
  {
    pump_sync.sync_status = HA_SYNC_STATUS_SYNCED;
    pump_sync.last_sync_time = now;
    ESP_LOGD(TAG, "Pump sync OK: %s", ha_device_state_to_string(remote_state));
  }
  else
  {
    pump_sync.sync_status = HA_SYNC_STATUS_OUT_OF_SYNC;
    ESP_LOGW(TAG, "Pump out of sync: local=%s, remote=%s",
             ha_device_state_to_string(pump_sync.local_state),
             ha_device_state_to_string(pump_sync.remote_state));
  }

  return pump_sync.sync_status;
}

bool ha_sync_pump_get_remote_state(ha_device_state_t *state)
{
  // This function should be implemented to make HTTP request to HA API
  // For now, this is a stub that should be replaced with actual HTTP client code

  ESP_LOGD(TAG, "Getting remote state for: %s", pump_sync.entity_id);

  // TODO: Implement actual HTTP request to:
  // GET http://192.168.50.193:8123/api/states/switch.pai_cha_4_1_zigbee_socket_4
  // Parse JSON response and extract state field

  // Stub implementation - replace with actual HTTP client
  *state = HA_DEVICE_STATE_UNKNOWN;
  return false; // Return true when implemented
}

bool ha_sync_pump_set_local_state(ha_device_state_t state)
{
  if (state == HA_DEVICE_STATE_UNKNOWN || state == HA_DEVICE_STATE_UNAVAILABLE)
  {
    ESP_LOGE(TAG, "Cannot set pump to invalid state: %s", ha_device_state_to_string(state));
    return false;
  }

  ESP_LOGI(TAG, "Setting pump local state: %s", ha_device_state_to_string(state));
  pump_sync.local_state = state;

  // Mark as potentially out of sync until next check
  if (pump_sync.sync_status == HA_SYNC_STATUS_SYNCED)
  {
    pump_sync.sync_status = HA_SYNC_STATUS_UNKNOWN;
  }

  return true;
}

bool ha_sync_pump_synchronize(void)
{
  if (!pump_sync.is_enabled)
  {
    ESP_LOGW(TAG, "Cannot sync disabled pump");
    return false;
  }

  ESP_LOGI(TAG, "Synchronizing pump: local=%s",
           ha_device_state_to_string(pump_sync.local_state));

  // TODO: Implement actual HTTP request to set pump state
  // POST http://192.168.50.193:8123/api/services/switch/turn_on
  // or POST http://192.168.50.193:8123/api/services/switch/turn_off
  // with body: {"entity_id": "switch.pai_cha_4_1_zigbee_socket_4"}

  // After successful HTTP request, verify state
  return ha_sync_pump_check_status() == HA_SYNC_STATUS_SYNCED;
}

bool ha_sync_pump_is_enabled(void)
{
  return pump_sync.is_enabled;
}

void ha_sync_pump_set_enabled(bool enabled)
{
  if (enabled != pump_sync.is_enabled)
  {
    ESP_LOGI(TAG, "Pump %s", enabled ? "ENABLED" : "DISABLED");
    pump_sync.is_enabled = enabled;

    if (enabled)
    {
      // Reset failure count when manually re-enabled
      pump_sync.failed_attempts = 0;
      pump_sync.sync_status = HA_SYNC_STATUS_UNKNOWN;
    }
    else
    {
      pump_sync.sync_status = HA_SYNC_STATUS_DISABLED;
    }
  }
}

const ha_device_sync_t *ha_sync_pump_get_info(void)
{
  return &pump_sync;
}

// ═══════════════════════════════════════════════════════════════════════════════
// GENERAL SYNC FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════

bool ha_sync_init(void)
{
  if (sync_system_initialized)
  {
    ESP_LOGW(TAG, "Sync system already initialized");
    return true;
  }

  ESP_LOGI(TAG, "Initializing Home Assistant sync system");

  // Initialize pump sync
  if (!ha_sync_pump_init())
  {
    ESP_LOGE(TAG, "Failed to initialize pump sync");
    return false;
  }

  sync_system_initialized = true;
  ESP_LOGI(TAG, "Sync system initialized successfully");
  return true;
}

void ha_sync_task(void)
{
  if (!sync_system_initialized)
  {
    return;
  }

  // Check pump sync status
  ha_sync_pump_check_status();

  // TODO: Add other devices here as needed
}

const char *ha_sync_status_to_string(ha_sync_status_t status)
{
  switch (status)
  {
  case HA_SYNC_STATUS_UNKNOWN:
    return "UNKNOWN";
  case HA_SYNC_STATUS_SYNCED:
    return "SYNCED";
  case HA_SYNC_STATUS_OUT_OF_SYNC:
    return "OUT_OF_SYNC";
  case HA_SYNC_STATUS_FAILED:
    return "FAILED";
  case HA_SYNC_STATUS_DISABLED:
    return "DISABLED";
  default:
    return "INVALID";
  }
}

const char *ha_device_state_to_string(ha_device_state_t state)
{
  switch (state)
  {
  case HA_DEVICE_STATE_UNKNOWN:
    return "UNKNOWN";
  case HA_DEVICE_STATE_ON:
    return "ON";
  case HA_DEVICE_STATE_OFF:
    return "OFF";
  case HA_DEVICE_STATE_UNAVAILABLE:
    return "UNAVAILABLE";
  default:
    return "INVALID";
  }
}

esp_err_t ha_sync_immediate_switches(void)
{
  ESP_LOGI(TAG, "Performing immediate switch sync using bulk API");

  // Entity IDs from smart config
  const char *switch_entity_ids[] = {
      HA_ENTITY_A, // Water pump
      HA_ENTITY_B, // Wave maker
      HA_ENTITY_C  // Light switch
  };
  const int switch_count = sizeof(switch_entity_ids) / sizeof(switch_entity_ids[0]);

  // Fetch all switch states in one bulk request
  ha_entity_state_t switch_states[switch_count];
  esp_err_t ret = ha_api_get_multiple_entity_states(switch_entity_ids, switch_count, switch_states);

  if (ret == ESP_OK)
  {
    // Update UI with switch states
    bool pump_on = (strcmp(switch_states[0].state, "on") == 0);
    bool wave_on = (strcmp(switch_states[1].state, "on") == 0);
    bool light_on = (strcmp(switch_states[2].state, "on") == 0);

    // Update the UI (we need to include the UI header)
    extern void system_monitor_ui_set_water_pump(bool on);
    extern void system_monitor_ui_set_wave_maker(bool on);
    extern void system_monitor_ui_set_light(bool on);

    system_monitor_ui_set_water_pump(pump_on);
    system_monitor_ui_set_wave_maker(wave_on);
    system_monitor_ui_set_light(light_on);

    ESP_LOGI(TAG, "Immediate sync completed: %s=%s, %s=%s, %s=%s",
             UI_LABEL_A, switch_states[0].state,
             UI_LABEL_B, switch_states[1].state,
             UI_LABEL_C, switch_states[2].state);

    return ESP_OK;
  }
  else
  {
    ESP_LOGW(TAG, "Immediate sync failed: %s", esp_err_to_name(ret));
    return ret;
  }
}
