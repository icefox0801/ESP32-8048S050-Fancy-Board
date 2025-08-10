/**
 * @file entity_states_parser.h
 * @brief Async Entity States JSON Parser for Home Assistant API
 *
 * This module provides asynchronous JSON parsing functionality for Home Assistant
 * entity states using SPIRAM for large response handling. The parser runs on
 * CPU idle time to avoid blocking the main application.
 *
 * Features:
 * - Async JSON parsing using FreeRTOS task
 * - SPIRAM allocation for large responses
 * - Background processing with idle-time CPU usage
 * - Entity state extraction and filtering
 * - Performance timing and monitoring
 *
 * @author System Monitor Dashboard
 * @date 2025-08-19
 */

#ifndef ENTITY_STATES_PARSER_H
#define ENTITY_STATES_PARSER_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "ha_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

// =======================================================================
// CONSTANTS AND CONFIGURATION
// =======================================================================

/** Maximum number of async parse jobs in queue */
#define ENTITY_PARSER_MAX_JOBS 2

/** Async parse task stack size */
#define ENTITY_PARSER_TASK_STACK_SIZE 8192

/** Async parse task priority (low for idle processing) */
#define ENTITY_PARSER_TASK_PRIORITY 2

/** Core affinity for parser task (same as LVGL) */
#define ENTITY_PARSER_TASK_CORE 1

  // =======================================================================
  // DATA STRUCTURES
  // =======================================================================

  /**
   * @brief Async JSON parsing job structure
   *
   * Contains all data needed for background JSON parsing in SPIRAM
   */
  typedef struct
  {
    char *json_data;           ///< JSON response data in SPIRAM
    size_t json_size;          ///< Size of JSON data
    const char **entity_ids;   ///< Array of entity IDs to search for
    int entity_count;          ///< Number of entities to find
    ha_entity_state_t *states; ///< Output states array
    TaskHandle_t caller_task;  ///< Task to notify when complete
  } entity_parse_job_t;

  /**
   * @brief Parser performance statistics
   */
  typedef struct
  {
    int64_t total_parse_time_ms;   ///< Total parsing time in milliseconds
    int64_t average_parse_time_ms; ///< Average parsing time per job
    uint32_t jobs_processed;       ///< Number of jobs processed
    uint32_t entities_found;       ///< Total entities successfully found
    uint32_t entities_missing;     ///< Total entities not found
    size_t largest_response_size;  ///< Largest JSON response processed
  } entity_parser_stats_t;

  // =======================================================================
  // PUBLIC FUNCTION DECLARATIONS
  // =======================================================================

  /**
   * @brief Initialize the async entity states parser
   *
   * Creates the background task and queue for async JSON processing.
   * Must be called before using any other parser functions.
   *
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t entity_states_parser_init(void);

  /**
   * @brief Deinitialize the async entity states parser
   *
   * Cleans up resources and stops the background parsing task.
   */
  void entity_states_parser_deinit(void);

  /**
   * @brief Submit JSON data for async parsing
   *
   * Submits JSON response data to be parsed asynchronously in the background.
   * The JSON data will be copied to SPIRAM and processed when CPU is idle.
   * The calling task will be notified when parsing is complete.
   *
   * @param json_data Raw JSON response data (will be copied to SPIRAM)
   * @param json_size Size of JSON data in bytes
   * @param entity_ids Array of entity IDs to search for
   * @param entity_count Number of entities in the array
   * @param states Output array for entity states (must remain valid until completion)
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t entity_states_parser_submit_async(
      const char *json_data,
      size_t json_size,
      const char **entity_ids,
      int entity_count,
      ha_entity_state_t *states);

  /**
   * @brief Wait for async parsing to complete
   *
   * Blocks the calling task until the async parsing job is complete.
   * This function should be called after submitting a job with
   * entity_states_parser_submit_async().
   *
   * @param timeout_ms Timeout in milliseconds (portMAX_DELAY for no timeout)
   * @return ESP_OK if parsing completed successfully, ESP_ERR_TIMEOUT on timeout
   */
  esp_err_t entity_states_parser_wait_completion(uint32_t timeout_ms);

  /**
   * @brief Parse JSON synchronously (blocking)
   *
   * Parses JSON data immediately in the calling task. Use this for small
   * responses or when immediate results are needed.
   *
   * @param json_data Raw JSON response data
   * @param entity_ids Array of entity IDs to search for
   * @param entity_count Number of entities in the array
   * @param states Output array for entity states
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t entity_states_parser_parse_sync(
      const char *json_data,
      const char **entity_ids,
      int entity_count,
      ha_entity_state_t *states);

  /**
   * @brief Get parser performance statistics
   *
   * Returns performance metrics for the async parser including timing
   * and success/failure counts.
   *
   * @param stats Pointer to statistics structure to fill
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t entity_states_parser_get_stats(entity_parser_stats_t *stats);

  /**
   * @brief Reset parser performance statistics
   *
   * Clears all performance counters and statistics.
   */
  void entity_states_parser_reset_stats(void);

  /**
   * @brief Check if parser is ready for use
   *
   * @return true if parser is initialized and ready, false otherwise
   */
  bool entity_states_parser_is_ready(void);

  /**
   * @brief Get number of pending parse jobs
   *
   * @return Number of jobs currently queued for processing
   */
  int entity_states_parser_get_queue_size(void);

#ifdef __cplusplus
}
#endif

#endif // ENTITY_STATES_PARSER_H
