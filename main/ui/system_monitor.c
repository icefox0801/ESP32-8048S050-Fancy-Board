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
static lv_obj_t *timestamp_label = NULL;
static lv_obj_t *connection_status_label = NULL;
static lv_obj_t *wifi_status_label = NULL;

// Smart Panel Elements
static lv_obj_t *switch_a = NULL;
static lv_obj_t *switch_b = NULL;
static lv_obj_t *switch_c = NULL;
static lv_obj_t *scene_button = NULL;
static lv_obj_t *ha_status_label = NULL;

// CPU Section Elements
static lv_obj_t *cpu_name_label = NULL;
static lv_obj_t *cpu_usage_label = NULL;
static lv_obj_t *cpu_temp_label = NULL;
static lv_obj_t *cpu_fan_label = NULL; // Added fan field

// GPU Section Elements
static lv_obj_t *gpu_name_label = NULL;
static lv_obj_t *gpu_usage_label = NULL;
static lv_obj_t *gpu_temp_label = NULL;
static lv_obj_t *gpu_mem_label = NULL;

// Memory Section Elements
static lv_obj_t *mem_usage_bar = NULL;
static lv_obj_t *mem_usage_label = NULL;
static lv_obj_t *mem_info_label = NULL;

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

/**
 * @brief Create the CPU monitoring panel
 * @param parent Parent screen object
 * @return Created CPU panel
 */
static lv_obj_t *create_cpu_panel(lv_obj_t *parent)
{
  lv_obj_t *cpu_panel = ui_create_panel(parent, 385, 150, 10, 120, 0x1a1a2e, 0x16213e);

  // CPU Title with separator
  ui_create_title_with_separator(cpu_panel, "CPU", 0x4fc3f7, 355);

  // CPU Name (positioned to the right of title, baseline aligned)
  cpu_name_label = lv_label_create(cpu_panel);
  lv_label_set_text(cpu_name_label, "Unknown CPU");
  lv_obj_set_style_text_font(cpu_name_label, font_small, 0);
  lv_obj_set_style_text_color(cpu_name_label, lv_color_hex(0x888888), 0);
  lv_obj_set_pos(cpu_name_label, 80, 8);

  // Create CPU fields - Temperature first
  cpu_temp_label = ui_create_field(cpu_panel, "Temp", "--°C", 10, font_normal, font_big_numbers, 0xaaaaaa, 0xff7043);
  cpu_usage_label = ui_create_field(cpu_panel, "Usage", "0%", 128, font_normal, font_big_numbers, 0xaaaaaa, 0x4fc3f7);
  cpu_fan_label = ui_create_field(cpu_panel, "Fan (RPM)", "--", 246, font_normal, font_big_numbers, 0xaaaaaa, 0x81c784);

  // Create vertical separators between fields
  ui_create_vertical_separator(cpu_panel, 118, 50, 60, 0x555555);
  ui_create_vertical_separator(cpu_panel, 236, 50, 60, 0x555555);

  return cpu_panel;
}

/**
 * @brief Create the GPU monitoring panel
 * @param parent Parent screen object
 * @return Created GPU panel
 */
static lv_obj_t *create_gpu_panel(lv_obj_t *parent)
{
  lv_obj_t *gpu_panel = ui_create_panel(parent, 385, 150, 405, 120, 0x1a2e1a, 0x2e4f2e);

  // GPU Title with separator
  ui_create_title_with_separator(gpu_panel, "GPU", 0x4caf50, 355);

  // GPU Name (positioned to the right of title, baseline aligned)
  gpu_name_label = lv_label_create(gpu_panel);
  lv_label_set_text(gpu_name_label, "Unknown GPU");
  lv_obj_set_style_text_font(gpu_name_label, font_small, 0);
  lv_obj_set_style_text_color(gpu_name_label, lv_color_hex(0x888888), 0);
  lv_obj_set_pos(gpu_name_label, 80, 8);

  // Create GPU fields - Temperature first
  gpu_temp_label = ui_create_field(gpu_panel, "Temp", "--°C", 10, font_normal, font_big_numbers, 0xaaaaaa, 0xff7043);
  gpu_usage_label = ui_create_field(gpu_panel, "Usage", "0%", 128, font_normal, font_big_numbers, 0xaaaaaa, 0x4caf50);
  gpu_mem_label = ui_create_field(gpu_panel, "Memory", "0%", 246, font_normal, font_big_numbers, 0xaaaaaa, 0x81c784);

  // Create vertical separators between GPU fields
  ui_create_vertical_separator(gpu_panel, 118, 50, 60, 0x555555);
  ui_create_vertical_separator(gpu_panel, 236, 50, 60, 0x555555);

  return gpu_panel;
}

/**
 * @brief Create the memory monitoring panel
 * @param parent Parent screen object
 * @return Created memory panel
 */
static lv_obj_t *create_memory_panel(lv_obj_t *parent)
{
  lv_obj_t *mem_panel = ui_create_panel(parent, 780, 120, 10, 280, 0x2e1a1a, 0x4f2e2e);

  // Memory title with separator
  ui_create_title_with_separator(mem_panel, "Memory", 0xff7043, 750);

  // Memory info positioned to the right of title (baseline aligned)
  mem_info_label = lv_label_create(mem_panel);
  lv_label_set_text(mem_info_label, "(-.- GB / -.- GB)");
  lv_obj_set_style_text_font(mem_info_label, font_small, 0);
  lv_obj_set_style_text_color(mem_info_label, lv_color_hex(0xcccccc), 0);
  lv_obj_set_pos(mem_info_label, 240, 8);

  // Create memory usage value (without label)
  mem_usage_label = lv_label_create(mem_panel);
  lv_label_set_text(mem_usage_label, "0%");
  lv_obj_set_style_text_font(mem_usage_label, font_big_numbers, 0);
  lv_obj_set_style_text_color(mem_usage_label, lv_color_hex(0xff7043), 0);
  lv_obj_align(mem_usage_label, LV_ALIGN_BOTTOM_LEFT, 10, -5);

  // Dimmed vertical separator between usage field and progress bar
  ui_create_vertical_separator(mem_panel, 150, 45, 45, 0x555555);

  // Progress bar (positioned to the right of the separator)
  mem_usage_bar = ui_create_progress_bar(mem_panel, 500, 25, 170, 65, 0x1a1a2e, 0xff7043, 12);

  return mem_panel;
}

/**
 * @brief Create the status panel with connection info
 * @param parent Parent screen object
 * @return Created status panel
 */
static lv_obj_t *create_status_info_panel(lv_obj_t *parent)
{
  lv_obj_t *status_panel = ui_create_status_panel(parent, 780, 50, 10, 410, 0x0f0f0f, 0x222222);

  // Serial connection status with last update time (left side)
  connection_status_label = lv_label_create(status_panel);
  lv_label_set_text(connection_status_label, "[SERIAL] Waiting... | Last: Never");
  lv_obj_set_style_text_font(connection_status_label, font_small, 0);
  lv_obj_set_style_text_color(connection_status_label, lv_color_hex(0xffaa00), 0);
  lv_obj_set_pos(connection_status_label, 10, 11);

  // WiFi status (right side)
  wifi_status_label = lv_label_create(status_panel);
  lv_label_set_text(wifi_status_label, "[WIFI] Connecting...");
  lv_obj_set_style_text_font(wifi_status_label, font_small, 0);
  lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0x00aaff), 0);
  lv_obj_align(wifi_status_label, LV_ALIGN_TOP_RIGHT, -10, 11);

  // Hidden timestamp label for internal timestamp tracking
  timestamp_label = lv_label_create(status_panel);
  lv_label_set_text(timestamp_label, "Last: Never");
  lv_obj_add_flag(timestamp_label, LV_OBJ_FLAG_HIDDEN); // Hide this element

  return status_panel;
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

  // ─────────────────────────────────────────────────────────────────
  // Update CPU Section
  // ─────────────────────────────────────────────────────────────────

  if (cpu_name_label)
  {
    lv_label_set_text(cpu_name_label, data->cpu.name);
  }

  if (cpu_usage_label)
  {
    char usage_str[16];
    snprintf(usage_str, sizeof(usage_str), "%d%%", data->cpu.usage);
    lv_label_set_text(cpu_usage_label, usage_str);
  }

  if (cpu_temp_label)
  {
    char temp_str[16];
    snprintf(temp_str, sizeof(temp_str), "%d°C", data->cpu.temp); // Shortened format
    lv_label_set_text(cpu_temp_label, temp_str);
  }

  if (cpu_fan_label)
  {
    char fan_str[16];
    snprintf(fan_str, sizeof(fan_str), "%d", data->cpu.fan);
    lv_label_set_text(cpu_fan_label, fan_str);
  }

  // ─────────────────────────────────────────────────────────────────
  // Update GPU Section
  // ─────────────────────────────────────────────────────────────────

  if (gpu_name_label)
  {
    lv_label_set_text(gpu_name_label, data->gpu.name);
  }

  if (gpu_usage_label)
  {
    char usage_str[16];
    snprintf(usage_str, sizeof(usage_str), "%d%%", data->gpu.usage);
    lv_label_set_text(gpu_usage_label, usage_str);
  }

  if (gpu_temp_label)
  {
    char temp_str[16];
    snprintf(temp_str, sizeof(temp_str), "%d°C", data->gpu.temp); // Shortened format
    lv_label_set_text(gpu_temp_label, temp_str);
  }

  if (gpu_mem_label)
  {
    uint8_t mem_usage_pct = (data->gpu.mem_used * 100) / data->gpu.mem_total;
    char mem_str[32];
    snprintf(mem_str, sizeof(mem_str), "%d%%", mem_usage_pct);
    lv_label_set_text(gpu_mem_label, mem_str);
  }

  // ─────────────────────────────────────────────────────────────────
  // Update Memory Section
  // ─────────────────────────────────────────────────────────────────

  if (mem_usage_bar && mem_usage_label)
  {
    lv_bar_set_value(mem_usage_bar, data->mem.usage, LV_ANIM_OFF); // Disable animation for instant updates
    char usage_str[16];
    snprintf(usage_str, sizeof(usage_str), "%d%%", data->mem.usage);
    lv_label_set_text(mem_usage_label, usage_str);
  }

  if (mem_info_label)
  {
    char mem_str[32];
    snprintf(mem_str, sizeof(mem_str), "(%.1f GB / %.1f GB)", // Updated format to match new compact layout
             data->mem.used, data->mem.total);
    lv_label_set_text(mem_info_label, mem_str);
  }

  // ─────────────────────────────────────────────────────────────────
  // Release LVGL Mutex and Log Update
  // ─────────────────────────────────────────────────────────────────

  lvgl_lock_release();

  // Log less frequently to avoid blocking UI updates
  static uint32_t log_counter = 0;
  if (++log_counter % 10 == 0) // Log every 10th update
  {
    ESP_LOGI(TAG, "UI updated - CPU: %d%%, GPU: %d%%, MEM: %d%%",
             data->cpu.usage, data->gpu.usage, data->mem.usage);
  }
}

// ══════════════════════════════════════════════════════════════════════════════�?
// CONNECTION STATUS MANAGEMENT
// ══════════════════════════════════════════════════════════════════════════════�?

/**
 * @brief Update connection status indicator
 * @param connected True if connection is active, false if lost
 * @note Changes color and text of status indicator based on connection state
 */
void system_monitor_ui_set_connection_status(bool connected)
{
  if (!connection_status_label)
    return;

  lvgl_lock_acquire();

  // Get current timestamp from the hidden timestamp label
  const char *current_timestamp = "Last: Never";
  if (timestamp_label)
  {
    current_timestamp = lv_label_get_text(timestamp_label);
  }

  char combined_status[128];
  if (connected)
  {
    snprintf(combined_status, sizeof(combined_status), "[SERIAL] Connected | %s", current_timestamp);
    lv_label_set_text(connection_status_label, combined_status);
    lv_obj_set_style_text_color(connection_status_label, lv_color_hex(0x00ff88), 0); // Green
  }
  else
  {
    snprintf(combined_status, sizeof(combined_status), "[SERIAL] Connection Lost | %s", current_timestamp);
    lv_label_set_text(connection_status_label, combined_status);
    lv_obj_set_style_text_color(connection_status_label, lv_color_hex(0xff4444), 0); // Red
  }

  lvgl_lock_release();
}

/**
 * @brief Update WiFi connection status in the status panel
 * @param status_text WiFi status message to display
 * @param connected True if WiFi is connected, false otherwise
 */
void system_monitor_ui_update_wifi_status(const char *status_text, bool connected)
{
  if (!wifi_status_label || !status_text)
    return;

  lvgl_lock_acquire();

  // Create formatted status message
  char wifi_msg[128];

  if (connected && strstr(status_text, "Connected:") != NULL)
  {
    // Extract SSID from "Connected: SSID (IP)" format
    const char *ssid_start = strstr(status_text, "Connected: ") + 11; // Skip "Connected: "
    const char *ssid_end = strchr(ssid_start, ' ');                   // Find space before (IP)

    if (ssid_end != NULL)
    {
      // Extract SSID
      size_t ssid_len = ssid_end - ssid_start;
      char ssid[64];
      strncpy(ssid, ssid_start, ssid_len);
      ssid[ssid_len] = '\0';

      snprintf(wifi_msg, sizeof(wifi_msg), "[WIFI:%s] Connected", ssid);
    }
    else
    {
      // Fallback if format is unexpected
      snprintf(wifi_msg, sizeof(wifi_msg), "[WIFI] %s", status_text);
    }
  }
  else
  {
    // For non-connected states, use normal format
    snprintf(wifi_msg, sizeof(wifi_msg), "[WIFI] %s", status_text);
  }

  lv_label_set_text(wifi_status_label, wifi_msg);

  // Set color based on connection status
  if (connected)
  {
    lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0x00ff88), 0); // Green
  }
  else
  {
    lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xff4444), 0); // Red
  }

  lvgl_lock_release();
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
