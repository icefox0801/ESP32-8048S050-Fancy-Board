/**
 * @file home_assistant_api.h
 * @brief Home Assistant REST API Client Header
 *
 * This module provides HTTP client functionality for interacting with Home Assistant
 * REST API, including device control, sensor reading, and state management.
 *
 * Features:
 * - HTTP client with authentication
 * - Entity state reading and writing
 * - Service calls (switch toggle, etc.)
 * - JSON response parsing
 * - Connection pooling and retry logic
 *
 * @author System Monitor Dashboard
 * @date 2025-08-14
 */

#ifndef HOME_ASSISTANT_API_H
#define HOME_ASSISTANT_API_H

#include <esp_err.h>
#include <esp_http_client.h>
#include <cJSON.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

// ═══════════════════════════════════════════════════════════════════════════════
// CONSTANTS AND CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════

/** Maximum length for entity IDs */
#define HA_MAX_ENTITY_ID_LEN 64

/** Maximum length for entity states */
#define HA_MAX_STATE_LEN 128

/** Maximum length for attribute values */
#define HA_MAX_ATTRIBUTE_LEN 256

/** Maximum number of attributes per entity */
#define HA_MAX_ATTRIBUTES 16

  // ═══════════════════════════════════════════════════════════════════════════════
  // DATA STRUCTURES
  // ═══════════════════════════════════════════════════════════════════════════════

  /**
   * @brief Home Assistant entity attribute structure
   */
  typedef struct
  {
    char key[32];                     ///< Attribute name
    char value[HA_MAX_ATTRIBUTE_LEN]; ///< Attribute value as string
  } ha_attribute_t;

  /**
   * @brief Home Assistant entity state structure
   */
  typedef struct
  {
    char entity_id[HA_MAX_ENTITY_ID_LEN];         ///< Entity ID (e.g., "switch.pump")
    char state[HA_MAX_STATE_LEN];                 ///< Current state (e.g., "on", "off")
    char friendly_name[64];                       ///< Human-readable name
    uint64_t last_changed;                        ///< Last change timestamp (Unix time)
    uint64_t last_updated;                        ///< Last update timestamp (Unix time)
    ha_attribute_t attributes[HA_MAX_ATTRIBUTES]; ///< Entity attributes
    uint8_t attribute_count;                      ///< Number of attributes
  } ha_entity_state_t;

  /**
   * @brief Home Assistant API response structure
   */
  typedef struct
  {
    int status_code;         ///< HTTP status code
    char *response_data;     ///< Raw response data (JSON)
    size_t response_len;     ///< Response data length
    bool success;            ///< Operation success flag
    char error_message[128]; ///< Error description if failed
  } ha_api_response_t;

  /**
   * @brief Service call data structure
   */
  typedef struct
  {
    char domain[32];                      ///< Service domain (e.g., "switch")
    char service[32];                     ///< Service name (e.g., "toggle")
    char entity_id[HA_MAX_ENTITY_ID_LEN]; ///< Target entity ID
    cJSON *service_data;                  ///< Additional service data (optional)
  } ha_service_call_t;

  // ═══════════════════════════════════════════════════════════════════════════════
  // PUBLIC FUNCTION DECLARATIONS
  // ═══════════════════════════════════════════════════════════════════════════════

  /**
   * @brief Initialize Home Assistant API client
   *
   * Sets up HTTP client with authentication headers and connection pooling.
   * Must be called before using any other HA API functions.
   *
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t ha_api_init(void);

  /**
   * @brief Deinitialize Home Assistant API client
   *
   * Cleans up HTTP client resources and connection pools.
   *
   * @return ESP_OK on success
   */
  esp_err_t ha_api_deinit(void);

  /**
   * @brief Test connection to Home Assistant server
   *
   * Performs a simple API call to verify connectivity and authentication.
   *
   * @return ESP_OK if connection successful, error code otherwise
   */
  esp_err_t ha_api_test_connection(void);

  /**
   * @brief Get state of a specific entity
   *
   * Retrieves current state and attributes for the specified entity.
   *
   * @param entity_id Entity ID to query (e.g., "switch.pump")
   * @param state Pointer to state structure to fill
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t ha_api_get_entity_state(const char *entity_id, ha_entity_state_t *state);

  /**
   * @brief Call a Home Assistant service
   *
   * Executes a service call (like turning on/off a switch).
   *
   * @param service_call Service call configuration
   * @param response Response structure (optional, can be NULL)
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t ha_api_call_service(const ha_service_call_t *service_call, ha_api_response_t *response);

  /**
   * @brief Toggle a switch entity
   *
   * Convenience function to toggle a switch on/off.
   *
   * @param entity_id Switch entity ID (e.g., "switch.pump")
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t ha_api_toggle_switch(const char *entity_id);

  /**
   * @brief Turn on a switch entity
   *
   * Convenience function to turn on a switch.
   *
   * @param entity_id Switch entity ID
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t ha_api_turn_on_switch(const char *entity_id);

  /**
   * @brief Turn off a switch entity
   *
   * Convenience function to turn off a switch.
   *
   * @param entity_id Switch entity ID
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t ha_api_turn_off_switch(const char *entity_id);

  /**
   * @brief Get sensor value as float
   *
   * Retrieves numeric sensor value (temperature, humidity, etc.).
   *
   * @param entity_id Sensor entity ID
   * @param value Pointer to store sensor value
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t ha_api_get_sensor_value(const char *entity_id, float *value);

  /**
   * @brief Get all entities matching a pattern
   *
   * Retrieves all entities whose IDs contain the specified pattern.
   *
   * @param pattern Search pattern (e.g., "sensor" to find all sensors)
   * @param entities Array to store found entities
   * @param max_entities Maximum number of entities to return
   * @param found_count Pointer to store actual number found
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t ha_api_get_entities_by_pattern(const char *pattern, ha_entity_state_t *entities,
                                           uint16_t max_entities, uint16_t *found_count);

  /**
   * @brief Set entity state
   *
   * Updates the state of an entity (for input sensors, etc.).
   *
   * @param entity_id Entity ID to update
   * @param new_state New state value
   * @param attributes Optional attributes JSON object
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t ha_api_set_entity_state(const char *entity_id, const char *new_state, cJSON *attributes);

  /**
   * @brief Parse JSON response into entity state
   *
   * Utility function to parse HA API JSON response into entity state structure.
   *
   * @param json_str JSON response string
   * @param state Pointer to state structure to fill
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t ha_api_parse_entity_state(const char *json_str, ha_entity_state_t *state);

  /**
   * @brief Free API response resources
   *
   * Cleans up memory allocated for API response data.
   *
   * @param response Response structure to clean up
   */
  void ha_api_free_response(ha_api_response_t *response);

  /**
   * @brief Get human-readable error message
   *
   * Converts error codes to descriptive error messages.
   *
   * @param error_code ESP error code
   * @return Human-readable error description
   */
  const char *ha_api_get_error_string(esp_err_t error_code);

#ifdef __cplusplus
}
#endif

#endif // HOME_ASSISTANT_API_H
