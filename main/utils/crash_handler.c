/**
 * @file crash_handler.c
 * @brief Crash Handler Implementation
 *
 * Provides automatic crash detection and logging functionality
 * for the ESP32-S3-8048S050 system monitor dashboard.
 */

#include "crash_handler.h"
#include "crash_log_manager.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "utils/system_debug_utils.h"

static bool crash_handler_initialized = false;

/**
 * @brief Custom panic handler that stores crash information
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void crash_panic_handler(void *info)
{
  // Try to extract crash information
  char reason[64] = "Unknown crash";
  char backtrace[256] = "Backtrace unavailable";

  // Get basic crash reason
  esp_reset_reason_t reset_reason = esp_reset_reason();
  switch (reset_reason)
  {
  case ESP_RST_PANIC:
    strncpy(reason, "Kernel panic", sizeof(reason) - 1);
    break;
  case ESP_RST_WDT:
    strncpy(reason, "Watchdog timeout", sizeof(reason) - 1);
    break;
  case ESP_RST_BROWNOUT:
    strncpy(reason, "Brownout reset", sizeof(reason) - 1);
    break;
  case ESP_RST_TASK_WDT:
    strncpy(reason, "Task watchdog", sizeof(reason) - 1);
    break;
  default:
    snprintf(reason, sizeof(reason), "Reset reason: %d", reset_reason);
    break;
  }

  // Try to get a simple backtrace
  snprintf(backtrace, sizeof(backtrace), "Backtrace: Post-crash analysis");
  // Note: Full backtrace formatting would require more complex implementation
  // For now, we just store basic information

  // Store the crash log (this should be safe in panic context)
  esp_err_t err = crash_log_store(reason, backtrace);
  if (err != ESP_OK)
  {
    // Can't do much here, just continue with default panic behavior
  }
}
#pragma GCC diagnostic pop

/**
 * @brief Check if the last reset was due to a crash and log it
 */
static void check_and_log_reset_reason(void)
{
  esp_reset_reason_t reset_reason = esp_reset_reason();

  // Only log if it was a crash-related reset
  if (reset_reason == ESP_RST_PANIC ||
      reset_reason == ESP_RST_WDT ||
      reset_reason == ESP_RST_TASK_WDT ||
      reset_reason == ESP_RST_BROWNOUT)
  {

    char reason[64];
    char backtrace[256] = "Post-reset analysis";

    switch (reset_reason)
    {
    case ESP_RST_PANIC:
      strncpy(reason, "Previous: Kernel panic", sizeof(reason) - 1);
      break;
    case ESP_RST_WDT:
      strncpy(reason, "Previous: Watchdog timeout", sizeof(reason) - 1);
      break;
    case ESP_RST_TASK_WDT:
      strncpy(reason, "Previous: Task watchdog", sizeof(reason) - 1);
      break;
    case ESP_RST_BROWNOUT:
      strncpy(reason, "Previous: Brownout reset", sizeof(reason) - 1);
      break;
    default:
      snprintf(reason, sizeof(reason), "Previous: Reset %d", reset_reason);
      break;
    }

    // Store this crash information
    crash_log_store(reason, backtrace);

    debug_log_warning_f(DEBUG_TAG_SYSTEM, "System recovered from crash: %s", reason);
  }
}

esp_err_t crash_handler_init(void)
{
  if (crash_handler_initialized)
  {
    return ESP_OK;
  }

  debug_log_startup(DEBUG_TAG_SYSTEM, "Crash Handler");

  // Initialize crash log manager first
  esp_err_t err = crash_log_manager_init();
  if (err != ESP_OK)
  {
    debug_log_error_f(DEBUG_TAG_SYSTEM, "Failed to initialize crash log manager: %s", esp_err_to_name(err));
    return err;
  }

  // Check if we're recovering from a crash
  check_and_log_reset_reason();

  // Note: ESP-IDF doesn't provide a clean way to register custom panic handlers
  // The crash detection will primarily work through reset reason analysis

  crash_handler_initialized = true;
  debug_log_info(DEBUG_TAG_SYSTEM, "Crash handler initialized");

  return ESP_OK;
}

void crash_handler_trigger_test(const char *reason)
{
  if (!crash_handler_initialized)
  {
    debug_log_error(DEBUG_TAG_SYSTEM, "Crash handler not initialized");
    return;
  }

  char test_reason[64];
  char test_backtrace[256];

  if (reason)
  {
    snprintf(test_reason, sizeof(test_reason), "TEST: %s", reason);
  }
  else
  {
    strncpy(test_reason, "TEST: Manual crash test", sizeof(test_reason) - 1);
  }

  strncpy(test_backtrace, "Test backtrace - no actual crash", sizeof(test_backtrace) - 1);

  // Store test crash log
  esp_err_t err = crash_log_store(test_reason, test_backtrace);
  if (err == ESP_OK)
  {
    debug_log_info_f(DEBUG_TAG_SYSTEM, "Test crash logged: %s", test_reason);
  }
  else
  {
    debug_log_error_f(DEBUG_TAG_SYSTEM, "Failed to log test crash: %s", esp_err_to_name(err));
  }
}

// Test crash trigger functions
void crash_handler_trigger_null_pointer(void)
{
  debug_log_info(DEBUG_TAG_SYSTEM, "Triggering null pointer dereference...");
  vTaskDelay(100 / portTICK_PERIOD_MS); // Give time for log to output

  volatile int *null_ptr = NULL;
  *null_ptr = 42; // This will cause a crash
}

// Separate recursive function for stack overflow (to avoid inline infinite recursion warning)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winfinite-recursion"
static void recursive_stack_overflow(void)
{
  volatile char big_buffer[2048];
  memset((void *)big_buffer, 0xAA, sizeof(big_buffer));
  recursive_stack_overflow(); // This will eventually overflow the stack
}
#pragma GCC diagnostic pop

void crash_handler_trigger_stack_overflow(void)
{
  debug_log_info(DEBUG_TAG_SYSTEM, "Triggering stack overflow...");
  vTaskDelay(100 / portTICK_PERIOD_MS);

  // Call the recursive function
  recursive_stack_overflow();
}

void crash_handler_trigger_heap_corruption(void)
{
  debug_log_info(DEBUG_TAG_SYSTEM, "Triggering heap corruption...");
  vTaskDelay(100 / portTICK_PERIOD_MS);

  // Allocate memory and corrupt it
  char *buffer = malloc(100);
  if (buffer)
  {
    // Write past the end of allocated memory
    memset(buffer + 100, 0xFF, 100); // Corrupt heap
    free(buffer);
  }

  // Try to allocate again to trigger corruption detection
  char *buffer2 = malloc(200);
  if (buffer2)
  {
    free(buffer2);
  }
}

void crash_handler_trigger_assert_fail(void)
{
  debug_log_info(DEBUG_TAG_SYSTEM, "Triggering assertion failure...");
  vTaskDelay(100 / portTICK_PERIOD_MS);

  assert(false); // This will trigger an assertion failure
}

void crash_handler_trigger_watchdog_timeout(void)
{
  debug_log_info(DEBUG_TAG_SYSTEM, "Triggering watchdog timeout...");
  vTaskDelay(100 / portTICK_PERIOD_MS);

  // Disable interrupts and loop forever to trigger watchdog
  portDISABLE_INTERRUPTS();
  while (1)
  {
    // Infinite loop with interrupts disabled
    // This will trigger the watchdog timeout
  }
}

void crash_handler_trigger_abort(void)
{
  debug_log_info(DEBUG_TAG_SYSTEM, "Triggering abort...");
  vTaskDelay(100 / portTICK_PERIOD_MS);

  abort(); // Direct abort call
}
