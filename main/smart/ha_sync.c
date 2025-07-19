/**
 * @file ha_sync.c
 * @brief Home Assistant Device State Synchronization Implementation
 *
 * This module provides the implementation for synchronizing device states
 * with Home Assistant
 */

#include "ha_sync.h"
#include "ha_api.h"
#include "ui_dashboard.h"
#include "ui_controls_panel.h"
#include "../utils/system_debug_utils.h"
#include <esp_timer.h>
#include <string.h>

// ══════════════════════════════════════════════════════════════════════════════�?
// GLOBAL SYNC STATE
// ══════════════════════════════════════════════════════════════════════════════�?

static ha_device_sync_t switch_a_sync = {
    .entity_id = HA_ENTITY_A_ID,
    .friendly_name = "Switch A",
    .local_state = HA_DEVICE_STATE_UNKNOWN,
    .remote_state = HA_DEVICE_STATE_UNKNOWN,
    .sync_status = HA_SYNC_STATUS_UNKNOWN,
    .last_sync_time = 0,
    .last_check_time = 0,
    .failed_attempts = 0,
    .is_enabled = true};

static bool sync_system_initialized = false;

// ══════════════════════════════════════════════════════════════════════════════�?
// INTERNAL HELPER FUNCTIONS
// ══════════════════════════════════════════════════════════════════════════════�?

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

// ══════════════════════════════════════════════════════════════════════════════�?
// FORWARD DECLARATIONS
// ══════════════════════════════════════════════════════════════════════════════�?

static bool ha_sync_switch_a_get_remote_state(ha_device_state_t *state);

// ══════════════════════════════════════════════════════════════════════════════�?
// SWITCH A SYNC IMPLEMENTATION
// ══════════════════════════════════════════════════════════════════════════════�?

bool ha_sync_switch_a_init(void)
{
  debug_log_info_f(DEBUG_TAG_HA_SYNC, "Initializing Switch A sync for entity: %s", switch_a_sync.entity_id);

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
    debug_log_info_f(DEBUG_TAG_HA_SYNC, "switch_a initialized with state: %s", ha_device_state_to_string(initial_state));
  }
  else
  {
    debug_log_warning(DEBUG_TAG_HA_SYNC, "Failed to get initial switch_a state - will retry later");
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
    debug_log_warning_f(DEBUG_TAG_HA_SYNC, "Failed to get switch_a state (attempt %d/%d)",
                        switch_a_sync.failed_attempts, HA_SYNC_RETRY_COUNT);

    if (switch_a_sync.failed_attempts >= HA_SYNC_RETRY_COUNT)
    {
      switch_a_sync.sync_status = HA_SYNC_STATUS_DISABLED;
      switch_a_sync.is_enabled = false;
      debug_log_error(DEBUG_TAG_HA_SYNC, "switch_a disabled due to sync failures");
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
    debug_log_debug_f(DEBUG_TAG_HA_SYNC, "switch_a sync OK: %s", ha_device_state_to_string(remote_state));
  }
  else
  {
    switch_a_sync.sync_status = HA_SYNC_STATUS_OUT_OF_SYNC;
    debug_log_warning_f(DEBUG_TAG_HA_SYNC, "switch_a out of sync: local=%s, remote=%s",
                        ha_device_state_to_string(switch_a_sync.local_state),
                        ha_device_state_to_string(switch_a_sync.remote_state));
  }

  return switch_a_sync.sync_status;
}

static bool ha_sync_switch_a_get_remote_state(ha_device_state_t *state)
{
  // This function should be implemented to make HTTP request to HA API
  // For now, this is a stub that should be replaced with actual HTTP client code

  debug_log_debug_f(DEBUG_TAG_HA_SYNC, "Getting remote state for: %s", switch_a_sync.entity_id);

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
    debug_log_error_f(DEBUG_TAG_HA_SYNC, "Cannot set switch_a to invalid state: %s", ha_device_state_to_string(state));
    return false;
  }

  debug_log_info_f(DEBUG_TAG_HA_SYNC, "Setting switch_a local state: %s", ha_device_state_to_string(state));
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
    debug_log_warning(DEBUG_TAG_HA_SYNC, "Cannot sync disabled switch_a");
    return false;
  }

  debug_log_info_f(DEBUG_TAG_HA_SYNC, "Synchronizing switch_a: local=%s",
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
    debug_log_info_f(DEBUG_TAG_HA_SYNC, "switch_a %s", enabled ? "ENABLED" : "DISABLED");
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

// ══════════════════════════════════════════════════════════════════════════════�?
// GENERAL SYNC FUNCTIONS
// ══════════════════════════════════════════════════════════════════════════════�?

bool ha_sync_init(void)
{
  if (sync_system_initialized)
  {
    debug_log_warning(DEBUG_TAG_HA_SYNC, "Sync system already initialized");
    return true;
  }

  debug_log_info(DEBUG_TAG_HA_SYNC, "Initializing Home Assistant sync system");

  // Initialize switch_a sync
  if (!ha_sync_switch_a_init())
  {
    debug_log_error(DEBUG_TAG_HA_SYNC, "Failed to initialize switch_a sync");
    return false;
  }

  sync_system_initialized = true;
  debug_log_info(DEBUG_TAG_HA_SYNC, "Sync system initialized successfully");
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

    // Update the UI with switch states
    controls_panel_set_switch(SWITCH_A, switch_a_on);
    controls_panel_set_switch(SWITCH_B, switch_b_on);
    controls_panel_set_switch(SWITCH_C, switch_c_on);

    debug_log_info_f(DEBUG_TAG_HA_SYNC, "Immediate sync completed: %s=%s, %s=%s, %s=%s",
                     "Water Pump", switch_states[0].state,
                     "Wave Maker", switch_states[1].state,
                     "Light Switch", switch_states[2].state);

    return ESP_OK;
  }
  else
  {
    debug_log_warning_f(DEBUG_TAG_HA_SYNC, "Immediate sync failed: %s", esp_err_to_name(ret));
    return ret;
  }
}
