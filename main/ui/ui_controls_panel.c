/**
 * @file ui_controls_panel.c
 * @brief Control Panel Implementation for Smart Home Integration
 *
 * Creates the control panel with smart home switches and scene controls
 * for the ESP32-S3-8048S050 system monitor dashboard.
 */

#include "ui_controls_panel.h"
#include "ui_config.h"
#include "ui_helpers.h"
#include "lvgl_setup.h"
#include "smart/smart_home.h"
#include "smart/smart_config.h"
#include "system_debug_utils.h"
#include <stdio.h>

static lv_obj_t *switch_a = NULL;
static lv_obj_t *switch_b = NULL;
static lv_obj_t *switch_c = NULL;
static lv_obj_t *scene_button = NULL;
static lv_obj_t *ha_status_label = NULL;

// ══════════════════════════════════════════════════════════════════════════════
// SWITCH ENUMERATION AND HELPER TYPES
// ══════════════════════════════════════════════════════════════════════════════

typedef struct
{
  lv_obj_t **switch_obj;
  const char *label;
  const char *entity;
} switch_config_t;

// Switch configuration table
static const switch_config_t switch_configs[3] = {
    {&switch_a, UI_CONTROLS_LABEL_A, HA_ENTITY_A_ID},
    {&switch_b, UI_CONTROLS_LABEL_B, HA_ENTITY_B_ID},
    {&switch_c, UI_CONTROLS_LABEL_C, HA_ENTITY_C_ID}};

// ══════════════════════════════════════════════════════════════════════════════
// LOCAL EVENT HANDLERS
// ══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Debug touch handler for logging UI touch events
 */
static void debug_touch_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);

  // Simplified touch debug - only log important events
  if (code == LV_EVENT_CLICKED)
  {
    debug_log_debug(DEBUG_TAG_UI_CONTROLS, "Control pressed");
  }
}

/**
 * @brief Generic switch event handler
 * @param e LVGL event object
 */
static void switch_event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);

  // Find which switch was triggered
  switch_id_t switch_id = SWITCH_A;
  for (int i = 0; i < 3; i++)
  {
    if (obj == *switch_configs[i].switch_obj)
    {
      switch_id = i;
      break;
    }
  }

  const switch_config_t *config = &switch_configs[switch_id];

  if (code == LV_EVENT_VALUE_CHANGED)
  {
    bool state = lv_obj_has_state(obj, LV_STATE_CHECKED);
    debug_log_debug(DEBUG_TAG_UI_CONTROLS, "Switch state changed");

    // Control the actual device via Home Assistant
    esp_err_t ret = smart_home_control_switch(config->entity, state);
    if (ret != ESP_OK)
    {
      debug_log_error_f(DEBUG_TAG_UI_CONTROLS, "Switch %s control failed: %s", config->label, esp_err_to_name(ret));
      // TODO: Consider reverting the switch state on failure
    }
  }
}

/**
 * @brief Scene button event handler
 */
static void scene_button_event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);

  debug_log_debug(DEBUG_TAG_UI_CONTROLS, "Scene button event");

  if (code == LV_EVENT_CLICKED)
  {
    debug_log_info(DEBUG_TAG_UI_CONTROLS, "Scene button pressed");

    // Trigger the scene via Home Assistant
    esp_err_t ret = smart_home_trigger_scene();
    if (ret != ESP_OK)
    {
      debug_log_error_f(DEBUG_TAG_UI_CONTROLS, "Scene trigger failed: %s", esp_err_to_name(ret));
    }
    else
    {
      debug_log_info(DEBUG_TAG_UI_CONTROLS, "Scene triggered successfully");
    }
  }
}

/**
 * @brief Create the control panel with smart home switches and scene button
 * @param parent Parent screen object
 * @return Created control panel
 */
lv_obj_t *create_controls_panel(lv_obj_t *parent)
{
  lv_obj_t *control_panel = ui_create_panel(parent, 780, 100, 10, 10, 0x1a1a2e, 0x2e2e4a);

  // Controls title (moved up)
  lv_obj_t *controls_title = lv_label_create(control_panel);
  lv_label_set_text(controls_title, "Controls");
  lv_obj_set_style_text_font(controls_title, font_title, 0);
  lv_obj_set_style_text_color(controls_title, lv_color_hex(0x4fc3f7), 0);
  lv_obj_align(controls_title, LV_ALIGN_TOP_LEFT, 0, 5);

  // HA status text below title
  ha_status_label = lv_label_create(control_panel);
  lv_label_set_text(ha_status_label, "HA: Connecting...");
  lv_obj_set_style_text_font(ha_status_label, font_small, 0);
  lv_obj_set_style_text_color(ha_status_label, lv_color_hex(0x888888), 0);
  lv_obj_align(ha_status_label, LV_ALIGN_TOP_LEFT, 0, 40);

  // Better spacing layout: Title section (140px) + 3 switches (120px each) + separators + button section
  // Total: 140 + 120*3 + 4*10 + 120 = 140 + 360 + 40 + 120 = 660px (fits in 780px panel)

  // Vertical separator after controls title
  ui_create_centered_vertical_separator(control_panel, 140, 60, 0x4fc3f7);

  // Switch A field - positioned with better spacing
  switch_a = ui_create_switch_field(control_panel, UI_CONTROLS_LABEL_A, 160);
  lv_obj_add_event_cb(switch_a, debug_touch_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(switch_a, switch_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(switch_a, switch_event_handler, LV_EVENT_CLICKED, NULL);
  debug_log_debug(DEBUG_TAG_UI_CONTROLS, "Switch A created");

  // Vertical separator after switch A
  ui_create_centered_vertical_separator(control_panel, 280, 60, 0x555555);

  // Switch B field - proper spacing from separator
  switch_b = ui_create_switch_field(control_panel, UI_CONTROLS_LABEL_B, 300);
  lv_obj_add_event_cb(switch_b, debug_touch_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(switch_b, switch_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(switch_b, switch_event_handler, LV_EVENT_CLICKED, NULL);
  debug_log_debug(DEBUG_TAG_UI_CONTROLS, "Switch B created");

  // Vertical separator after switch B
  ui_create_centered_vertical_separator(control_panel, 420, 60, 0x555555);

  // Switch C field - proper spacing from separator
  switch_c = ui_create_switch_field(control_panel, UI_CONTROLS_LABEL_C, 440);
  lv_obj_add_event_cb(switch_c, debug_touch_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(switch_c, switch_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(switch_c, switch_event_handler, LV_EVENT_CLICKED, NULL);
  debug_log_debug(DEBUG_TAG_UI_CONTROLS, "Switch C created");

  // Vertical separator before scene button
  ui_create_centered_vertical_separator(control_panel, 560, 60, 0x555555);

  // Scene button (right with proper padding, centered using align API)
  scene_button = lv_btn_create(control_panel);
  lv_obj_set_size(scene_button, 120, 50);
  lv_obj_align(scene_button, LV_ALIGN_RIGHT_MID, -20, 0); // 20px margin from right edge
  lv_obj_set_style_bg_color(scene_button, lv_color_hex(0x4caf50), 0);
  lv_obj_set_style_radius(scene_button, 10, 0);
  lv_obj_add_event_cb(scene_button, debug_touch_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(scene_button, scene_button_event_handler, LV_EVENT_CLICKED, NULL);

  lv_obj_t *scene_label = lv_label_create(scene_button);
  lv_label_set_text(scene_label, UI_CONTROLS_LABEL_D);
  lv_obj_set_style_text_font(scene_label, font_normal, 0);
  lv_obj_set_style_text_color(scene_label, lv_color_hex(0xffffff), 0);
  lv_obj_center(scene_label);
  return control_panel;
}

/**
 * @brief Generic function to set switch state
 * @param switch_id Which switch to control (SWITCH_A, SWITCH_B, SWITCH_C)
 * @param state True to turn on, false to turn off
 */
void controls_panel_set_switch(int switch_id, bool state)
{
  if (switch_id < 0 || switch_id >= SWITCH_COUNT || !*switch_configs[switch_id].switch_obj)
    return;

  lvgl_lock_acquire();
  if (state)
    lv_obj_add_state(*switch_configs[switch_id].switch_obj, LV_STATE_CHECKED);
  else
    lv_obj_clear_state(*switch_configs[switch_id].switch_obj, LV_STATE_CHECKED);
  lvgl_lock_release();
}

/**
 * @brief Generic function to get switch state
 * @param switch_id Which switch to query (SWITCH_A, SWITCH_B, SWITCH_C)
 * @return True if on, false if off
 */
bool controls_panel_get_switch(int switch_id)
{
  if (switch_id < 0 || switch_id >= SWITCH_COUNT || !*switch_configs[switch_id].switch_obj)
    return false;

  bool state = false;
  lvgl_lock_acquire();
  state = lv_obj_has_state(*switch_configs[switch_id].switch_obj, LV_STATE_CHECKED);
  lvgl_lock_release();
  return state;
}

/**
 * @brief Update Home Assistant connection status in the controls panel
 * @param status_text HA status message to display
 * @param connected True if HA is connected, false otherwise
 */
void controls_panel_update_ha_status(const char *status_text, bool connected)
{
  if (!ha_status_label || !status_text)
    return;

  lvgl_lock_acquire();

  // Create formatted status message (keep it short)
  char ha_msg[32];
  snprintf(ha_msg, sizeof(ha_msg), "HA: %s", status_text);

  lv_label_set_text(ha_status_label, ha_msg);

  // Set color based on connection status
  if (connected)
  {
    lv_obj_set_style_text_color(ha_status_label, lv_color_hex(0x00ff88), 0); // Green
  }
  else
  {
    lv_obj_set_style_text_color(ha_status_label, lv_color_hex(0xff4444), 0); // Red
  }

  lvgl_lock_release();
}
