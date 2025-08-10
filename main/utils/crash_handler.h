/**
 * @file crash_handler.h
 * @brief Crash Handler Header
 *
 * Provides automatic crash detection and logging functionality
 * for the ESP32-S3-8048S050 system monitor dashboard.
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @brief Initialize crash handler
   * @return ESP_OK on success, error code otherwise
   */
  esp_err_t crash_handler_init(void);

  /**
   * @brief Manual crash log trigger (for testing)
   * @param reason Custom crash reason
   */
  void crash_handler_trigger_test(const char *reason);

  /**
   * @brief Trigger different types of crashes for testing
   */
  void crash_handler_trigger_null_pointer(void);
  void crash_handler_trigger_stack_overflow(void);
  void crash_handler_trigger_heap_corruption(void);
  void crash_handler_trigger_assert_fail(void);
  void crash_handler_trigger_watchdog_timeout(void);
  void crash_handler_trigger_abort(void);

#ifdef __cplusplus
}
#endif
