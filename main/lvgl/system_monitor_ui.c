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

#include "system_monitor_ui.h"

#include "esp_log.h"
#include "lvgl_setup.h"
#include <stdio.h>
#include <time.h>

static const char *TAG = "system_monitor";

// ═══════════════════════════════════════════════════════════════════════════════
// UI ELEMENT HANDLES FOR REAL-TIME UPDATES
// ═══════════════════════════════════════════════════════════════════════════════

// Status and Info Elements
static lv_obj_t *timestamp_label = NULL;
static lv_obj_t *connection_status_label = NULL;
static lv_obj_t *wifi_status_label = NULL;

// Smart Panel Elements
static lv_obj_t *water_pump_switch = NULL;
static lv_obj_t *wave_maker_switch = NULL;
static lv_obj_t *light_switch = NULL;
static lv_obj_t *feed_button = NULL;

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

// ═══════════════════════════════════════════════════════════════════════════════
// FONT DEFINITIONS
// ═══════════════════════════════════════════════════════════════════════════════
static const lv_font_t *font_title = &lv_font_montserrat_28;       // Large title font (28px)
static const lv_font_t *font_normal = &lv_font_montserrat_16;      // Normal text
static const lv_font_t *font_small = &lv_font_montserrat_14;       // Small text
static const lv_font_t *font_big_numbers = &lv_font_montserrat_32; // Large numbers (32px)

// ═══════════════════════════════════════════════════════════════════════════════
// HELPER FUNCTIONS FOR UI CREATION
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Create a standard panel with common styling
 * @param parent Parent object
 * @param width Panel width
 * @param height Panel height
 * @param x X position
 * @param y Y position
 * @param bg_color Background color (hex)
 * @param border_color Border color (hex)
 * @return Created panel object
 */
static lv_obj_t *create_panel(lv_obj_t *parent, int width, int height, int x, int y,
                              uint32_t bg_color, uint32_t border_color)
{
  lv_obj_t *panel = lv_obj_create(parent);
  lv_obj_set_size(panel, width, height);
  lv_obj_set_pos(panel, x, y);
  lv_obj_set_style_bg_color(panel, lv_color_hex(bg_color), 0);
  lv_obj_set_style_border_color(panel, lv_color_hex(border_color), 0);
  lv_obj_set_style_border_width(panel, 2, 0);
  lv_obj_set_style_radius(panel, 8, 0);
  lv_obj_set_style_pad_all(panel, 15, 0);
  lv_obj_set_scrollbar_mode(panel, LV_SCROLLBAR_MODE_OFF);
  return panel;
}

/**
 * @brief Create a title label with separator line
 * @param parent Parent panel
 * @param title Title text
 * @param title_color Title color (hex)
 * @param separator_width Width of separator line
 * @return Created title label object
 */
static lv_obj_t *create_title_with_separator(lv_obj_t *parent, const char *title,
                                             uint32_t title_color, int separator_width)
{
  // Title label
  lv_obj_t *title_label = lv_label_create(parent);
  lv_label_set_text(title_label, title);
  lv_obj_set_style_text_font(title_label, font_title, 0);
  lv_obj_set_style_text_color(title_label, lv_color_hex(title_color), 0);
  lv_obj_set_pos(title_label, 0, 0);

  // Separator line
  lv_obj_t *separator = lv_obj_create(parent);
  lv_obj_set_size(separator, separator_width, 2);
  lv_obj_set_pos(separator, 0, 35);
  lv_obj_set_style_bg_color(separator, lv_color_hex(title_color), 0);
  lv_obj_set_style_border_width(separator, 0, 0);
  lv_obj_set_style_radius(separator, 1, 0);

  return title_label;
}

/**
 * @brief Create a field (label + value) pair
 * @param parent Parent panel
 * @param field_name Field name text
 * @param default_value Default value text
 * @param x X position
 * @param label_font Font for field label
 * @param value_font Font for field value
 * @param label_color Color for field label
 * @param value_color Color for field value
 * @return Pointer to the value label (for updating)
 */
static lv_obj_t *create_field(lv_obj_t *parent, const char *field_name, const char *default_value,
                              int x, const lv_font_t *label_font, const lv_font_t *value_font,
                              uint32_t label_color, uint32_t value_color)
{
  // Field label
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, field_name);
  lv_obj_set_style_text_font(label, label_font, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(label_color), 0);
  lv_obj_set_pos(label, x, 55);

  // Field value - using left-bottom anchor for consistent baseline alignment
  lv_obj_t *value = lv_label_create(parent);
  lv_label_set_text(value, default_value);
  lv_obj_set_style_text_font(value, value_font, 0);
  lv_obj_set_style_text_color(value, lv_color_hex(value_color), 0);
  // Use align to position value text with left-bottom anchor at consistent Y baseline
  lv_obj_align(value, LV_ALIGN_BOTTOM_LEFT, x, -5); // Bottom-left anchor, very close to bottom for maximum spacing

  return value;
}

/**
 * @brief Create a vertical separator line
 * @param parent Parent panel
 * @param x X position
 * @param y Y position
 * @param height Height of separator
 * @param color Color (hex)
 * @return Created separator object
 */
static lv_obj_t *create_vertical_separator(lv_obj_t *parent, int x, int y, int height, uint32_t color)
{
  lv_obj_t *separator = lv_obj_create(parent);
  lv_obj_set_size(separator, 1, height);
  lv_obj_set_pos(separator, x, y);
  lv_obj_set_style_bg_color(separator, lv_color_hex(color), 0);
  lv_obj_set_style_border_width(separator, 0, 0);
  lv_obj_set_style_radius(separator, 0, 0);
  return separator;
}

/**
 * @brief Create a vertically centered separator using alignment API
 * @param parent Parent panel
 * @param x X position
 * @param height Separator height
 * @param color Separator color (hex)
 * @return Created separator object
 */
static lv_obj_t *create_centered_vertical_separator(lv_obj_t *parent, int x, int height, uint32_t color)
{
  lv_obj_t *separator = lv_obj_create(parent);
  lv_obj_set_size(separator, 1, height);
  lv_obj_set_style_bg_color(separator, lv_color_hex(color), 0);
  lv_obj_set_style_border_width(separator, 0, 0);
  lv_obj_set_style_radius(separator, 0, 0);
  lv_obj_align(separator, LV_ALIGN_LEFT_MID, x, 0);
  return separator;
}

/**
 * @brief Create a switch field with label above it, positioned and aligned together
 * @param parent Parent panel
 * @param label_text Text for the label above the switch
 * @param x_offset X position offset from left edge
 * @return Created switch object
 */
static lv_obj_t *create_switch_field(lv_obj_t *parent, const char *label_text, int x_offset)
{
  // Create the label first (positioned above where the switch will be)
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, label_text);
  lv_obj_set_style_text_font(label, font_small, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(0xcccccc), 0);
  lv_obj_align(label, LV_ALIGN_LEFT_MID, x_offset, -25); // Position 25px above center

  // Create the switch (moved down 10px from center)
  lv_obj_t *switch_obj = lv_switch_create(parent);
  lv_obj_set_size(switch_obj, 60, 30);
  lv_obj_align(switch_obj, LV_ALIGN_LEFT_MID, x_offset, 10);

  return switch_obj;
}

/**
 * @brief Create a progress bar
 * @param parent Parent panel
 * @param width Bar width
 * @param height Bar height
 * @param x X position
 * @param y Y position
 * @param bg_color Background color (hex)
 * @param indicator_color Indicator color (hex)
 * @param radius Corner radius
 * @return Created progress bar object
 */
static lv_obj_t *create_progress_bar(lv_obj_t *parent, int width, int height, int x, int y,
                                     uint32_t bg_color, uint32_t indicator_color, int radius)
{
  lv_obj_t *bar = lv_bar_create(parent);
  lv_obj_set_size(bar, width, height);
  lv_obj_set_pos(bar, x, y);
  lv_obj_set_style_bg_color(bar, lv_color_hex(bg_color), LV_PART_MAIN);
  lv_obj_set_style_bg_color(bar, lv_color_hex(indicator_color), LV_PART_INDICATOR);
  lv_obj_set_style_radius(bar, radius, 0);
  lv_bar_set_value(bar, 0, LV_ANIM_OFF);
  return bar;
}

/**
 * @brief Create a status panel with minimal styling
 * @param parent Parent object
 * @param width Panel width
 * @param height Panel height
 * @param x X position
 * @param y Y position
 * @param bg_color Background color (hex)
 * @param border_color Border color (hex)
 * @return Created panel object
 */
static lv_obj_t *create_status_panel(lv_obj_t *parent, int width, int height, int x, int y,
                                     uint32_t bg_color, uint32_t border_color)
{
  lv_obj_t *panel = lv_obj_create(parent);
  lv_obj_set_size(panel, width, height);
  lv_obj_set_pos(panel, x, y);
  lv_obj_set_style_bg_color(panel, lv_color_hex(bg_color), 0);
  lv_obj_set_style_border_color(panel, lv_color_hex(border_color), 0);
  lv_obj_set_style_border_width(panel, 1, 0);
  lv_obj_set_style_radius(panel, 6, 0);
  lv_obj_set_style_pad_all(panel, 6, 0);
  lv_obj_set_scrollbar_mode(panel, LV_SCROLLBAR_MODE_OFF);
  return panel;
}

// ═══════════════════════════════════════════════════════════════════════════════
// PANEL CREATION FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Create the control panel
 * @param parent Parent screen object
 * @return Created control panel
 */
static lv_obj_t *create_control_panel(lv_obj_t *parent)
{
  lv_obj_t *control_panel = create_panel(parent, 780, 100, 10, 10, 0x1a1a2e, 0x2e2e4a);

  // Controls title (centered vertically using align API)
  lv_obj_t *controls_title = lv_label_create(control_panel);
  lv_label_set_text(controls_title, "Controls");
  lv_obj_set_style_text_font(controls_title, font_title, 0);
  lv_obj_set_style_text_color(controls_title, lv_color_hex(0x4fc3f7), 0);
  lv_obj_align(controls_title, LV_ALIGN_LEFT_MID, 0, 0);

  // Vertical separator after controls title (centered using align API)
  create_centered_vertical_separator(control_panel, 140, 60, 0x4fc3f7);

  // Water pump switch field
  water_pump_switch = create_switch_field(control_panel, "Water Pump", 180);

  // Vertical separator after water pump (centered using align API)
  create_centered_vertical_separator(control_panel, 300, 60, 0x555555);

  // Wave maker switch field
  wave_maker_switch = create_switch_field(control_panel, "Wave Maker", 340);

  // Vertical separator after wave maker (centered using align API)
  create_centered_vertical_separator(control_panel, 460, 60, 0x555555);

  // Light switch field
  light_switch = create_switch_field(control_panel, "Light", 500);

  // Vertical separator before feed button (centered using align API)
  create_centered_vertical_separator(control_panel, 580, 60, 0x555555);

  // Feed button (right with proper padding, centered using align API)
  feed_button = lv_btn_create(control_panel);
  lv_obj_set_size(feed_button, 120, 50);
  lv_obj_align(feed_button, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_set_style_bg_color(feed_button, lv_color_hex(0x4caf50), 0);
  lv_obj_set_style_radius(feed_button, 10, 0);

  lv_obj_t *feed_label = lv_label_create(feed_button);
  lv_label_set_text(feed_label, "FEED");
  lv_obj_set_style_text_font(feed_label, font_normal, 0);
  lv_obj_set_style_text_color(feed_label, lv_color_hex(0xffffff), 0);
  lv_obj_center(feed_label);
  return control_panel;
}

/**
 * @brief Create the CPU monitoring panel
 * @param parent Parent screen object
 * @return Created CPU panel
 */
static lv_obj_t *create_cpu_panel(lv_obj_t *parent)
{
  lv_obj_t *cpu_panel = create_panel(parent, 385, 150, 10, 120, 0x1a1a2e, 0x16213e);

  // CPU Title with separator
  create_title_with_separator(cpu_panel, "CPU", 0x4fc3f7, 355);

  // CPU Name (positioned to the right of title, baseline aligned)
  cpu_name_label = lv_label_create(cpu_panel);
  lv_label_set_text(cpu_name_label, "Unknown CPU");
  lv_obj_set_style_text_font(cpu_name_label, font_small, 0);
  lv_obj_set_style_text_color(cpu_name_label, lv_color_hex(0x888888), 0);
  lv_obj_set_pos(cpu_name_label, 80, 8);

  // Create CPU fields - Temperature first
  cpu_temp_label = create_field(cpu_panel, "Temp", "--°C", 10, font_normal, font_big_numbers, 0xaaaaaa, 0xff7043);
  cpu_usage_label = create_field(cpu_panel, "Usage", "0%", 128, font_normal, font_big_numbers, 0xaaaaaa, 0x4fc3f7);
  cpu_fan_label = create_field(cpu_panel, "Fan (RPM)", "--", 246, font_normal, font_big_numbers, 0xaaaaaa, 0x81c784);

  // Create vertical separators between fields
  create_vertical_separator(cpu_panel, 118, 50, 60, 0x555555);
  create_vertical_separator(cpu_panel, 236, 50, 60, 0x555555);

  return cpu_panel;
}

/**
 * @brief Create the GPU monitoring panel
 * @param parent Parent screen object
 * @return Created GPU panel
 */
static lv_obj_t *create_gpu_panel(lv_obj_t *parent)
{
  lv_obj_t *gpu_panel = create_panel(parent, 385, 150, 405, 120, 0x1a2e1a, 0x2e4f2e);

  // GPU Title with separator
  create_title_with_separator(gpu_panel, "GPU", 0x4caf50, 355);

  // GPU Name (positioned to the right of title, baseline aligned)
  gpu_name_label = lv_label_create(gpu_panel);
  lv_label_set_text(gpu_name_label, "Unknown GPU");
  lv_obj_set_style_text_font(gpu_name_label, font_small, 0);
  lv_obj_set_style_text_color(gpu_name_label, lv_color_hex(0x888888), 0);
  lv_obj_set_pos(gpu_name_label, 80, 8);

  // Create GPU fields - Temperature first
  gpu_temp_label = create_field(gpu_panel, "Temp", "--°C", 10, font_normal, font_big_numbers, 0xaaaaaa, 0xff7043);
  gpu_usage_label = create_field(gpu_panel, "Usage", "0%", 128, font_normal, font_big_numbers, 0xaaaaaa, 0x4caf50);
  gpu_mem_label = create_field(gpu_panel, "Memory", "0%", 246, font_normal, font_big_numbers, 0xaaaaaa, 0x81c784);

  // Create vertical separators between GPU fields
  create_vertical_separator(gpu_panel, 118, 50, 60, 0x555555);
  create_vertical_separator(gpu_panel, 236, 50, 60, 0x555555);

  return gpu_panel;
}

/**
 * @brief Create the memory monitoring panel
 * @param parent Parent screen object
 * @return Created memory panel
 */
static lv_obj_t *create_memory_panel(lv_obj_t *parent)
{
  lv_obj_t *mem_panel = create_panel(parent, 780, 120, 10, 280, 0x2e1a1a, 0x4f2e2e);

  // Memory title with separator
  create_title_with_separator(mem_panel, "Memory", 0xff7043, 750);

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
  create_vertical_separator(mem_panel, 150, 45, 45, 0x555555);

  // Progress bar (positioned to the right of the separator)
  mem_usage_bar = create_progress_bar(mem_panel, 500, 25, 170, 65, 0x1a1a2e, 0xff7043, 12);

  return mem_panel;
}

/**
 * @brief Create the status panel with connection info
 * @param parent Parent screen object
 * @return Created status panel
 */
static lv_obj_t *create_status_info_panel(lv_obj_t *parent)
{
  lv_obj_t *status_panel = create_status_panel(parent, 780, 50, 10, 410, 0x0f0f0f, 0x222222);

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

// ═══════════════════════════════════════════════════════════════════════════════
// PUBLIC FUNCTIONS - MAIN UI INTERFACE
// ═══════════════════════════════════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════════════════════════════════
// SYSTEM MONITOR UPDATE FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════════════════════════════════
// CONNECTION STATUS MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════════════════════════════════
// SMART HOME CONTROL FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Set the state of the water pump switch
 * @param state True to turn on, false to turn off
 */
void system_monitor_ui_set_water_pump(bool state)
{
  if (!water_pump_switch)
    return;

  lvgl_lock_acquire();
  if (state)
    lv_obj_add_state(water_pump_switch, LV_STATE_CHECKED);
  else
    lv_obj_clear_state(water_pump_switch, LV_STATE_CHECKED);
  lvgl_lock_release();
}

/**
 * @brief Set the state of the wave maker switch
 * @param state True to turn on, false to turn off
 */
void system_monitor_ui_set_wave_maker(bool state)
{
  if (!wave_maker_switch)
    return;

  lvgl_lock_acquire();
  if (state)
    lv_obj_add_state(wave_maker_switch, LV_STATE_CHECKED);
  else
    lv_obj_clear_state(wave_maker_switch, LV_STATE_CHECKED);
  lvgl_lock_release();
}

/**
 * @brief Set the state of the light switch
 * @param state True to turn on, false to turn off
 */
void system_monitor_ui_set_light(bool state)
{
  if (!light_switch)
    return;

  lvgl_lock_acquire();
  if (state)
    lv_obj_add_state(light_switch, LV_STATE_CHECKED);
  else
    lv_obj_clear_state(light_switch, LV_STATE_CHECKED);
  lvgl_lock_release();
}

/**
 * @brief Get the state of the water pump switch
 * @return True if on, false if off
 */
bool system_monitor_ui_get_water_pump(void)
{
  if (!water_pump_switch)
    return false;

  bool state = false;
  lvgl_lock_acquire();
  state = lv_obj_has_state(water_pump_switch, LV_STATE_CHECKED);
  lvgl_lock_release();
  return state;
}

/**
 * @brief Get the state of the wave maker switch
 * @return True if on, false if off
 */
bool system_monitor_ui_get_wave_maker(void)
{
  if (!wave_maker_switch)
    return false;

  bool state = false;
  lvgl_lock_acquire();
  state = lv_obj_has_state(wave_maker_switch, LV_STATE_CHECKED);
  lvgl_lock_release();
  return state;
}

/**
 * @brief Get the state of the light switch
 * @return True if on, false if off
 */
bool system_monitor_ui_get_light(void)
{
  if (!light_switch)
    return false;

  bool state = false;
  lvgl_lock_acquire();
  state = lv_obj_has_state(light_switch, LV_STATE_CHECKED);
  lvgl_lock_release();
  return state;
}
