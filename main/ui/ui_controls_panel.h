/**
 * @file ui_controls_panel.h
 * @brief Control Panel Header for Smart Home Integration
 *
 * Provides the control panel creation and management functions for smart home
 * switches and scene controls on the ESP32-S3-8048S050 system monitor dashboard.
 */

#pragma once

#include "lvgl.h"

// Switch identifiers for generic functions
typedef enum
{
  SWITCH_A = 0,
  SWITCH_B = 1,
  SWITCH_C = 2,
  SWITCH_COUNT
} switch_id_t;
#include <stdbool.h>

/**
 * @brief Create the control panel with smart home switches and scene button
 * @param parent Parent screen object
 * @return Created control panel object
 */
lv_obj_t *create_controls_panel(lv_obj_t *parent);

/**
 * @brief Update Home Assistant connection status in the controls panel
 * @param status_text HA status message to display
 * @param connected True if HA is connected, false otherwise
 */
void controls_panel_update_ha_status(const char *status_text, bool connected);

// ═══════════════════════════════════════════════════════════════════════════════
// SMART HOME CONTROL FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Set the state of any switch
 * @param switch_id Switch identifier (0=A, 1=B, 2=C)
 * @param state True to turn on, false to turn off
 */
void controls_panel_set_switch(int switch_id, bool state);

/**
 * @brief Get the state of any switch
 * @param switch_id Switch identifier (0=A, 1=B, 2=C)
 * @return True if on, false if off
 */
bool controls_panel_get_switch(int switch_id);
