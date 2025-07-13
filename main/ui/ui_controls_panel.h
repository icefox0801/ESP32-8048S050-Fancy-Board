/**
 * @file ui_controls_panel.h
 * @brief Control Panel Header for Smart Home Integration
 *
 * Provides the control panel creation and management functions for smart home
 * switches and scene controls on the ESP32-S3-8048S050 system monitor dashboard.
 */

#pragma once

#include "lvgl.h"
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
 * @brief Set the state of switch A
 * @param state True to turn on, false to turn off
 */
void controls_panel_set_switch_a(bool state);

/**
 * @brief Set the state of switch B
 * @param state True to turn on, false to turn off
 */
void controls_panel_set_switch_b(bool state);

/**
 * @brief Set the state of switch C
 * @param state True to turn on, false to turn off
 */
void controls_panel_set_switch_c(bool state);

/**
 * @brief Get the state of switch A
 * @return True if on, false if off
 */
bool controls_panel_get_switch_a(void);

/**
 * @brief Get the state of switch B
 * @return True if on, false if off
 */
bool controls_panel_get_switch_b(void);

/**
 * @brief Get the state of switch C
 * @return True if on, false if off
 */
bool controls_panel_get_switch_c(void);
