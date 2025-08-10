/**
 * @file crash_log_manager.h
 * @brief Crash Log Manager Header
 *
 * Provides crash logging functionality with persistent storage in flash
 * for the ESP32-S3-8048S050 system monitor dashboard.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Maximum number of crash logs to store
 */
#define CRASH_LOG_MAX_ENTRIES 5

/**
 * @brief Maximum size for crash reason string
 */
#define CRASH_REASON_MAX_LEN 64

/**
 * @brief Maximum size for backtrace string
 */
#define CRASH_BACKTRACE_MAX_LEN 256

  /**
   * @brief Crash log entry structure
   */
  typedef struct
  {
    uint32_t timestamp;                      // Unix timestamp of crash
    uint32_t uptime_seconds;                 // System uptime at crash
    char reason[CRASH_REASON_MAX_LEN];       // Crash reason (exception type)
    char backtrace[CRASH_BACKTRACE_MAX_LEN]; // Stack backtrace
    uint32_t free_heap;                      // Free heap at crash time
    uint32_t min_free_heap;                  // Minimum free heap ever
    bool valid;                              // Entry validity flag
  } crash_log_entry_t;

  /**
   * @brief Initialize crash log manager
   * @return ESP_OK on success, error code otherwise
   */
  esp_err_t crash_log_manager_init(void);

  /**
   * @brief Store a crash log entry
   * @param reason Crash reason string
   * @param backtrace Stack backtrace string
   * @return ESP_OK on success, error code otherwise
   */
  esp_err_t crash_log_store(const char *reason, const char *backtrace);

  /**
   * @brief Print all stored crash logs to console
   */
  void crash_log_print_all(void);

  /**
   * @brief Get number of stored crash logs
   * @return Number of valid crash log entries
   */
  uint8_t crash_log_get_count(void);

  /**
   * @brief Clear all stored crash logs
   * @return ESP_OK on success, error code otherwise
   */
  esp_err_t crash_log_clear_all(void);

  /**
   * @brief Get a specific crash log entry
   * @param index Index of crash log (0 = most recent)
   * @param entry Pointer to store the crash log entry
   * @return ESP_OK on success, ESP_ERR_INVALID_ARG if index out of range
   */
  esp_err_t crash_log_get_entry(uint8_t index, crash_log_entry_t *entry);

#ifdef __cplusplus
}
#endif
