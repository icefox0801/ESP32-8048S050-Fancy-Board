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

// ═══════════════════════════════════════════════════════════════════════════════
// CONSTANTS AND CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════

/** Maximum number of monitored devices */
#define SMART_HOME_MAX_DEVICES 16

/** Status update intervals */
#define SMART_HOME_FAST_UPDATE_MS 5000  // Quick status updates
#define SMART_HOME_SLOW_UPDATE_MS 30000 // Sensor readings

  // ═══════════════════════════════════════════════════════════════════════════════
  // DATA STRUCTURES
  // ═══════════════════════════════════════════════════════════════════════════════

  /**
   * @brief Smart home device types
   */
  typedef enum
  {
    SMART_DEVICE_SWITCH = 0, ///< On/off switch
    SMART_DEVICE_LIGHT,      ///< Dimmable light
    SMART_DEVICE_SENSOR,     ///< Sensor (temperature, humidity, etc.)
    SMART_DEVICE_CLIMATE,    ///< Thermostat/AC control
    SMART_DEVICE_FAN,        ///< Variable speed fan
    SMART_DEVICE_CAMERA,     ///< Security camera
    SMART_DEVICE_LOCK,       ///< Smart lock
    SMART_DEVICE_UNKNOWN     ///< Unknown device type
  } smart_device_type_t;

  /**
   * @brief Smart home device status
   */
  typedef struct
  {
    char entity_id[64];       ///< Home Assistant entity ID
    char friendly_name[64];   ///< Human-readable name
    smart_device_type_t type; ///< Device type
    char state[32];           ///< Current state (on/off, temperature, etc.)
    bool available;           ///< Device availability status
    uint64_t last_updated;    ///< Last update timestamp
    float numeric_value;      ///< Numeric value for sensors
    char unit[16];            ///< Unit of measurement
  } smart_device_status_t;

  /**
   * @brief Smart home system status
   */
  typedef struct
  {
    bool ha_connected;          ///< Home Assistant connection status
    uint32_t total_devices;     ///< Total number of devices
    uint32_t online_devices;    ///< Number of online devices
    uint32_t offline_devices;   ///< Number of offline devices
    uint64_t last_full_update;  ///< Last complete status update
    char connection_error[128]; ///< Last connection error message
  } smart_home_status_t;

  /**
   * @brief Smart home event callback function type
   * @param device_status Updated device status
   * @param user_data User-provided callback data
   */
  typedef void (*smart_home_event_callback_t)(const smart_device_status_t *device_status, void *user_data);

  // ═══════════════════════════════════════════════════════════════════════════════
  // PUBLIC FUNCTION DECLARATIONS
  // ═══════════════════════════════════════════════════════════════════════════════

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

  /**
   * @brief Get overall smart home system status
   *
   * @param status Pointer to status structure to fill
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t smart_home_get_status(smart_home_status_t *status);

  /**
   * @brief Toggle switch A
   *
   * Convenience function to toggle the configured switch A entity.
   *
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t smart_home_toggle_pump(void);

  /**
   * @brief Turn on switch A
   *
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t smart_home_pump_on(void);

  /**
   * @brief Turn off switch A
   *
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t smart_home_pump_off(void);

  /**
   * @brief Get switch A status
   *
   * @param is_on Pointer to store switch on/off status
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t smart_home_get_pump_status(bool *is_on);

  /**
   * @brief Get temperature sensor reading
   *
   * @param temperature Pointer to store temperature value
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t smart_home_get_temperature(float *temperature);

  /**
   * @brief Get humidity sensor reading
   *
   * @param humidity Pointer to store humidity value
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t smart_home_get_humidity(float *humidity);

  /**
   * @brief Get device status by entity ID
   *
   * @param entity_id Home Assistant entity ID
   * @param device_status Pointer to device status structure to fill
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t smart_home_get_device_status(const char *entity_id, smart_device_status_t *device_status);

  /**
   * @brief Get all monitored devices
   *
   * @param devices Array to store device statuses
   * @param max_devices Maximum number of devices to return
   * @param device_count Pointer to store actual number of devices
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t smart_home_get_all_devices(smart_device_status_t *devices, uint32_t max_devices, uint32_t *device_count);

  /**
   * @brief Register event callback for device updates
   *
   * @param callback Callback function to call on device updates
   * @param user_data User data to pass to callback
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t smart_home_register_callback(smart_home_event_callback_t callback, void *user_data);

  /**
   * @brief Unregister event callback
   *
   * @return ESP_OK on success
   */
  esp_err_t smart_home_unregister_callback(void);

  /**
   * @brief Force refresh of all device statuses
   *
   * Triggers an immediate update of all monitored devices.
   *
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t smart_home_refresh_all(void);

  /**
   * @brief Check Home Assistant connection
   *
   * Tests connectivity to the Home Assistant server.
   *
   * @return ESP_OK if connected, error code if not
   */
  esp_err_t smart_home_test_connection(void);

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
   * @brief Update smart home status in system monitor UI
   *
   * Updates the UI with current smart home connection and device status.
   *
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t smart_home_update_ui(void);

  // ═══════════════════════════════════════════════════════════════════════════════
  // CONVENIENCE FUNCTIONS FOR SPECIFIC DEVICES
  // ═══════════════════════════════════════════════════════════════════════════════

  /**
   * @brief Turn on switch B
   *
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t smart_home_wave_maker_on(void);

  /**
   * @brief Turn off switch B
   *
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t smart_home_wave_maker_off(void);

  /**
   * @brief Toggle switch B
   *
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t smart_home_wave_maker_toggle(void);

  /**
   * @brief Turn on switch C
   *
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t smart_home_light_on(void);

  /**
   * @brief Turn off switch C
   *
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t smart_home_light_off(void);

  /**
   * @brief Toggle switch C
   *
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t smart_home_light_toggle(void);

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
