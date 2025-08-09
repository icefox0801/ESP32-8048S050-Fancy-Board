/**
 * @file ha_status.c
 * @brief Home Assistant Status Management Module Implementation
 *
 * This module provides centralized status management for Home Assistant integration,
 * including status change notifications and callbacks.
 *
 * @author System Monitor Dashboard
 * @date 2025-08-19
 */

#include "ha_status.h"
#include "utils/system_debug_utils.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <string.h>

// =======================================================================
// CONSTANTS AND MACROS
// =======================================================================

// =======================================================================
// STATIC VARIABLES
// =======================================================================

static ha_status_t current_status = HA_STATUS_OFFLINE;
static ha_status_change_callback_t status_callback = NULL;
static SemaphoreHandle_t status_mutex = NULL;
static bool initialized = false;

// =======================================================================
// STATUS TEXT MAPPING
// =======================================================================

static const char *status_text_map[] = {
    [HA_STATUS_OFFLINE] = "Offline",
    [HA_STATUS_SYNCING] = "Syncing...",
    [HA_STATUS_READY] = "Ready",
    [HA_STATUS_STATES_SYNCED] = "States Synced",
    [HA_STATUS_PARTIAL_SYNC] = "Partial Sync",
    [HA_STATUS_SYNC_FAILED] = "Sync Failed",
};

static bool ha_status_is_ready(void)
{
  ha_status_t status = ha_status_get_current();
  return (status == HA_STATUS_READY || status == HA_STATUS_STATES_SYNCED);
}

static bool ha_status_is_syncing(void)
{
  ha_status_t status = ha_status_get_current();
  return (status == HA_STATUS_SYNCING);
}

// =======================================================================
// PUBLIC FUNCTIONS
// =======================================================================

esp_err_t ha_status_init(void)
{
  if (initialized)
  {
    debug_log_warning(DEBUG_TAG_HA_SYNC, "Already initialized");
    return ESP_OK;
  }

  // Create mutex for thread safety
  status_mutex = xSemaphoreCreateMutex();
  if (status_mutex == NULL)
  {
    debug_log_error(DEBUG_TAG_HA_SYNC, "Failed to create status mutex");
    return ESP_ERR_NO_MEM;
  }

  initialized = true;
  ha_status_change(HA_STATUS_OFFLINE);

  debug_log_startup(DEBUG_TAG_HA_SYNC, "HA Status Module");
  return ESP_OK;
}

esp_err_t ha_status_deinit(void)
{
  if (!initialized)
  {
    return ESP_OK;
  }

  if (xSemaphoreTake(status_mutex, pdMS_TO_TICKS(1000)) == pdTRUE)
  {
    status_callback = NULL;
    current_status = HA_STATUS_OFFLINE;
    xSemaphoreGive(status_mutex);
  }

  if (status_mutex != NULL)
  {
    vSemaphoreDelete(status_mutex);
    status_mutex = NULL;
  }

  initialized = false;

  return ESP_OK;
}

void ha_status_register_change_callback(ha_status_change_callback_t callback)
{
  if (!initialized)
  {
    debug_log_error(DEBUG_TAG_HA_SYNC, "Module not initialized");
    return;
  }

  if (xSemaphoreTake(status_mutex, pdMS_TO_TICKS(1000)) == pdTRUE)
  {
    status_callback = callback;
    debug_log_info_f(DEBUG_TAG_HA_SYNC, "Status change callback %s",
                     callback ? "registered" : "unregistered");
    xSemaphoreGive(status_mutex);
  }
  else
  {
    debug_log_error(DEBUG_TAG_HA_SYNC, "Failed to acquire mutex for callback registration");
  }
}

void ha_status_change(ha_status_t status)
{
  if (!initialized)
  {
    debug_log_error(DEBUG_TAG_HA_SYNC, "Module not initialized");
    return;
  }

  // Validate status enum
  if (status >= sizeof(status_text_map) / sizeof(status_text_map[0]))
  {
    debug_log_error_f(DEBUG_TAG_HA_SYNC, "Invalid status value: %d", status);
    return;
  }

  ha_status_change_callback_t callback_to_call = NULL;
  const char *status_text = ha_status_get_text(status);

  if (xSemaphoreTake(status_mutex, pdMS_TO_TICKS(1000)) == pdTRUE)
  {
    // Only update if status actually changed
    if (current_status != status)
    {
      ha_status_t old_status = current_status;
      current_status = status;
      callback_to_call = status_callback;

      debug_log_info_f(DEBUG_TAG_HA_SYNC, "Status changed: %s -> %s",
                       ha_status_get_text(old_status),
                       ha_status_get_text(status));
    }
    xSemaphoreGive(status_mutex);
  }
  else
  {
    debug_log_error(DEBUG_TAG_HA_SYNC, "Failed to acquire mutex for status change");
    return;
  }

  // Call callback outside of mutex to avoid deadlock
  if (callback_to_call != NULL)
  {
    callback_to_call(ha_status_is_ready(), ha_status_is_syncing(), status_text);
  }
}

ha_status_t ha_status_get_current(void)
{
  if (!initialized)
  {
    return HA_STATUS_OFFLINE;
  }

  ha_status_t status = HA_STATUS_OFFLINE;
  if (xSemaphoreTake(status_mutex, pdMS_TO_TICKS(1000)) == pdTRUE)
  {
    status = current_status;
    xSemaphoreGive(status_mutex);
  }
  return status;
}

const char *ha_status_get_text(ha_status_t status)
{
  if (status >= sizeof(status_text_map) / sizeof(status_text_map[0]))
  {
    return "Unknown";
  }
  return status_text_map[status];
}

void ha_status_register_callback(ha_status_change_callback_t callback)
{
  // This is an alias for ha_status_register_change_callback
  ha_status_register_change_callback(callback);
}
