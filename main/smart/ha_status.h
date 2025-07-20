/**
 * @file ha_status.h
 * @brief Home Assistant Status Management Module
 *
 * This module provides centralized status management for Home Assistant integration,
 * including status change notifications and callbacks.
 *
 * @author System Monitor Dashboard
 * @date 2025-08-19
 */

#ifndef HA_STATUS_H
#define HA_STATUS_H

#include <esp_err.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

  // ═══════════════════════════════════════════════════════════════════════════════
  // STATUS ENUMERATIONS
  // ═══════════════════════════════════════════════════════════════════════════════

  /**
   * @brief Home Assistant status enumeration
   */
  typedef enum
  {
    HA_STATUS_OFFLINE = 0,   /**< HA API is offline/not initialized */
    HA_STATUS_SYNCING,       /**< HA API is syncing data */
    HA_STATUS_READY,         /**< HA API is ready and operational */
    HA_STATUS_STATES_SYNCED, /**< Entity states successfully synced */
    HA_STATUS_PARTIAL_SYNC,  /**< Partial sync completed */
    HA_STATUS_SYNC_FAILED,   /**< Sync operation failed */
  } ha_status_t;

  // ═══════════════════════════════════════════════════════════════════════════════
  // CALLBACK TYPES
  // ═══════════════════════════════════════════════════════════════════════════════

  /**
   * @brief HA status change callback function type
   * @param status Current HA status
   * @param status_text Human-readable status description
   * @param is_ready True if the HA API is ready, false otherwise
   * @param is_syncing True if the HA API is syncing, false otherwise
   */
  typedef void (*ha_status_change_callback_t)(bool is_ready, bool is_syncing, const char *status_text);

  // ═══════════════════════════════════════════════════════════════════════════════
  // PUBLIC FUNCTIONS
  // ═══════════════════════════════════════════════════════════════════════════════

  /**
   * @brief Initialize the HA status module
   * @return ESP_OK on success, error code on failure
   */
  esp_err_t ha_status_init(void);

  /**
   * @brief Deinitialize the HA status module
   * @return ESP_OK on success
   */
  esp_err_t ha_status_deinit(void);

  /**
   * @brief Register a callback for HA status changes
   * @param callback Function to call when HA status changes (NULL to unregister)
   */
  void ha_status_register_change_callback(ha_status_change_callback_t callback);

  /**
   * @brief Update the current HA status
   * @param status New HA status
   */
  void ha_status_change(ha_status_t status);

  /**
   * @brief Get the current HA status
   * @return Current HA status
   */
  ha_status_t ha_status_get_current(void);

  /**
   * @brief Get human-readable status text for a status enum
   * @param status HA status enum value
   * @return Static string describing the status
   */
  const char *ha_status_get_text(ha_status_t status);

#ifdef __cplusplus
}
#endif

#endif // HA_STATUS_H
