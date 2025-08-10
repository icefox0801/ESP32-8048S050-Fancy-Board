/**
 * @file ui_controls_panel.h
 * @brief Control Panel Header for Smart Home Integration
 *
 * Provides the control panel creation and management functions for smart home
 * switches and scene controls on the ESP32-S3-8048S050 system monitor dashboard.
 */

#pragma once

#include <stdbool.h>
#include "lvgl.h"

// Switch identifiers for generic functions
typedef enum
{
  SWITCH_A = 0,
  SWITCH_B = 1,
  SWITCH_C = 2,
  SWITCH_COUNT
} switch_id_t;

/**
 * @brief Create the control panel with smart home switches and scene button
 * @param parent Parent screen object
 * @return Created control panel object
 */
lv_obj_t *create_controls_panel(lv_obj_t *parent);

/**
 * @brief Update Home Assistant connection status in the controls panel
 * @param is_ready True if HA is ready, false otherwise
 * @param is_syncing True if HA is syncing, false otherwise
 * @param status_text HA status message to display
 */
void controls_panel_update_ha_status(bool is_ready, bool is_syncing, const char *status_text);

// =======================================================================
// SMART HOME CONTROL FUNCTIONS
// =======================================================================

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

// =======================================================================
// EVENT CALLBACK REGISTRATION (DECOUPLING)
// =======================================================================

/**
 * @brief Callback function type for switch control
 * @param entity_id Home Assistant entity ID
 * @param state True to turn on, false to turn off
 * @return ESP_OK on success, error code on failure
 */
typedef esp_err_t (*switch_control_callback_t)(const char *entity_id, bool state);

/**
 * @brief Callback function type for scene trigger
 * @return ESP_OK on success, error code on failure
 */
typedef esp_err_t (*scene_trigger_callback_t)(void);

/**
 * @brief Smart home callback structure for UI decoupling
 */
typedef struct
{
  switch_control_callback_t switch_callback; /**< Function to call when switch state changes */
  scene_trigger_callback_t scene_callback;   /**< Function to call when scene button is pressed */
} smart_home_callbacks_t;

/**
 * @brief Register event callbacks to decouple UI from smart home logic
 * @param callbacks Pointer to smart home callback structure
 */
void controls_panel_register_event_callbacks(const smart_home_callbacks_t *callbacks);
