/**
 * @file crash_log_manager.c
 * @brief Crash Log Manager Implementation
 *
 * Provides crash logging functionality with persistent storage in flash
 * for the ESP32-S3-8048S050 system monitor dashboard.
 */

#include "crash_log_manager.h"

#include <string.h>
#include <time.h>
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "utils/system_debug_utils.h"

#define CRASH_LOG_NVS_NAMESPACE "crash_logs"
#define CRASH_LOG_COUNT_KEY "count"
#define CRASH_LOG_INDEX_KEY "index"
#define CRASH_LOG_ENTRY_KEY_PREFIX "log_"

static nvs_handle_t crash_log_nvs_handle = 0;
static bool crash_log_initialized = false;
static uint8_t crash_log_count = 0;
static uint8_t crash_log_write_index = 0;

/**
 * @brief Generate NVS key for crash log entry
 */
static void get_crash_log_key(uint8_t index, char *key, size_t key_size)
{
  snprintf(key, key_size, "%s%d", CRASH_LOG_ENTRY_KEY_PREFIX, index);
}

/**
 * @brief Load crash log metadata from NVS
 */
static esp_err_t load_crash_log_metadata(void)
{
  size_t required_size = sizeof(uint8_t);

  // Load crash count
  esp_err_t err = nvs_get_blob(crash_log_nvs_handle, CRASH_LOG_COUNT_KEY,
                               &crash_log_count, &required_size);
  if (err == ESP_ERR_NVS_NOT_FOUND)
  {
    crash_log_count = 0;
  }
  else if (err != ESP_OK)
  {
    return err;
  }

  // Load write index
  required_size = sizeof(uint8_t);
  err = nvs_get_blob(crash_log_nvs_handle, CRASH_LOG_INDEX_KEY,
                     &crash_log_write_index, &required_size);
  if (err == ESP_ERR_NVS_NOT_FOUND)
  {
    crash_log_write_index = 0;
  }
  else if (err != ESP_OK)
  {
    return err;
  }

  // Validate loaded values
  if (crash_log_count > CRASH_LOG_MAX_ENTRIES)
  {
    crash_log_count = CRASH_LOG_MAX_ENTRIES;
  }
  if (crash_log_write_index >= CRASH_LOG_MAX_ENTRIES)
  {
    crash_log_write_index = 0;
  }

  return ESP_OK;
}

/**
 * @brief Save crash log metadata to NVS
 */
static esp_err_t save_crash_log_metadata(void)
{
  esp_err_t err = nvs_set_blob(crash_log_nvs_handle, CRASH_LOG_COUNT_KEY,
                               &crash_log_count, sizeof(uint8_t));
  if (err != ESP_OK)
  {
    return err;
  }

  err = nvs_set_blob(crash_log_nvs_handle, CRASH_LOG_INDEX_KEY,
                     &crash_log_write_index, sizeof(uint8_t));
  if (err != ESP_OK)
  {
    return err;
  }

  return nvs_commit(crash_log_nvs_handle);
}

esp_err_t crash_log_manager_init(void)
{
  if (crash_log_initialized)
  {
    return ESP_OK;
  }

  debug_log_startup(DEBUG_TAG_SYSTEM, "Crash Log Manager");

  // Open NVS handle
  esp_err_t err = nvs_open(CRASH_LOG_NVS_NAMESPACE, NVS_READWRITE, &crash_log_nvs_handle);
  if (err != ESP_OK)
  {
    debug_log_error_f(DEBUG_TAG_SYSTEM, "Failed to open NVS for crash logs: %s", esp_err_to_name(err));
    return err;
  }

  // Load existing metadata
  err = load_crash_log_metadata();
  if (err != ESP_OK)
  {
    debug_log_error_f(DEBUG_TAG_SYSTEM, "Failed to load crash log metadata: %s", esp_err_to_name(err));
    nvs_close(crash_log_nvs_handle);
    return err;
  }

  crash_log_initialized = true;

  debug_log_info_f(DEBUG_TAG_SYSTEM, "Crash log manager initialized - %d logs stored", crash_log_count);

  // Print existing crash logs at startup
  if (crash_log_count > 0)
  {
    debug_log_info(DEBUG_TAG_SYSTEM, "=== PREVIOUS CRASH LOGS ===");
    crash_log_print_all();
    debug_log_info(DEBUG_TAG_SYSTEM, "=== END CRASH LOGS ===");
  }

  return ESP_OK;
}

esp_err_t crash_log_store(const char *reason, const char *backtrace)
{
  if (!crash_log_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  if (!reason || !backtrace)
  {
    return ESP_ERR_INVALID_ARG;
  }

  crash_log_entry_t entry = {0};

  // Fill entry data
  entry.timestamp = (uint32_t)time(NULL);
  entry.uptime_seconds = xTaskGetTickCount() / configTICK_RATE_HZ;
  strncpy(entry.reason, reason, CRASH_REASON_MAX_LEN - 1);
  strncpy(entry.backtrace, backtrace, CRASH_BACKTRACE_MAX_LEN - 1);
  entry.free_heap = esp_get_free_heap_size();
  entry.min_free_heap = esp_get_minimum_free_heap_size();
  entry.valid = true;

  // Generate key for this entry
  char key[32];
  get_crash_log_key(crash_log_write_index, key, sizeof(key));

  // Store entry in NVS
  esp_err_t err = nvs_set_blob(crash_log_nvs_handle, key, &entry, sizeof(crash_log_entry_t));
  if (err != ESP_OK)
  {
    debug_log_error_f(DEBUG_TAG_SYSTEM, "Failed to store crash log: %s", esp_err_to_name(err));
    return err;
  }

  // Update metadata
  crash_log_write_index = (crash_log_write_index + 1) % CRASH_LOG_MAX_ENTRIES;
  if (crash_log_count < CRASH_LOG_MAX_ENTRIES)
  {
    crash_log_count++;
  }

  // Save metadata
  err = save_crash_log_metadata();
  if (err != ESP_OK)
  {
    debug_log_error_f(DEBUG_TAG_SYSTEM, "Failed to save crash log metadata: %s", esp_err_to_name(err));
    return err;
  }

  debug_log_info_f(DEBUG_TAG_SYSTEM, "Crash log stored (entry %d/%d)", crash_log_count, CRASH_LOG_MAX_ENTRIES);

  return ESP_OK;
}

void crash_log_print_all(void)
{
  if (!crash_log_initialized || crash_log_count == 0)
  {
    debug_log_info(DEBUG_TAG_SYSTEM, "No crash logs stored");
    return;
  }

  for (uint8_t i = 0; i < crash_log_count; i++)
  {
    crash_log_entry_t entry;
    if (crash_log_get_entry(i, &entry) == ESP_OK)
    {
      // Calculate relative index (most recent first)
      uint8_t display_index = i + 1;

      debug_log_info_f(DEBUG_TAG_SYSTEM, "--- CRASH LOG %d ---", display_index);
      debug_log_info_f(DEBUG_TAG_SYSTEM, "Timestamp: %lu", entry.timestamp);
      debug_log_info_f(DEBUG_TAG_SYSTEM, "Uptime: %lu seconds", entry.uptime_seconds);
      debug_log_info_f(DEBUG_TAG_SYSTEM, "Reason: %s", entry.reason);
      debug_log_info_f(DEBUG_TAG_SYSTEM, "Free heap: %lu bytes", entry.free_heap);
      debug_log_info_f(DEBUG_TAG_SYSTEM, "Min free heap: %lu bytes", entry.min_free_heap);
      debug_log_info_f(DEBUG_TAG_SYSTEM, "Backtrace: %s", entry.backtrace);
    }
  }
}

uint8_t crash_log_get_count(void)
{
  return crash_log_count;
}

esp_err_t crash_log_clear_all(void)
{
  if (!crash_log_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  // Clear all entries
  for (uint8_t i = 0; i < CRASH_LOG_MAX_ENTRIES; i++)
  {
    char key[32];
    get_crash_log_key(i, key, sizeof(key));
    nvs_erase_key(crash_log_nvs_handle, key); // Ignore errors for non-existent keys
  }

  // Reset metadata
  crash_log_count = 0;
  crash_log_write_index = 0;

  esp_err_t err = save_crash_log_metadata();
  if (err == ESP_OK)
  {
    debug_log_info(DEBUG_TAG_SYSTEM, "All crash logs cleared");
  }

  return err;
}

esp_err_t crash_log_get_entry(uint8_t index, crash_log_entry_t *entry)
{
  if (!crash_log_initialized || !entry)
  {
    return ESP_ERR_INVALID_STATE;
  }

  if (index >= crash_log_count)
  {
    return ESP_ERR_INVALID_ARG;
  }

  // Calculate actual storage index (most recent entries first)
  uint8_t actual_index;
  if (crash_log_count < CRASH_LOG_MAX_ENTRIES)
  {
    // Not wrapped around yet
    actual_index = crash_log_count - 1 - index;
  }
  else
  {
    // Wrapped around - calculate from write index
    actual_index = (crash_log_write_index - 1 - index + CRASH_LOG_MAX_ENTRIES) % CRASH_LOG_MAX_ENTRIES;
  }

  char key[32];
  get_crash_log_key(actual_index, key, sizeof(key));

  size_t required_size = sizeof(crash_log_entry_t);
  esp_err_t err = nvs_get_blob(crash_log_nvs_handle, key, entry, &required_size);

  if (err != ESP_OK || !entry->valid)
  {
    return ESP_ERR_NOT_FOUND;
  }

  return ESP_OK;
}
