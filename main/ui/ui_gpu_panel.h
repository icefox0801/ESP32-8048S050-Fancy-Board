/**
 * @file ui_gpu_panel.h
 * @brief GPU Monitoring Panel Header
 *
 * Provides the GPU monitoring panel creation function for the
 * ESP32-S3-8048S050 system monitor dashboard.
 */

#pragma once

#include "lvgl.h"
#include "common_types.h"

/**
 * @brief Create the GPU monitoring panel
 * @param parent Parent screen object
 * @return Created GPU panel object
 */
lv_obj_t *create_gpu_panel(lv_obj_t *parent);

/**
 * @brief Update GPU panel with new data
 * @param gpu_data Pointer to GPU monitoring data
 */
void update_gpu_panel(const void *gpu_data);
