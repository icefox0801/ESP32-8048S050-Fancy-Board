/**
 * @file ha_api.h
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

#ifndef HA_API_H
#define HA_API_H

#include <esp_err.h>
#include <esp_http_client.h>
#include <cJSON.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "ha_status.h"

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
#define HA_MAX_STATE_LEN 256

/** Maximum length for friendly names */
#define HA_MAX_FRIENDLY_NAME_LEN 64

  // ═══════════════════════════════════════════════════════════════════════════════
  // DATA STRUCTURES
  // ═══════════════════════════════════════════════════════════════════════════════

  /**
   * @brief Simplified Home Assistant entity state structure
   * Optimized for switch states without complex attributes
   */
  typedef struct
  {
    char entity_id[HA_MAX_ENTITY_ID_LEN];         ///< Entity ID (e.g., "switch.pump")
    char state[HA_MAX_STATE_LEN];                 ///< Current state (e.g., "on", "off")
    char friendly_name[HA_MAX_FRIENDLY_NAME_LEN]; ///< Human-readable name
    time_t last_updated;                          ///< Last update timestamp (Unix time)
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
   * @brief Get states of multiple entities in bulk
   *
   * Retrieves current states for multiple entities efficiently using the bulk states API.
   *
   * @param entity_ids Array of entity IDs to query
   * @param entity_count Number of entities to query
   * @param states Array of state structures to fill (must be same size as entity_ids)
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t ha_api_get_multiple_entity_states(const char **entity_ids, int entity_count, ha_entity_state_t *states);

  /**
   * @brief Get states of multiple entities using bulk API request
   *
   * More efficient alternative that fetches ALL states in one request and filters results.
   * Better for multiple entities but larger payload size.
   *
   * @param entity_ids Array of entity IDs to query
   * @param entity_count Number of entities to query
   * @param states Array of state structures to fill (must be same size as entity_ids)
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t ha_api_get_multiple_entity_states_bulk(const char **entity_ids, int entity_count, ha_entity_state_t *states);

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
   * @brief Check if Home Assistant API is ready
   *
   * @return true if HA API is initialized and ready, false otherwise
   */
  bool ha_api_is_ready(void);

#ifdef __cplusplus
}
#endif

#endif
