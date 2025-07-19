/**
 * @file system_debug_utils.h
 * @brief Minimal Debug Utilities
 *
 * Minimal debug functions controlled by CONFIG_SYSTEM_DEBUG_ENABLED.
 */

#ifndef SYSTEM_DEBUG_UTILS_H
#define SYSTEM_DEBUG_UTILS_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @brief Debug component tags enum
   */
  typedef enum
  {
    DEBUG_TAG_DASHBOARD = 0,
    DEBUG_TAG_SERIAL_DATA,
    DEBUG_TAG_WIFI_MANAGER,
    DEBUG_TAG_SMART_HOME,
    DEBUG_TAG_UI_DASHBOARD,
    DEBUG_TAG_UI_CONTROLS,
    DEBUG_TAG_GT911_TOUCH,
    DEBUG_TAG_HA_TASK_MGR,
    DEBUG_TAG_HA_SYNC,
    DEBUG_TAG_HA_API,
    DEBUG_TAG_LVGL_SETUP,
    DEBUG_TAG_SYSTEM,
    DEBUG_TAG_MAX
  } debug_tag_t;

  /**
   * @brief Log system startup information
   * @param tag Debug tag identifying the component
   * @param component_name Name of the component being started
   */
  void debug_log_startup(debug_tag_t tag, const char *component_name);

  /**
   * @brief Log critical errors
   * @param tag Debug tag identifying the component
   * @param error_msg Error message
   */
  void debug_log_error(debug_tag_t tag, const char *error_msg);

  /**
   * @brief Log important operational events
   * @param tag Debug tag identifying the component
   * @param event_msg Event description
   */
  void debug_log_event(debug_tag_t tag, const char *event_msg);

  /**
   * @brief Check stack and heap health for current task
   * @param tag Debug tag identifying the component
   */
  void debug_check_task_health(debug_tag_t tag);

  /**
   * @brief Check if sufficient heap is available for task creation
   * @param tag Debug tag identifying the component
   * @param required_bytes Minimum required free heap in bytes
   * @return true if sufficient memory available, false otherwise
   */
  bool debug_check_heap_sufficient(debug_tag_t tag, size_t required_bytes);

  /**
   * @brief Print basic memory usage information
   * @param tag Debug tag identifying the component
   * @param task_handle Optional task handle to check stack usage (can be NULL)
   */
  void debug_print_memory_usage(debug_tag_t tag, void *task_handle);

  /**
   * @brief Log general information messages
   * @param tag Debug tag identifying the component
   * @param info_msg Information message
   */
  void debug_log_info(debug_tag_t tag, const char *info_msg);

  /**
   * @brief Log warning messages
   * @param tag Debug tag identifying the component
   * @param warning_msg Warning message
   */
  void debug_log_warning(debug_tag_t tag, const char *warning_msg);

  /**
   * @brief Log debug messages (verbose)
   * @param tag Debug tag identifying the component
   * @param debug_msg Debug message
   */
  void debug_log_debug(debug_tag_t tag, const char *debug_msg);

  /**
   * @brief Log formatted information messages
   * @param tag Debug tag identifying the component
   * @param format Printf-style format string
   * @param ... Variable arguments
   */
  void debug_log_info_f(debug_tag_t tag, const char *format, ...);

  /**
   * @brief Log formatted error messages
   * @param tag Debug tag identifying the component
   * @param format Printf-style format string
   * @param ... Variable arguments
   */
  void debug_log_error_f(debug_tag_t tag, const char *format, ...);

  /**
   * @brief Log formatted warning messages
   * @param tag Debug tag identifying the component
   * @param format Printf-style format string
   * @param ... Variable arguments
   */
  void debug_log_warning_f(debug_tag_t tag, const char *format, ...);

  /**
   * @brief Log formatted debug messages (verbose)
   * @param tag Debug tag identifying the component
   * @param format Printf-style format string
   * @param ... Variable arguments
   */
  void debug_log_debug_f(debug_tag_t tag, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_DEBUG_UTILS_H
