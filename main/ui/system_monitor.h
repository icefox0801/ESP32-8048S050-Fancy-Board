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
#include "ui_controls_panel.h"

// Include panel headers for data structure definitions
#include "ui_cpu_panel.h"
#include "ui_gpu_panel.h"
#include "ui_memory_panel.h"
#include "ui_status_info.h"

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

  // CPU Information Section - uses struct from ui_cpu_panel.h
  struct cpu_info cpu;

  // GPU Information Section - uses struct from ui_gpu_panel.h
  struct gpu_info gpu;

  // System Memory Information Section - uses struct from ui_memory_panel.h
  struct memory_info mem;
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
