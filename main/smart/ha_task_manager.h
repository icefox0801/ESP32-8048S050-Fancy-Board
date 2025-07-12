/**
 * @file ha_task_manager.h
 * @brief Home Assistant Task Manager
 *
 * This module manages the Home Assistant sync task and provides
 * a simple interface for starting/stopping and controlling sync operations.
 */

#ifndef HA_TASK_MANAGER_H
#define HA_TASK_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @brief Initialize the Home Assistant task manager
   * @return ESP_OK on success
   */
  esp_err_t ha_task_manager_init(void);

  /**
   * @brief Deinitialize the Home Assistant task manager
   * @return ESP_OK on success
   */
  esp_err_t ha_task_manager_deinit(void);

  /**
   * @brief Start the Home Assistant sync task
   * @return ESP_OK on success
   */
  esp_err_t ha_task_manager_start_task(void);

  /**
   * @brief Stop the Home Assistant sync task
   * @return ESP_OK on success
   */
  esp_err_t ha_task_manager_stop_task(void);

  /**
   * @brief Check if the Home Assistant task is running
   * @return true if task is running, false otherwise
   */
  bool ha_task_manager_is_task_running(void);

  /**
   * @brief Request immediate synchronization
   * @return ESP_OK on success
   */
  esp_err_t ha_task_manager_request_immediate_sync(void);

  /**
   * @brief Request Home Assistant API initialization
   * @return ESP_OK on success
   */
  esp_err_t ha_task_manager_request_init(void);

  /**
   * @brief WiFi connection callback for Home Assistant task management
   * @param is_connected true if WiFi is connected, false otherwise
   */
  void ha_task_manager_wifi_callback(bool is_connected);

  /**
   * @brief Print detailed memory usage information for all tasks
   * Shows internal RAM usage, task stack usage, and system memory statistics
   */
  void ha_task_manager_print_memory_usage(void);

#ifdef __cplusplus
}
#endif

#endif // HA_TASK_MANAGER_H
