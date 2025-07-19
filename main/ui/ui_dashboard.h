/**
 * @file ui_dashboard.h
 * @brief Dashboard UI Header
 *
 * Defines the dashboard UI interface for displaying real-time
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
#include "dashboard_data.h"
#include "ui_controls_panel.h"

// Include panel headers for LVGL UI functions only
#include "ui_cpu_panel.h"
#include "ui_gpu_panel.h"
#include "ui_memory_panel.h"
#include "ui_status_info.h"

// ═══════════════════════════════════════════════════════════════════════════════
// PUBLIC FUNCTION PROTOTYPES
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Create dashboard UI
 * @param disp LVGL display handle
 */
void ui_dashboard_create(lv_display_t *disp);

/**
 * @brief Update dashboard display with new data
 * @param data System monitoring data
 */
void ui_dashboard_update(const system_data_t *data);

/**
 * @brief Update connection status
 * @param connected True if receiving data, false if connection lost
 */
void ui_dashboard_set_connection_status(bool connected);

/**
 * @brief Update WiFi connection status display
 * @param status_text WiFi status message to display
 * @param connected True if WiFi is connected, false otherwise
 */
void ui_dashboard_update_wifi_status(const char *status_text, bool connected);
