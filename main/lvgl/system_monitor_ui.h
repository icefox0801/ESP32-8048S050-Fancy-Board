/**
 * @file system_monitor_ui.h
 * @brief System Monitor Dashboard UI Header
 *
 * Defines the system monitoring UI interface for displaying real-time
 * CPU, GPU, and memory statistics on ESP32-S3-8048S050 LCD display.
 *
 * Features:
 * - Real-time system metrics display
 * - JSON data parsing from serial input
 * - Beautiful progress bars and gauges
 * - Connection status monitoring
 * - Professional dashboard layout
 */

#pragma once

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

// ═══════════════════════════════════════════════════════════════════════════════
// DATA STRUCTURES
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief System monitoring data structure
 *
 * Contains all system metrics received via JSON from serial port:
 * - Timestamp for data freshness tracking
 * - CPU usage, temperature, and identification
 * - GPU usage, temperature, memory, and identification
 * - System memory usage and availability
 */
typedef struct
{
  // Timestamp (milliseconds since epoch)
  uint64_t timestamp;

  // CPU Information Section
  struct
  {
    uint8_t usage; ///< CPU usage percentage (0-100)
    uint8_t temp;  ///< CPU temperature in Celsius
    uint16_t fan;  ///< CPU fan speed in RPM
    char name[32]; ///< CPU name/model string
  } cpu;

  // GPU Information Section
  struct
  {
    uint8_t usage;      ///< GPU usage percentage (0-100)
    uint8_t temp;       ///< GPU temperature in Celsius
    char name[32];      ///< GPU name/model string
    uint32_t mem_used;  ///< GPU memory used (MB)
    uint32_t mem_total; ///< GPU memory total (MB)
  } gpu;

  // System Memory Information Section
  struct
  {
    uint8_t usage; ///< Memory usage percentage (0-100)
    float used;    ///< Memory used (GB)
    float total;   ///< Memory total (GB)
    float avail;   ///< Memory available (GB)
  } mem;
} system_data_t;

// ═══════════════════════════════════════════════════════════════════════════════
// PUBLIC FUNCTION PROTOTYPES
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Create system monitor UI
 * @param disp LVGL display handle
 */
void system_monitor_ui_create(lv_display_t *disp);

/**
 * @brief Update system monitor display with new data
 * @param data System monitoring data
 */
void system_monitor_ui_update(const system_data_t *data);

/**
 * @brief Update connection status
 * @param connected True if receiving data, false if connection lost
 */
void system_monitor_ui_set_connection_status(bool connected);

/**
 * @brief Update WiFi connection status display
 * @param status_text WiFi status message to display
 * @param connected True if WiFi is connected, false otherwise
 */
void system_monitor_ui_update_wifi_status(const char *status_text, bool connected);

// ═══════════════════════════════════════════════════════════════════════════════
// SMART HOME CONTROL FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Set the state of the water pump switch
 * @param state True to turn on, false to turn off
 */
void system_monitor_ui_set_water_pump(bool state);

/**
 * @brief Set the state of the wave maker switch
 * @param state True to turn on, false to turn off
 */
void system_monitor_ui_set_wave_maker(bool state);

/**
 * @brief Set the state of the light switch
 * @param state True to turn on, false to turn off
 */
void system_monitor_ui_set_light(bool state);

/**
 * @brief Get the state of the water pump switch
 * @return True if on, false if off
 */
bool system_monitor_ui_get_water_pump(void);

/**
 * @brief Get the state of the wave maker switch
 * @return True if on, false if off
 */
bool system_monitor_ui_get_wave_maker(void);

/**
 * @brief Get the state of the light switch
 * @return True if on, false if off
 */
bool system_monitor_ui_get_light(void);
