/**
 * @file serial_data_handler.h
 * @brief Serial Data Handler for System Monitor Dashboard
 *
 * This module handles UART communication and JSON parsing for real-time
 * system monitoring data reception. Designed for ESP32-S3 with LVGL
 * graphics integration.
 *
 * @author ESP32 System Monitor
 * @version 1.0
 * @date 2024
 */

#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// STANDARD INCLUDES
// ═══════════════════════════════════════════════════════════════════════════════

#include "system_monitor_ui.h"
#include "esp_err.h"

// ═══════════════════════════════════════════════════════════════════════════════
// PUBLIC FUNCTION PROTOTYPES
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Initialize serial data receiver system
 * @return ESP_OK on successful initialization, error code otherwise
 * @note Sets up UART configuration and internal data structures
 */
esp_err_t serial_data_init(void);

/**
 * @brief Start serial data reception task
 * @note Creates FreeRTOS task for continuous data monitoring
 * @warning Ensure serial_data_init() is called first
 */
void serial_data_start_task(void);

/**
 * @brief Stop serial data reception and cleanup resources
 * @note Terminates reception task and frees allocated memory
 */
void serial_data_stop(void);
