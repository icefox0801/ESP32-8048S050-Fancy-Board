/**
 * @file smart_home.h
 * @brief Smart Home Integration Manager
 *
 * This module provides a high-level interface for smart home automation
 * including Home Assistant integration, device control, and sensor monitoring.
 *
 * Features:
 * - Simplified device control interface
 * - Automatic sensor data collection
 * - Smart home status monitoring
 * - Integration with system monitor UI
 *
 * @author System Monitor Dashboard
 * @date 2025-08-14
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
   * Sets up Home Assistant connection and starts monitoring tasks.
   *
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t smart_home_init(void);

  /**
   * @brief Deinitialize smart home integration
   *
   * Stops monitoring tasks and cleans up resources.
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

#ifdef __cplusplus
}
#endif

#endif // SMART_HOME_H
