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
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "ui_controls_panel";

// External references to global UI elements
extern lv_obj_t *switch_a;
extern lv_obj_t *switch_b;
extern lv_obj_t *switch_c;
extern lv_obj_t *scene_button;
extern lv_obj_t *ha_status_label;

// ══════════════════════════════════════════════════════════════════════════════
// SWITCH ENUMERATION AND HELPER TYPES
// ══════════════════════════════════════════════════════════════════════════════

typedef struct
{
  lv_obj_t **switch_obj;
  const char *label;
  const char *entity;
  const char *icon;
} switch_config_t;

// Switch configuration table
static const switch_config_t switch_configs[3] = {
    {&switch_a, UI_LABEL_A, HA_ENTITY_A, "💡"},
    {&switch_b, UI_LABEL_B, HA_ENTITY_B, "🔌"},
    {&switch_c, UI_LABEL_C, HA_ENTITY_C, "💡"}};

// ══════════════════════════════════════════════════════════════════════════════
// LOCAL EVENT HANDLERS
// ══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Debug touch handler for logging UI touch events
 */
static void debug_touch_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);

  // Get object position and size for debugging
  lv_area_t coords;
  lv_obj_get_coords(obj, &coords);

  const char *obj_name = "Unknown";
  if (obj == switch_a)
    obj_name = UI_LABEL_A;
  else if (obj == switch_b)
    obj_name = UI_LABEL_B;
  else if (obj == switch_c)
    obj_name = UI_LABEL_C;
  else if (obj == scene_button)
    obj_name = UI_LABEL_D;

  ESP_LOGI(TAG, "Touch debug - %s switch: code=%d, coords=(%d,%d)-(%d,%d)",
           obj_name, code, coords.x1, coords.y1, coords.x2, coords.y2);
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

  ESP_LOGI(TAG, "%s SWITCH %s (%s): Event handler called with code %d",
           config->icon, config->label, config->label, code);

  if (code == LV_EVENT_VALUE_CHANGED)
  {
    bool state = lv_obj_has_state(obj, LV_STATE_CHECKED);
    ESP_LOGI(TAG, "%s SWITCH %s (%s) TOUCH EVENT: User selected %s",
             config->icon, config->label, config->label, state ? "ON" : "OFF");

    // Control the actual device via Home Assistant
    esp_err_t ret = smart_home_control_switch(config->entity, state);
    if (ret != ESP_OK)
    {
      ESP_LOGE(TAG, "%s SWITCH %s (%s) FAILED: %s",
               config->icon, config->label, config->label, esp_err_to_name(ret));
      // TODO: Consider reverting the switch state on failure
    }
    else
    {
      ESP_LOGI(TAG, "%s SWITCH %s (%s) SUCCESS: Device state changed to %s",
               config->icon, config->label, config->label, state ? "ON" : "OFF");
    }
  }
}

/**
 * @brief Scene button event handler
 */
static void scene_button_event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);

  ESP_LOGI(TAG, "🎬 SCENE BUTTON (%s): Event handler called with code %d", UI_LABEL_D, code);

  if (code == LV_EVENT_CLICKED)
  {
    ESP_LOGI(TAG, "🎬 SCENE BUTTON (%s) PRESSED", UI_LABEL_D);

    // Trigger the scene via Home Assistant
    esp_err_t ret = smart_home_trigger_scene();
    if (ret != ESP_OK)
    {
      ESP_LOGE(TAG, "🎬 SCENE BUTTON (%s) FAILED: %s", UI_LABEL_D, esp_err_to_name(ret));
    }
    else
    {
      ESP_LOGI(TAG, "🎬 SCENE BUTTON (%s) SUCCESS: Scene triggered", UI_LABEL_D);
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
  switch_a = ui_create_switch_field(control_panel, UI_LABEL_A, 160);
  lv_obj_add_event_cb(switch_a, debug_touch_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(switch_a, switch_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(switch_a, switch_event_handler, LV_EVENT_CLICKED, NULL);
  ESP_LOGI(TAG, "Switch A (%s) created at x=160, better spacing", UI_LABEL_A);

  // Vertical separator after switch A
  ui_create_centered_vertical_separator(control_panel, 280, 60, 0x555555);

  // Switch B field - proper spacing from separator
  switch_b = ui_create_switch_field(control_panel, UI_LABEL_B, 300);
  lv_obj_add_event_cb(switch_b, debug_touch_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(switch_b, switch_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(switch_b, switch_event_handler, LV_EVENT_CLICKED, NULL);
  ESP_LOGI(TAG, "Switch B (%s) created at x=300, better spacing", UI_LABEL_B);

  // Vertical separator after switch B
  ui_create_centered_vertical_separator(control_panel, 420, 60, 0x555555);

  // Switch C field - proper spacing from separator
  switch_c = ui_create_switch_field(control_panel, UI_LABEL_C, 440);
  lv_obj_add_event_cb(switch_c, debug_touch_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(switch_c, switch_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(switch_c, switch_event_handler, LV_EVENT_CLICKED, NULL);
  ESP_LOGI(TAG, "Switch C (%s) created at x=440, better spacing", UI_LABEL_C);

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
  lv_label_set_text(scene_label, UI_LABEL_D);
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
