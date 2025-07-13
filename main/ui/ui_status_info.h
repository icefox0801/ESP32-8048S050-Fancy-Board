/**
 * @file ui_status_info.h
 * @brief Status Information Panel Header
 *
 * Provides the status information panel creation and management functions
 * for the ESP32-S3-8048S050 system monitor dashboard.
 */

#pragma once

#include "lvgl.h"
#include <stdbool.h>

/**
 * @brief Create the status information panel
 * @param parent Parent screen object
 * @return Created status panel object
 */
lv_obj_t *create_status_info_panel(lv_obj_t *parent);

/**
 * @brief Update connection status indicator
 * @param connected True if connection is active, false if lost
 * @note Changes color and text of status indicator based on connection state
 */
void status_info_set_connection_status(bool connected);

/**
 * @brief Update WiFi connection status in the status panel
 * @param status_text WiFi status message to display
 * @param connected True if WiFi is connected, false otherwise
 */
void status_info_update_wifi_status(const char *status_text, bool connected);

/**
 * @brief Update serial connection status in the status panel
 * @param status_text Serial status message to display
 * @param connected True if serial is connected, false otherwise
 */
void status_info_update_serial_status(const char *status_text, bool connected);

/**
 * @brief Update timestamp in the status panel
 * @param timestamp Timestamp in milliseconds since epoch
 */
void status_info_update_timestamp(uint64_t timestamp);
