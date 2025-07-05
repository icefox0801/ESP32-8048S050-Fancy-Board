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
// WATER PUMP SYNC FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Initialize water pump synchronization
 * @return true if initialization successful, false otherwise
 */
bool ha_sync_pump_init(void);

/**
 * @brief Check if water pump is in sync with Home Assistant
 * @return Current sync status
 */
ha_sync_status_t ha_sync_pump_check_status(void);

/**
 * @brief Get current water pump state from Home Assistant
 * @param state Pointer to store the retrieved state
 * @return true if state retrieved successfully, false otherwise
 */
bool ha_sync_pump_get_remote_state(ha_device_state_t *state);

/**
 * @brief Set local water pump state expectation
 * @param state The expected state (ON/OFF)
 * @return true if state set successfully, false otherwise
 */
bool ha_sync_pump_set_local_state(ha_device_state_t state);

/**
 * @brief Synchronize water pump state with Home Assistant
 * If sync fails multiple times, the device will be disabled
 * @return true if sync successful, false otherwise
 */
bool ha_sync_pump_synchronize(void);

/**
 * @brief Check if water pump is enabled for control
 * @return true if enabled, false if disabled due to sync issues
 */
bool ha_sync_pump_is_enabled(void);

/**
 * @brief Force enable/disable water pump control
 * @param enabled true to enable, false to disable
 */
void ha_sync_pump_set_enabled(bool enabled);

/**
 * @brief Get detailed sync information for water pump
 * @return Pointer to sync structure (read-only)
 */
const ha_device_sync_t *ha_sync_pump_get_info(void);

// ═══════════════════════════════════════════════════════════════════════════════
// GENERAL SYNC FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Initialize the entire sync system
 * @return true if initialization successful, false otherwise
 */
bool ha_sync_init(void);

/**
 * @brief Periodic sync task - call this regularly from main loop
 * This will check all monitored devices and update their sync status
 */
void ha_sync_task(void);

/**
 * @brief Get sync status string for display
 * @param status The sync status enum
 * @return Human-readable status string
 */
const char *ha_sync_status_to_string(ha_sync_status_t status);

/**
 * @brief Convert device state enum to string
 * @param state The device state enum
 * @return Human-readable state string
 */
const char *ha_device_state_to_string(ha_device_state_t state);

#endif // HA_SYNC_H
