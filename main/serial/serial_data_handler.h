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

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
#include "dashboard_data.h"

// ═══════════════════════════════════════════════════════════════════════════════
// CALLBACK FUNCTION TYPES
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Callback function type for connection status changes
 * @param connected True if connection is active, false if lost
 */
typedef void (*serial_connection_callback_t)(bool connected);

/**
 * @brief Callback function type for data updates
 * @param data Pointer to system data structure with new monitoring data
 */
typedef void (*serial_data_callback_t)(const system_data_t *data);

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

/**
 * @brief Register callback for connection status changes
 * @param callback Function to call when connection status changes
 * @note Pass NULL to unregister the callback
 */
void serial_data_register_connection_callback(serial_connection_callback_t callback);

/**
 * @brief Register callback for data updates
 * @param callback Function to call when new monitoring data arrives
 * @note Pass NULL to unregister the callback
 */
void serial_data_register_data_callback(serial_data_callback_t callback);
