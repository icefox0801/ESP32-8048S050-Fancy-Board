/**
 * @file ha_sync.h
 * @brief Home Assistant Device State Synchronization
 *
 * This module handles synchronizing local device states with Home Assistant
 * and disabling devices that fail to sync properly.
 */

#ifndef HA_SYNC_H
#define HA_SYNC_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "smart_config.h"

// ═══════════════════════════════════════════════════════════════════════════════
// SYNC STATUS DEFINITIONS
// ═══════════════════════════════════════════════════════════════════════════════

typedef enum
{
  HA_SYNC_STATUS_UNKNOWN = 0,
  HA_SYNC_STATUS_SYNCED,
  HA_SYNC_STATUS_OUT_OF_SYNC,
  HA_SYNC_STATUS_FAILED,
  HA_SYNC_STATUS_DISABLED
} ha_sync_status_t;

typedef enum
{
  HA_DEVICE_STATE_UNKNOWN = 0,
  HA_DEVICE_STATE_ON,
  HA_DEVICE_STATE_OFF,
  HA_DEVICE_STATE_UNAVAILABLE
} ha_device_state_t;

// ═══════════════════════════════════════════════════════════════════════════════
// DEVICE SYNC STRUCTURE
// ═══════════════════════════════════════════════════════════════════════════════

typedef struct
{
  const char *entity_id;          // Home Assistant entity ID
  const char *friendly_name;      // Display name for UI
  ha_device_state_t local_state;  // Local device state (what we think it should be)
  ha_device_state_t remote_state; // Remote HA state (what HA reports)
  ha_sync_status_t sync_status;   // Current sync status
  uint32_t last_sync_time;        // Last successful sync timestamp
  uint32_t last_check_time;       // Last time we checked the state
  uint8_t failed_attempts;        // Number of consecutive failed sync attempts
  bool is_enabled;                // Whether device is enabled for control
} ha_device_sync_t;

// ═══════════════════════════════════════════════════════════════════════════════
// GENERAL SYNC FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Immediately sync all switch states from Home Assistant using bulk API
 * This function is called when WiFi connects to get immediate state updates
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ha_sync_immediate_switches(void);

#endif // HA_SYNC_H
