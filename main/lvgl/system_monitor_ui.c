/**
 * @file system_monitor_ui.c
 * @brief System Monitor Dashboard UI for ESP32-S3-8048S050
 *
 * Creates a real-time system monitoring dashboard that displays:
 * - CPU usage, temperature, and name
 * - GPU usage, temperature, memory, and name
 * - System memory usage and statistics
 * - Connection status and timestamp
 */

#include "system_monitor_ui.h"
#include "lvgl_setup.h"
#include "esp_log.h"
#include <stdio.h>
#include <time.h>

static const char *TAG = "system_monitor";

// ═══════════════════════════════════════════════════════════════════════════════
// UI ELEMENT HANDLES FOR REAL-TIME UPDATES
// ═══════════════════════════════════════════════════════════════════════════════

// Status and Info Elements
static lv_obj_t *timestamp_label = NULL;
static lv_obj_t *connection_status_label = NULL;

// CPU Section Elements
static lv_obj_t *cpu_name_label = NULL;
static lv_obj_t *cpu_usage_bar = NULL;
static lv_obj_t *cpu_usage_label = NULL;
static lv_obj_t *cpu_temp_label = NULL;

// GPU Section Elements
static lv_obj_t *gpu_name_label = NULL;
static lv_obj_t *gpu_usage_bar = NULL;
static lv_obj_t *gpu_usage_label = NULL;
static lv_obj_t *gpu_temp_label = NULL;
static lv_obj_t *gpu_mem_bar = NULL;
static lv_obj_t *gpu_mem_label = NULL;

// Memory Section Elements
static lv_obj_t *mem_usage_bar = NULL;
static lv_obj_t *mem_usage_label = NULL;
static lv_obj_t *mem_info_label = NULL;

// ═══════════════════════════════════════════════════════════════════════════════
// FONT DEFINITIONS
// ═══════════════════════════════════════════════════════════════════════════════
static const lv_font_t *font_title = &lv_font_montserrat_14;
static const lv_font_t *font_normal = &lv_font_montserrat_14;
static const lv_font_t *font_small = &lv_font_montserrat_14;

// ═══════════════════════════════════════════════════════════════════════════════
// PRIVATE HELPER FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Create a labeled progress bar section
 * @param parent Parent object to attach to
 * @param title Section title text
 * @param bar_out Output pointer for progress bar object
 * @param label_out Output pointer for value label object
 * @param x X coordinate position
 * @param y Y coordinate position
 * @param width Progress bar width
 * @return Title label object
 */
static lv_obj_t *create_progress_section(lv_obj_t *parent, const char *title,
                                         lv_obj_t **bar_out, lv_obj_t **label_out,
                                         lv_coord_t x, lv_coord_t y, lv_coord_t width)
{
  // Create title label
  lv_obj_t *title_label = lv_label_create(parent);
  lv_label_set_text(title_label, title);
  lv_obj_set_style_text_font(title_label, font_normal, 0);
  lv_obj_set_style_text_color(title_label, lv_color_white(), 0);
  lv_obj_set_pos(title_label, x, y);

  // Create progress bar
  lv_obj_t *bar = lv_bar_create(parent);
  lv_obj_set_size(bar, width, 16);
  lv_obj_set_pos(bar, x, y + 22);
  lv_obj_set_style_bg_color(bar, lv_color_hex(0x333333), LV_PART_MAIN);
  lv_obj_set_style_bg_color(bar, lv_color_hex(0x00aaff), LV_PART_INDICATOR);
  lv_obj_set_style_radius(bar, 8, 0);

  // Create value label
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, "0%");
  lv_obj_set_style_text_font(label, font_small, 0);
  lv_obj_set_style_text_color(label, lv_color_white(), 0);
  lv_obj_set_pos(label, x + width + 10, y + 20);

  *bar_out = bar;
  *label_out = label;

  return title_label;
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

  // ─────────────────────────────────────────────────────────────────────────────
  // Header Section - Title and Status
  // ─────────────────────────────────────────────────────────────────────────────

  // Main dashboard title
  lv_obj_t *title_label = lv_label_create(screen);
  lv_label_set_text(title_label, "System Monitor Dashboard");
  lv_obj_set_style_text_font(title_label, font_title, 0);
  lv_obj_set_style_text_color(title_label, lv_color_hex(0x00ff88), 0);
  lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 5);

  // Connection status indicator
  connection_status_label = lv_label_create(screen);
  lv_label_set_text(connection_status_label, "● Waiting for data...");
  lv_obj_set_style_text_font(connection_status_label, font_small, 0);
  lv_obj_set_style_text_color(connection_status_label, lv_color_hex(0xffaa00), 0);
  lv_obj_set_pos(connection_status_label, 10, 30);

  // Timestamp
  timestamp_label = lv_label_create(screen);
  lv_label_set_text(timestamp_label, "Last Update: Never");
  lv_obj_set_style_text_font(timestamp_label, font_small, 0);
  lv_obj_set_style_text_color(timestamp_label, lv_color_hex(0xcccccc), 0);
  lv_obj_align(timestamp_label, LV_ALIGN_TOP_RIGHT, -10, 30);

  // ===== CPU SECTION =====
  lv_obj_t *cpu_panel = lv_obj_create(screen);
  lv_obj_set_size(cpu_panel, 360, 100);
  lv_obj_set_pos(cpu_panel, 10, 60);
  lv_obj_set_style_bg_color(cpu_panel, lv_color_hex(0x1a1a2e), 0);
  lv_obj_set_style_border_color(cpu_panel, lv_color_hex(0x16213e), 0);
  lv_obj_set_style_border_width(cpu_panel, 2, 0);
  lv_obj_set_style_radius(cpu_panel, 8, 0);

  // ─────────────────────────────────────────────────────────────────
  // CPU Section UI Elements
  // ─────────────────────────────────────────────────────────────────

  lv_obj_t *cpu_title = lv_label_create(cpu_panel);
  lv_label_set_text(cpu_title, "🖥️ CPU");
  lv_obj_set_style_text_color(cpu_title, lv_color_hex(0x4fc3f7), 0);
  lv_obj_set_pos(cpu_title, 10, 5);

  cpu_name_label = lv_label_create(cpu_panel);
  lv_label_set_text(cpu_name_label, "Unknown CPU");
  lv_obj_set_style_text_font(cpu_name_label, font_small, 0);
  lv_obj_set_style_text_color(cpu_name_label, lv_color_hex(0xcccccc), 0);
  lv_obj_set_pos(cpu_name_label, 60, 5);

  // Create CPU usage progress bar
  create_progress_section(cpu_panel, "Usage", &cpu_usage_bar, &cpu_usage_label, 10, 25, 140);

  cpu_temp_label = lv_label_create(cpu_panel);
  lv_label_set_text(cpu_temp_label, "Temp: --°C");
  lv_obj_set_style_text_font(cpu_temp_label, font_small, 0);
  lv_obj_set_style_text_color(cpu_temp_label, lv_color_white(), 0);
  lv_obj_set_pos(cpu_temp_label, 10, 70);

  // ─────────────────────────────────────────────────────────────────
  // GPU Section Container Setup
  // ─────────────────────────────────────────────────────────────────

  lv_obj_t *gpu_panel = lv_obj_create(screen);
  lv_obj_set_size(gpu_panel, 360, 130);
  lv_obj_set_pos(gpu_panel, 10, 170);
  lv_obj_set_style_bg_color(gpu_panel, lv_color_hex(0x1a2e1a), 0); // Dark green theme
  lv_obj_set_style_border_color(gpu_panel, lv_color_hex(0x2e4f2e), 0);
  lv_obj_set_style_border_width(gpu_panel, 2, 0);
  lv_obj_set_style_radius(gpu_panel, 8, 0);

  // ─────────────────────────────────────────────────────────────────
  // GPU Section UI Elements
  // ─────────────────────────────────────────────────────────────────

  lv_obj_t *gpu_title = lv_label_create(gpu_panel);
  lv_label_set_text(gpu_title, "🎮 GPU");
  lv_obj_set_style_text_color(gpu_title, lv_color_hex(0x4caf50), 0);
  lv_obj_set_pos(gpu_title, 10, 5);

  gpu_name_label = lv_label_create(gpu_panel);
  lv_label_set_text(gpu_name_label, "Unknown GPU");
  lv_obj_set_style_text_font(gpu_name_label, font_small, 0);
  lv_obj_set_style_text_color(gpu_name_label, lv_color_hex(0xcccccc), 0);
  lv_obj_set_pos(gpu_name_label, 60, 5);

  // Create GPU usage and memory progress bars
  create_progress_section(gpu_panel, "Usage", &gpu_usage_bar, &gpu_usage_label, 10, 25, 140);
  create_progress_section(gpu_panel, "Memory", &gpu_mem_bar, &gpu_mem_label, 10, 65, 140);

  gpu_temp_label = lv_label_create(gpu_panel);
  lv_label_set_text(gpu_temp_label, "Temp: --°C");
  lv_obj_set_style_text_font(gpu_temp_label, font_small, 0);
  lv_obj_set_style_text_color(gpu_temp_label, lv_color_white(), 0);
  lv_obj_set_pos(gpu_temp_label, 10, 105);

  // ─────────────────────────────────────────────────────────────────
  // Memory Section Container Setup
  // ─────────────────────────────────────────────────────────────────

  lv_obj_t *mem_panel = lv_obj_create(screen);
  lv_obj_set_size(mem_panel, 360, 100);
  lv_obj_set_pos(mem_panel, 10, 310);
  lv_obj_set_style_bg_color(mem_panel, lv_color_hex(0x2e1a1a), 0); // Dark red theme
  lv_obj_set_style_border_color(mem_panel, lv_color_hex(0x4f2e2e), 0);
  lv_obj_set_style_border_width(mem_panel, 2, 0);
  lv_obj_set_style_radius(mem_panel, 8, 0);

  // ─────────────────────────────────────────────────────────────────
  // Memory Section UI Elements
  // ─────────────────────────────────────────────────────────────────

  lv_obj_t *mem_title = lv_label_create(mem_panel);
  lv_label_set_text(mem_title, "💾 Memory");
  lv_obj_set_style_text_color(mem_title, lv_color_hex(0xff7043), 0);
  lv_obj_set_pos(mem_title, 10, 5);

  // Create memory usage progress bar
  create_progress_section(mem_panel, "Usage", &mem_usage_bar, &mem_usage_label, 10, 25, 140);

  mem_info_label = lv_label_create(mem_panel);
  lv_label_set_text(mem_info_label, "Used: -.-- GB / Total: -.-- GB");
  lv_obj_set_style_text_font(mem_info_label, font_small, 0);
  lv_obj_set_style_text_color(mem_info_label, lv_color_white(), 0);
  lv_obj_set_pos(mem_info_label, 10, 70);

  // ─────────────────────────────────────────────────────────────────
  // Information Panel Container Setup
  // ─────────────────────────────────────────────────────────────────

  lv_obj_t *info_panel = lv_obj_create(screen);
  lv_obj_set_size(info_panel, 410, 350);
  lv_obj_set_pos(info_panel, 380, 60);
  lv_obj_set_style_bg_color(info_panel, lv_color_hex(0x2e2e2e), 0); // Dark grey theme
  lv_obj_set_style_border_color(info_panel, lv_color_hex(0x555555), 0);
  lv_obj_set_style_border_width(info_panel, 2, 0);
  lv_obj_set_style_radius(info_panel, 10, 0);

  // ─────────────────────────────────────────────────────────────────
  // Information Panel UI Elements
  // ─────────────────────────────────────────────────────────────────

  lv_obj_t *info_title = lv_label_create(info_panel);
  lv_label_set_text(info_title, "📊 Live Data Feed");
  lv_obj_set_style_text_color(info_title, lv_color_hex(0xffa726), 0);
  lv_obj_set_pos(info_title, 10, 10);

  lv_obj_t *info_text = lv_label_create(info_panel);
  lv_label_set_text(info_text,
                    "ESP32-S3-8048S050\n"
                    "5.0\" 800x480 RGB LCD\n"
                    "Real-time System Monitor\n\n"
                    "Receiving JSON data via\n"
                    "Serial UART at 115200 baud\n\n"
                    "Updates: Every 1 second\n"
                    "Protocol: JSON over Serial\n\n"
                    "Features:\n"
                    "• CPU monitoring\n"
                    "• GPU monitoring\n"
                    "• Memory usage\n"
                    "• Temperature readings\n"
                    "• Connection status\n"
                    "• Timestamp tracking");
  lv_obj_set_style_text_color(info_text, lv_color_white(), 0);
  lv_obj_set_style_text_font(info_text, font_small, 0);
  lv_obj_set_pos(info_text, 10, 40);

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
  // Update Timestamp Display
  // ─────────────────────────────────────────────────────────────────

  if (timestamp_label)
  {
    time_t timestamp_sec = data->timestamp / 1000;
    struct tm *timeinfo = localtime(&timestamp_sec);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "Last: %H:%M:%S", timeinfo);
    lv_label_set_text(timestamp_label, time_str);
  }

  // ─────────────────────────────────────────────────────────────────
  // Update CPU Section
  // ─────────────────────────────────────────────────────────────────

  if (cpu_name_label)
  {
    lv_label_set_text(cpu_name_label, data->cpu.name);
  }

  if (cpu_usage_bar && cpu_usage_label)
  {
    lv_bar_set_value(cpu_usage_bar, data->cpu.usage, LV_ANIM_ON);
    char usage_str[16];
    snprintf(usage_str, sizeof(usage_str), "%d%%", data->cpu.usage);
    lv_label_set_text(cpu_usage_label, usage_str);
  }

  if (cpu_temp_label)
  {
    char temp_str[32];
    snprintf(temp_str, sizeof(temp_str), "Temp: %d°C", data->cpu.temp);
    lv_label_set_text(cpu_temp_label, temp_str);
  }

  // ─────────────────────────────────────────────────────────────────
  // Update GPU Section
  // ─────────────────────────────────────────────────────────────────

  if (gpu_name_label)
  {
    lv_label_set_text(gpu_name_label, data->gpu.name);
  }

  if (gpu_usage_bar && gpu_usage_label)
  {
    lv_bar_set_value(gpu_usage_bar, data->gpu.usage, LV_ANIM_ON);
    char usage_str[16];
    snprintf(usage_str, sizeof(usage_str), "%d%%", data->gpu.usage);
    lv_label_set_text(gpu_usage_label, usage_str);
  }

  if (gpu_temp_label)
  {
    char temp_str[32];
    snprintf(temp_str, sizeof(temp_str), "Temp: %d°C", data->gpu.temp);
    lv_label_set_text(gpu_temp_label, temp_str);
  }

  if (gpu_mem_bar && gpu_mem_label)
  {
    uint8_t mem_usage_pct = (data->gpu.mem_used * 100) / data->gpu.mem_total;
    lv_bar_set_value(gpu_mem_bar, mem_usage_pct, LV_ANIM_ON);
    char mem_str[32];
    snprintf(mem_str, sizeof(mem_str), "%d%%", mem_usage_pct);
    lv_label_set_text(gpu_mem_label, mem_str);
  }

  // ─────────────────────────────────────────────────────────────────
  // Update Memory Section
  // ─────────────────────────────────────────────────────────────────

  if (mem_usage_bar && mem_usage_label)
  {
    lv_bar_set_value(mem_usage_bar, data->mem.usage, LV_ANIM_ON);
    char usage_str[16];
    snprintf(usage_str, sizeof(usage_str), "%d%%", data->mem.usage);
    lv_label_set_text(mem_usage_label, usage_str);
  }

  if (mem_info_label)
  {
    char mem_str[64];
    snprintf(mem_str, sizeof(mem_str), "Used: %.1f GB / Total: %.1f GB",
             data->mem.used, data->mem.total);
    lv_label_set_text(mem_info_label, mem_str);
  }

  // ─────────────────────────────────────────────────────────────────
  // Release LVGL Mutex and Log Update
  // ─────────────────────────────────────────────────────────────────

  lvgl_lock_release();

  ESP_LOGI(TAG, "UI updated - CPU: %d%%, GPU: %d%%, MEM: %d%%",
           data->cpu.usage, data->gpu.usage, data->mem.usage);
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

  if (connected)
  {
    lv_label_set_text(connection_status_label, "● Connected");
    lv_obj_set_style_text_color(connection_status_label, lv_color_hex(0x00ff88), 0); // Green
  }
  else
  {
    lv_label_set_text(connection_status_label, "● Connection Lost");
    lv_obj_set_style_text_color(connection_status_label, lv_color_hex(0xff4444), 0); // Red
  }

  lvgl_lock_release();
}
