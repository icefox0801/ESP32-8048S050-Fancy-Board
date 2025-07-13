/**
 * @file system_monitor_ui.c
 * @brief System Monitor Dashboard UI for ESP32-S3-8048S050
 *
 * Creates a clean, spacious real-time system monitoring dashboard with:
 * - CPU Panel (Left): Usage, Temperature, Fan Speed
 * - GPU Panel (Center): Usage, Memory, Temperature
 * - Memory Panel (Right): System Memory Usage
 *
 * Features improved layout with:
 * - Larger fonts for better readability
 * - Proper vertical alignment between labels and values
 * - Equal padding for all panels
 * - Increased panel heights for better spacing
 * - Clean design without emoji icons
 */

#include "system_monitor.h"

#include "esp_log.h"
#include "lvgl_setup.h"
#include "ui_config.h"
#include "ui_helpers.h"
#include "ui_cpu_panel.h"
#include "ui_gpu_panel.h"
#include "ui_memory_panel.h"
#include "ui_status_info.h"
#include "smart/ha_api.h"
#include "smart/smart_home.h"
#include "smart/smart_config.h"
#include <stdio.h>
#include <time.h>

static const char *TAG = "system_monitor";

// ══════════════════════════════════════════════════════════════════════════════�?
// UI ELEMENT HANDLES FOR REAL-TIME UPDATES
// ══════════════════════════════════════════════════════════════════════════════�?

// Status and Info Elements
lv_obj_t *timestamp_label = NULL;
lv_obj_t *connection_status_label = NULL;
lv_obj_t *wifi_status_label = NULL;

// CPU Panel Elements
lv_obj_t *cpu_name_label = NULL;
lv_obj_t *cpu_usage_label = NULL;
lv_obj_t *cpu_temp_label = NULL;
lv_obj_t *cpu_fan_label = NULL;

// GPU Panel Elements
lv_obj_t *gpu_name_label = NULL;
lv_obj_t *gpu_usage_label = NULL;
lv_obj_t *gpu_temp_label = NULL;
lv_obj_t *gpu_mem_label = NULL;

// Memory Panel Elements
lv_obj_t *mem_usage_bar = NULL;
lv_obj_t *mem_usage_label = NULL;
lv_obj_t *mem_info_label = NULL;

// Smart Panel Elements
static lv_obj_t *switch_a = NULL;
static lv_obj_t *switch_b = NULL;
static lv_obj_t *switch_c = NULL;
static lv_obj_t *scene_button = NULL;
static lv_obj_t *ha_status_label = NULL;

// ══════════════════════════════════════════════════════════════════════════════�?
// EVENT HANDLERS - USING SYSTEM MANAGER TO PREVENT LVGL BLOCKING
// ══════════════════════════════════════════════════════════════════════════════�?

/**
 * @brief Generic touch debug handler for all switches
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
 * @brief Switch A event handler
 */
static void switch_a_event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);

  ESP_LOGI(TAG, "�?SWITCH A (%s): Event handler called with code %d", UI_LABEL_A, code);

  if (code == LV_EVENT_VALUE_CHANGED)
  {
    bool state = lv_obj_has_state(obj, LV_STATE_CHECKED);
    ESP_LOGI(TAG, "�?SWITCH A (%s) TOUCH EVENT: User selected %s", UI_LABEL_A, state ? "ON" : "OFF");

    // Control the actual device via Home Assistant
    esp_err_t ret = smart_home_control_switch(HA_ENTITY_A, state);
    if (ret != ESP_OK)
    {
      ESP_LOGE(TAG, "�?SWITCH A (%s) FAILED: %s", UI_LABEL_A, esp_err_to_name(ret));
      // TODO: Consider reverting the switch state on failure
    }
    else
    {
      ESP_LOGI(TAG, "�?SWITCH A (%s) SUCCESS: Device state changed to %s", UI_LABEL_A, state ? "ON" : "OFF");
    }
  }
}

/**
 * @brief Switch B event handler
 */
static void switch_b_event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);

  ESP_LOGI(TAG, "🔌 SWITCH B (%s): Event handler called with code %d", UI_LABEL_B, code);

  if (code == LV_EVENT_VALUE_CHANGED)
  {
    bool state = lv_obj_has_state(obj, LV_STATE_CHECKED);
    ESP_LOGI(TAG, "🔌 SWITCH B (%s) TOUCH EVENT: User selected %s", UI_LABEL_B, state ? "ON" : "OFF");

    // Control the actual device via Home Assistant
    esp_err_t ret = smart_home_control_switch(HA_ENTITY_B, state);
    if (ret != ESP_OK)
    {
      ESP_LOGE(TAG, "🔌 SWITCH B (%s) FAILED: %s", UI_LABEL_B, esp_err_to_name(ret));
      // TODO: Consider reverting the switch state on failure
    }
    else
    {
      ESP_LOGI(TAG, "🔌 SWITCH B (%s) SUCCESS: Device state changed to %s", UI_LABEL_B, state ? "ON" : "OFF");
    }
  }
}

/**
 * @brief Switch C event handler
 */
static void switch_c_event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);

  ESP_LOGI(TAG, "�?SWITCH C (%s): Event handler called with code %d", UI_LABEL_C, code);

  if (code == LV_EVENT_VALUE_CHANGED)
  {
    bool state = lv_obj_has_state(obj, LV_STATE_CHECKED);
    ESP_LOGI(TAG, "�?SWITCH C (%s) TOUCH EVENT: User selected %s", UI_LABEL_C, state ? "ON" : "OFF");

    // Control the actual device via Home Assistant
    esp_err_t ret = smart_home_control_switch(HA_ENTITY_C, state);
    if (ret != ESP_OK)
    {
      ESP_LOGE(TAG, "�?SWITCH C (%s) FAILED: %s", UI_LABEL_C, esp_err_to_name(ret));
      // TODO: Consider reverting the switch state on failure
    }
    else
    {
      ESP_LOGI(TAG, "�?SWITCH C (%s) SUCCESS: Device state changed to %s", UI_LABEL_C, state ? "ON" : "OFF");
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

// ══════════════════════════════════════════════════════════════════════════════�?
// PANEL CREATION FUNCTIONS
// ══════════════════════════════════════════════════════════════════════════════�?

/**
 * @brief Create the control panel
 * @param parent Parent screen object
 * @return Created control panel
 */
static lv_obj_t *create_control_panel(lv_obj_t *parent)
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
  lv_obj_add_event_cb(switch_a, switch_a_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(switch_a, switch_a_event_handler, LV_EVENT_CLICKED, NULL);
  ESP_LOGI(TAG, "Switch A (%s) created at x=160, better spacing", UI_LABEL_A);

  // Vertical separator after switch A
  ui_create_centered_vertical_separator(control_panel, 280, 60, 0x555555);

  // Switch B field - proper spacing from separator
  switch_b = ui_create_switch_field(control_panel, UI_LABEL_B, 300);
  lv_obj_add_event_cb(switch_b, debug_touch_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(switch_b, switch_b_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(switch_b, switch_b_event_handler, LV_EVENT_CLICKED, NULL);
  ESP_LOGI(TAG, "Switch B (%s) created at x=300, better spacing", UI_LABEL_B);

  // Vertical separator after switch B
  ui_create_centered_vertical_separator(control_panel, 420, 60, 0x555555);

  // Switch C field - proper spacing from separator
  switch_c = ui_create_switch_field(control_panel, UI_LABEL_C, 440);
  lv_obj_add_event_cb(switch_c, debug_touch_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(switch_c, switch_c_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(switch_c, switch_c_event_handler, LV_EVENT_CLICKED, NULL);
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

// ══════════════════════════════════════════════════════════════════════════════�?
// PUBLIC FUNCTIONS - MAIN UI INTERFACE
// ══════════════════════════════════════════════════════════════════════════════�?

/**
 * @brief Create the complete system monitor dashboard UI
 * @param disp LVGL display handle
 */
void system_monitor_ui_create(lv_display_t *disp)
{
  // Initialize LVGL theme with dark mode and blue/red accents
  lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                        LV_THEME_DEFAULT_DARK, font_normal);

  lv_obj_t *screen = lv_display_get_screen_active(disp);

  // Create all UI panels (smart panel at top, status panel at bottom)
  create_control_panel(screen);
  create_cpu_panel(screen);
  create_gpu_panel(screen);
  create_memory_panel(screen);
  create_status_info_panel(screen);

  ESP_LOGI(TAG, "System Monitor UI created successfully");
}

// ══════════════════════════════════════════════════════════════════════════════�?
// SYSTEM MONITOR UPDATE FUNCTIONS
// ══════════════════════════════════════════════════════════════════════════════�?

/**
 * @brief Update all system monitor display elements with new data
 * @param data Pointer to system monitoring data structure
 * @note This function is thread-safe and handles LVGL mutex locking
 */
void system_monitor_ui_update(const system_data_t *data)
{
  if (!data)
    return;

  lvgl_lock_acquire();

  // ─────────────────────────────────────────────────────────────────
  // Update Timestamp and Clock Display
  // ─────────────────────────────────────────────────────────────────

  if (timestamp_label && connection_status_label)
  {
    time_t timestamp_sec = data->timestamp / 1000;
    struct tm *timeinfo = localtime(&timestamp_sec);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "Last: %H:%M:%S", timeinfo);
    lv_label_set_text(timestamp_label, time_str);

    // Update the combined connection status with timestamp
    char combined_status[128];
    const char *current_status = lv_label_get_text(connection_status_label);
    if (strstr(current_status, "[SERIAL] Connected"))
    {
      snprintf(combined_status, sizeof(combined_status), "[SERIAL] Connected | %s", time_str);
    }
    else if (strstr(current_status, "[SERIAL] Connection Lost"))
    {
      snprintf(combined_status, sizeof(combined_status), "[SERIAL] Connection Lost | %s", time_str);
    }
    else
    {
      snprintf(combined_status, sizeof(combined_status), "[SERIAL] Waiting... | %s", time_str);
    }
    lv_label_set_text(connection_status_label, combined_status);
  }

  update_cpu_panel(&data->cpu);
  update_gpu_panel(&data->gpu);
  update_memory_panel(&data->mem);
  lvgl_lock_release();

  // Log less frequently to avoid blocking UI updates
  static uint32_t log_counter = 0;
  if (++log_counter % 10 == 0) // Log every 10th update
  {
    ESP_LOGI(TAG, "UI updated - CPU: %d%%, GPU: %d%%, MEM: %d%%",
             data->cpu.usage, data->gpu.usage, data->mem.usage);
  }
}

/**
 * @brief Update Home Assistant connection status in the controls panel
 * @param status_text HA status message to display
 * @param connected True if HA is connected, false otherwise
 */
void system_monitor_ui_update_ha_status(const char *status_text, bool connected)
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

// ══════════════════════════════════════════════════════════════════════════════�?
// SMART HOME CONTROL FUNCTIONS
// ══════════════════════════════════════════════════════════════════════════════�?

/**
 * @brief Set the state of switch A
 * @param state True to turn on, false to turn off
 */
void system_monitor_ui_set_switch_a(bool state)
{
  if (!switch_a)
    return;

  lvgl_lock_acquire();
  if (state)
    lv_obj_add_state(switch_a, LV_STATE_CHECKED);
  else
    lv_obj_clear_state(switch_a, LV_STATE_CHECKED);
  lvgl_lock_release();
}

/**
 * @brief Set the state of switch B
 * @param state True to turn on, false to turn off
 */
void system_monitor_ui_set_switch_b(bool state)
{
  if (!switch_b)
    return;

  lvgl_lock_acquire();
  if (state)
    lv_obj_add_state(switch_b, LV_STATE_CHECKED);
  else
    lv_obj_clear_state(switch_b, LV_STATE_CHECKED);
  lvgl_lock_release();
}

/**
 * @brief Set the state of switch C
 * @param state True to turn on, false to turn off
 */
void system_monitor_ui_set_switch_c(bool state)
{
  if (!switch_c)
    return;

  lvgl_lock_acquire();
  if (state)
    lv_obj_add_state(switch_c, LV_STATE_CHECKED);
  else
    lv_obj_clear_state(switch_c, LV_STATE_CHECKED);
  lvgl_lock_release();
}

/**
 * @brief Get the state of switch A
 * @return True if on, false if off
 */
bool system_monitor_ui_get_switch_a(void)
{
  if (!switch_a)
    return false;

  bool state = false;
  lvgl_lock_acquire();
  state = lv_obj_has_state(switch_a, LV_STATE_CHECKED);
  lvgl_lock_release();
  return state;
}

/**
 * @brief Get the state of switch B
 * @return True if on, false if off
 */
bool system_monitor_ui_get_switch_b(void)
{
  if (!switch_b)
    return false;

  bool state = false;
  lvgl_lock_acquire();
  state = lv_obj_has_state(switch_b, LV_STATE_CHECKED);
  lvgl_lock_release();
  return state;
}

/**
 * @brief Get the state of switch C
 * @return True if on, false if off
 */
bool system_monitor_ui_get_switch_c(void)
{
  if (!switch_c)
    return false;

  bool state = false;
  lvgl_lock_acquire();
  state = lv_obj_has_state(switch_c, LV_STATE_CHECKED);
  lvgl_lock_release();
  return state;
}
