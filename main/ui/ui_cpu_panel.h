/**
 * @file ui_cpu_panel.h
 * @brief CPU Monitoring Panel Header
 *
 * Provides the CPU monitoring panel creation function for the
 * ESP32-S3-8048S050 system monitor dashboard.
 */

#pragma once

#include "lvgl.h"
#include "dashboard_data.h"

/**
 * @brief Create the CPU monitoring panel
 * @param parent Parent screen object
 * @return Created CPU panel object
 */
lv_obj_t *create_cpu_panel(lv_obj_t *parent);

/**
 * @brief Update CPU panel with new data
 * @param cpu_data Pointer to CPU monitoring data
 */
void update_cpu_panel(const void *cpu_data);

/**
 * @brief Reset CPU panel to default values (no connection)
 */
void reset_cpu_panel(void);
