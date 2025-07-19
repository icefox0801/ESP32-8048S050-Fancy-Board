/**
 * @file smart_home.h
 * @brief Smart Home Integration Manager
 *
 * This module provides a high-level interface for smart home automation
 * including Home Assistant integration, device control, and sensor monitoring.
 *
 * Features:
 * - Simplified device control interface
 * - Periodic switch state synchronization (30s intervals)
 * - Smart home status monitoring
 * - Integration with system monitor UI
 * - WiFi connection status handling
 *
 * @author System Monitor Dashboard
 * @date 2025-08-15
 */

#ifndef SMART_HOME_H
#define SMART_HOME_H

#include "ha_api.h"
#include <esp_err.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif
  /**
   * @brief Initialize smart home integration
   *
   * Sets up Home Assistant connection and starts periodic sync tasks.
   * Creates a background task that syncs switch states every 30 seconds.
   *
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t smart_home_init(void);

  /**
   * @brief Deinitialize smart home integration
   *
   * Stops periodic sync tasks and cleans up resources.
   *
   * @return ESP_OK on success
   */
  esp_err_t smart_home_deinit(void);

  // Functions removed - were declared but not implemented or used

  /**
   * @brief Control any switch entity
   *
   * Generic function to control any switch-type entity.
   *
   * @param entity_id Entity ID of the switch
   * @param turn_on True to turn on, false to turn off
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t smart_home_control_switch(const char *entity_id, bool turn_on);

  /**
   * @brief Trigger the scene
   *
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t smart_home_trigger_scene(void);

  /**
   * @brief Sync switch states with Home Assistant
   *
   * Immediately fetches current switch states from HA and updates the UI.
   * This function is called both manually and by the periodic sync task.
   *
   * @note This function performs network operations and may block briefly
   */
  void smart_home_sync_switch_states(void);

  /**
   * @brief Update WiFi connection status
   *
   * Handles WiFi connection state changes by starting/stopping HA tasks.
   * When connected, starts the HA task manager and requests initialization.
   * When disconnected, stops the HA task manager.
   *
   * @param is_connected True if WiFi is connected, false if disconnected
   */
  void smart_home_update_wifi_status(bool is_connected);

  /**
   * @brief Smart home status callback function type
   * @param connected True if smart home is connected and operational, false otherwise
   * @param status_text Human-readable status message describing current state
   */
  typedef void (*smart_home_status_callback_t)(bool connected, const char *status_text);

  /**
   * @brief Smart home states sync callback function type
   * @param switch_states Array of switch states (A, B, C)
   * @param state_count Number of switch states in the array
   */
  typedef void (*smart_home_states_sync_callback_t)(bool switch_states[3], int state_count);

  /**
   * @brief Register a callback for smart home status updates
   *
   * The callback will be called whenever the smart home connection status changes.
   * This allows other components to react to connection events without tight coupling.
   *
   * @param callback Function to call when status changes (can be NULL to unregister)
   */
  void smart_home_register_status_callback(smart_home_status_callback_t callback);

  /**
   * @brief Register a callback for smart home states synchronization updates
   *
   * The callback will be called whenever switch states are synchronized with Home Assistant.
   * This allows other components to be notified of state changes without tight coupling.
   *
   * @param callback Function to call when states are synced (can be NULL to unregister)
   */
  void smart_home_register_states_sync_callback(smart_home_states_sync_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif // SMART_HOME_H
