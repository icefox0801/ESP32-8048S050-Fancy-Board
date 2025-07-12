/**
 * @file ha_sync.c
 * @brief Home Assistant Device State Synchronization Implementation
 *
 * This module provides the implementation for synchronizing device states
 * with Home Assistant bool ha_sync_switch_a_sync(void)
{
  if (!switch_a_sync.is_enabled)
  {
    ESP_LOGW(TAG, "Cannot sync disabled switch_a");
    return false;
  }

  ESP_LOGI(TAG, "Synchronizing switch_a: local=%s",
           ha_device_state_to_string(switch_a_sync.local_state));

  // Prepare service call structure
  ha_service_call_t service_call = {0};
  strcpy(service_call.domain, "switch");
  strcpy(service_call.entity_id, switch_a_sync.entity_id);

  if (switch_a_sync.local_state == HA_DEVICE_STATE_ON)
  {
    strcpy(service_call.service, "turn_on");
  }
  else if (switch_a_sync.local_state == HA_DEVICE_STATE_OFF)
  {
    strcpy(service_call.service, "turn_off");
  }
  else
  {
    ESP_LOGW(TAG, "Cannot sync switch_a with unknown state");
    return false;
  }

  // Make service call to Home Assistant
  ha_api_response_t response;
  esp_err_t ret = ha_api_call_service(&service_call, &response);

  if (ret == ESP_OK && response.success)
  {
    ESP_LOGI(TAG, "Successfully sent switch_a command to Home Assistant");

    // Wait a moment for the device to respond
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Verify the state change took effect
    ha_device_state_t new_remote_state;
    if (ha_sync_switch_a_get_remote_state(&new_remote_state))
    {
      switch_a_sync.remote_state = new_remote_state;
      switch_a_sync.last_sync_time = get_timestamp_ms();

      if (switch_a_sync.local_state == new_remote_state)
      {
        switch_a_sync.sync_status = HA_SYNC_STATUS_SYNCED;
        switch_a_sync.failed_attempts = 0;
        ESP_LOGI(TAG, "switch_a sync successful: %s", ha_device_state_to_string(new_remote_state));

        // Clean up response data
        if (response.response_data) {
          free(response.response_data);
        }
        return true;
      }
      else
      {
        switch_a_sync.sync_status = HA_SYNC_STATUS_OUT_OF_SYNC;
        ESP_LOGW(TAG, "switch_a sync failed - state mismatch: local=%s, remote=%s",
                 ha_device_state_to_string(switch_a_sync.local_state),
                 ha_device_state_to_string(new_remote_state));
      }
    }
  }
  else
  {
    ESP_LOGE(TAG, "Failed to send switch_a command: %s", response.error_message);
    switch_a_sync.failed_attempts++;
    switch_a_sync.sync_status = HA_SYNC_STATUS_FAILED;
  }

  // Clean up response data
  if (response.response_data) {
    free(response.response_data);
  }

  return false;failures.
 */

#include "ha_sync.h"
#include "ha_api.h"
#include "../lvgl/system_monitor_ui.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <string.h>

static const char *TAG = "HA_SYNC";

// ═══════════════════════════════════════════════════════════════════════════════
// GLOBAL SYNC STATE
// ═══════════════════════════════════════════════════════════════════════════════

static ha_device_sync_t switch_a_sync = {
    .entity_id = HA_ENTITY_A,
    .friendly_name = "Switch A",
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

static ha_device_state_t __attribute__((unused)) parse_ha_state(const char *state_str)
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
// FORWARD DECLARATIONS
// ═══════════════════════════════════════════════════════════════════════════════

static bool ha_sync_switch_a_get_remote_state(ha_device_state_t *state);

// ═══════════════════════════════════════════════════════════════════════════════
// SWITCH A SYNC IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════════

bool ha_sync_switch_a_init(void)
{
  ESP_LOGI(TAG, "Initializing Switch A sync for entity: %s", switch_a_sync.entity_id);

  switch_a_sync.local_state = HA_DEVICE_STATE_UNKNOWN;
  switch_a_sync.remote_state = HA_DEVICE_STATE_UNKNOWN;
  switch_a_sync.sync_status = HA_SYNC_STATUS_UNKNOWN;
  switch_a_sync.last_sync_time = 0;
  switch_a_sync.last_check_time = 0;
  switch_a_sync.failed_attempts = 0;
  switch_a_sync.is_enabled = true;

  // Perform initial state check
  ha_device_state_t initial_state;
  if (ha_sync_switch_a_get_remote_state(&initial_state))
  {
    switch_a_sync.remote_state = initial_state;
    switch_a_sync.local_state = initial_state; // Assume we start in sync
    switch_a_sync.sync_status = HA_SYNC_STATUS_SYNCED;
    switch_a_sync.last_sync_time = get_timestamp_ms();
    ESP_LOGI(TAG, "switch_a initialized with state: %s", ha_device_state_to_string(initial_state));
  }
  else
  {
    ESP_LOGW(TAG, "Failed to get initial switch_a state - will retry later");
    switch_a_sync.sync_status = HA_SYNC_STATUS_FAILED;
    switch_a_sync.failed_attempts = 1;
  }

  return true;
}

ha_sync_status_t ha_sync_switch_a_check_status(void)
{
  uint32_t now = get_timestamp_ms();

  // Don't check too frequently
  if (now - switch_a_sync.last_check_time < HA_SYNC_CHECK_INTERVAL_MS)
  {
    return switch_a_sync.sync_status;
  }

  switch_a_sync.last_check_time = now;

  // If device is disabled, don't try to sync
  if (!switch_a_sync.is_enabled)
  {
    return HA_SYNC_STATUS_DISABLED;
  }

  // Get current remote state
  ha_device_state_t remote_state;
  if (!ha_sync_switch_a_get_remote_state(&remote_state))
  {
    switch_a_sync.failed_attempts++;
    ESP_LOGW(TAG, "Failed to get switch_a state (attempt %d/%d)",
             switch_a_sync.failed_attempts, HA_SYNC_RETRY_COUNT);

    if (switch_a_sync.failed_attempts >= HA_SYNC_RETRY_COUNT)
    {
      switch_a_sync.sync_status = HA_SYNC_STATUS_DISABLED;
      switch_a_sync.is_enabled = false;
      ESP_LOGE(TAG, "switch_a disabled due to sync failures");
    }
    else
    {
      switch_a_sync.sync_status = HA_SYNC_STATUS_FAILED;
    }

    return switch_a_sync.sync_status;
  }

  // Update remote state and reset failure count
  switch_a_sync.remote_state = remote_state;
  switch_a_sync.failed_attempts = 0;

  // Check if states match
  if (switch_a_sync.local_state == switch_a_sync.remote_state)
  {
    switch_a_sync.sync_status = HA_SYNC_STATUS_SYNCED;
    switch_a_sync.last_sync_time = now;
    ESP_LOGD(TAG, "switch_a sync OK: %s", ha_device_state_to_string(remote_state));
  }
  else
  {
    switch_a_sync.sync_status = HA_SYNC_STATUS_OUT_OF_SYNC;
    ESP_LOGW(TAG, "switch_a out of sync: local=%s, remote=%s",
             ha_device_state_to_string(switch_a_sync.local_state),
             ha_device_state_to_string(switch_a_sync.remote_state));
  }

  return switch_a_sync.sync_status;
}

static bool ha_sync_switch_a_get_remote_state(ha_device_state_t *state)
{
  // This function should be implemented to make HTTP request to HA API
  // For now, this is a stub that should be replaced with actual HTTP client code

  ESP_LOGD(TAG, "Getting remote state for: %s", switch_a_sync.entity_id);

  // TODO: Implement actual HTTP request to:
  // GET http://192.168.50.193:8123/api/states/switch.pai_cha_4_1_zigbee_socket_4
  // Parse JSON response and extract state field

  // Stub implementation - replace with actual HTTP client
  *state = HA_DEVICE_STATE_UNKNOWN;
  return false; // Return true when implemented
}

bool ha_sync_switch_a_set_local_state(ha_device_state_t state)
{
  if (state == HA_DEVICE_STATE_UNKNOWN || state == HA_DEVICE_STATE_UNAVAILABLE)
  {
    ESP_LOGE(TAG, "Cannot set switch_a to invalid state: %s", ha_device_state_to_string(state));
    return false;
  }

  ESP_LOGI(TAG, "Setting switch_a local state: %s", ha_device_state_to_string(state));
  switch_a_sync.local_state = state;

  // Mark as potentially out of sync until next check
  if (switch_a_sync.sync_status == HA_SYNC_STATUS_SYNCED)
  {
    switch_a_sync.sync_status = HA_SYNC_STATUS_UNKNOWN;
  }

  return true;
}

bool ha_sync_switch_a_synchronize(void)
{
  if (!switch_a_sync.is_enabled)
  {
    ESP_LOGW(TAG, "Cannot sync disabled switch_a");
    return false;
  }

  ESP_LOGI(TAG, "Synchronizing switch_a: local=%s",
           ha_device_state_to_string(switch_a_sync.local_state));

  // TODO: Implement actual HTTP request to set switch_a state
  // POST http://192.168.50.193:8123/api/services/switch/turn_on
  // or POST http://192.168.50.193:8123/api/services/switch/turn_off
  // with body: {"entity_id": "switch.pai_cha_4_1_zigbee_socket_4"}

  // After successful HTTP request, verify state
  return ha_sync_switch_a_check_status() == HA_SYNC_STATUS_SYNCED;
}

bool ha_sync_switch_a_is_enabled(void)
{
  return switch_a_sync.is_enabled;
}

void ha_sync_switch_a_set_enabled(bool enabled)
{
  if (enabled != switch_a_sync.is_enabled)
  {
    ESP_LOGI(TAG, "switch_a %s", enabled ? "ENABLED" : "DISABLED");
    switch_a_sync.is_enabled = enabled;

    if (enabled)
    {
      // Reset failure count when manually re-enabled
      switch_a_sync.failed_attempts = 0;
      switch_a_sync.sync_status = HA_SYNC_STATUS_UNKNOWN;
    }
    else
    {
      switch_a_sync.sync_status = HA_SYNC_STATUS_DISABLED;
    }
  }
}

const ha_device_sync_t *ha_sync_switch_a_get_info(void)
{
  return &switch_a_sync;
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

  // Initialize switch_a sync
  if (!ha_sync_switch_a_init())
  {
    ESP_LOGE(TAG, "Failed to initialize switch_a sync");
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

  // Check switch_a sync status
  ha_sync_switch_a_check_status();

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
      HA_ENTITY_A, // Switch A
      HA_ENTITY_B, // Switch B
      HA_ENTITY_C  // Switch C
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

    // Update the UI with switch states
    system_monitor_ui_set_switch_a(switch_a_on);
    system_monitor_ui_set_switch_b(switch_b_on);
    system_monitor_ui_set_switch_c(switch_c_on);

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
