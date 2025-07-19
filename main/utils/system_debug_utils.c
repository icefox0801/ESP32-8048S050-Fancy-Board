/**
 * @file system_debug_utils.c
 * @brief Minimal Debug Utilities
 *
 * Minimal debug functions controlled by CONFIG_SYSTEM_DEBUG_ENABLED.
 * Only essential logging with simplified implementation.
 */

#include "system_debug_utils.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <stdarg.h>
#include <stdio.h>

#ifdef CONFIG_SYSTEM_DEBUG_ENABLED

// Array of tag strings corresponding to debug_tag_t enum
static const char *debug_tag_strings[DEBUG_TAG_MAX] = {
    "dashboard",
    "serial_data",
    "WiFi_Manager",
    "SmartHome",
    "ui_dashboard",
    "ui_controls_panel",
    "gt911_touch",
    "ha_task_mgr",
    "HA_SYNC",
    "HA_API",
    "lvgl_setup",
    "SYSTEM"};

void debug_log_startup(debug_tag_t tag, const char *component_name)
{
  if (tag < DEBUG_TAG_MAX && component_name)
  {
    ESP_LOGI(debug_tag_strings[tag], "%s started", component_name);
  }
}

void debug_log_error(debug_tag_t tag, const char *error_msg)
{
  if (tag < DEBUG_TAG_MAX && error_msg)
  {
    ESP_LOGE(debug_tag_strings[tag], "%s", error_msg);
  }
}

void debug_log_event(debug_tag_t tag, const char *event_msg)
{
  if (tag < DEBUG_TAG_MAX && event_msg)
  {
    ESP_LOGI(debug_tag_strings[tag], "%s", event_msg);
  }
}

void debug_check_task_health(debug_tag_t tag)
{
  UBaseType_t current_stack = uxTaskGetStackHighWaterMark(NULL);
  size_t free_heap = esp_get_free_heap_size();

  if (current_stack < 512)
  {
    debug_log_error(tag, "Low stack detected");
  }

  if (free_heap < 50000)
  {
    debug_log_error(tag, "Low heap detected");
  }
}

bool debug_check_heap_sufficient(debug_tag_t tag, size_t required_bytes)
{
  size_t free_heap = esp_get_free_heap_size();
  if (free_heap < required_bytes)
  {
    debug_log_error(tag, "Insufficient heap for operation");
    return false;
  }
  return true;
}

void debug_print_memory_usage(debug_tag_t tag, void *task_handle)
{
  if (tag >= DEBUG_TAG_MAX)
    return;

  size_t free_heap = esp_get_free_heap_size();
  size_t min_heap = esp_get_minimum_free_heap_size();

  ESP_LOGI(debug_tag_strings[tag], "Free heap: %zu bytes, Min heap: %zu bytes", free_heap, min_heap);

  if (task_handle != NULL)
  {
    UBaseType_t stack_hwm = uxTaskGetStackHighWaterMark((TaskHandle_t)task_handle);
    ESP_LOGI(debug_tag_strings[tag], "Task stack HWM: %u bytes", (unsigned int)(stack_hwm * sizeof(StackType_t)));
  }
}

void debug_log_info(debug_tag_t tag, const char *info_msg)
{
  if (tag < DEBUG_TAG_MAX && info_msg)
  {
    ESP_LOGI(debug_tag_strings[tag], "%s", info_msg);
  }
}

void debug_log_warning(debug_tag_t tag, const char *warning_msg)
{
  if (tag < DEBUG_TAG_MAX && warning_msg)
  {
    ESP_LOGW(debug_tag_strings[tag], "%s", warning_msg);
  }
}

void debug_log_debug(debug_tag_t tag, const char *debug_msg)
{
  if (tag < DEBUG_TAG_MAX && debug_msg)
  {
    ESP_LOGD(debug_tag_strings[tag], "%s", debug_msg);
  }
}

void debug_log_info_f(debug_tag_t tag, const char *format, ...)
{
  if (tag >= DEBUG_TAG_MAX || !format)
    return;

  va_list args;
  va_start(args, format);
  esp_log_writev(ESP_LOG_INFO, debug_tag_strings[tag], format, args);
  va_end(args);
}

void debug_log_error_f(debug_tag_t tag, const char *format, ...)
{
  if (tag >= DEBUG_TAG_MAX || !format)
    return;

  va_list args;
  va_start(args, format);
  esp_log_writev(ESP_LOG_ERROR, debug_tag_strings[tag], format, args);
  va_end(args);
}

void debug_log_warning_f(debug_tag_t tag, const char *format, ...)
{
  if (tag >= DEBUG_TAG_MAX || !format)
    return;

  va_list args;
  va_start(args, format);
  esp_log_writev(ESP_LOG_WARN, debug_tag_strings[tag], format, args);
  va_end(args);
}

void debug_log_debug_f(debug_tag_t tag, const char *format, ...)
{
  if (tag >= DEBUG_TAG_MAX || !format)
    return;

  va_list args;
  va_start(args, format);
  esp_log_writev(ESP_LOG_DEBUG, debug_tag_strings[tag], format, args);
  va_end(args);
}

#else

// Empty implementations when debug is disabled
void debug_log_startup(debug_tag_t tag, const char *component_name)
{
  (void)tag;
  (void)component_name;
}

void debug_log_error(debug_tag_t tag, const char *error_msg)
{
  (void)tag;
  (void)error_msg;
}

void debug_log_event(debug_tag_t tag, const char *event_msg)
{
  (void)tag;
  (void)event_msg;
}

void debug_check_task_health(debug_tag_t tag)
{
  (void)tag;
}

bool debug_check_heap_sufficient(debug_tag_t tag, size_t required_bytes)
{
  (void)tag;
  (void)required_bytes;
  return true; // Always return true when debug is disabled
}

void debug_print_memory_usage(debug_tag_t tag, void *task_handle)
{
  (void)tag;
  (void)task_handle;
}

void debug_log_info(debug_tag_t tag, const char *info_msg)
{
  (void)tag;
  (void)info_msg;
}

void debug_log_warning(debug_tag_t tag, const char *warning_msg)
{
  (void)tag;
  (void)warning_msg;
}

void debug_log_debug(debug_tag_t tag, const char *debug_msg)
{
  (void)tag;
  (void)debug_msg;
}

void debug_log_info_f(debug_tag_t tag, const char *format, ...)
{
  (void)tag;
  (void)format;
}

void debug_log_error_f(debug_tag_t tag, const char *format, ...)
{
  (void)tag;
  (void)format;
}

void debug_log_warning_f(debug_tag_t tag, const char *format, ...)
{
  (void)tag;
  (void)format;
}

void debug_log_debug_f(debug_tag_t tag, const char *format, ...)
{
  (void)tag;
  (void)format;
}

#endif
