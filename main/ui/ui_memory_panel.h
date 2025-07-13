/**
 * @file ui_memory_panel.h
 * @brief Memory Monitoring Panel Header
 *
 * Provides the memory monitoring panel creation function for the
 * ESP32-S3-8048S050 system monitor dashboard.
 */

#pragma once

#include "lvgl.h"

// Memory monitoring data structure
struct memory_info
{
  uint8_t usage; ///< Memory usage percentage (0-100)
  float used;    ///< Memory used (GB)
  float total;   ///< Memory total (GB)
  float avail;   ///< Memory available (GB)
};

/**
 * @brief Create the memory monitoring panel
 * @param parent Parent screen object
 * @return Created memory panel object
 */
lv_obj_t *create_memory_panel(lv_obj_t *parent);

/**
 * @brief Update memory panel with new data
 * @param memory_data Pointer to memory monitoring data
 */
void update_memory_panel(const void *memory_data);
