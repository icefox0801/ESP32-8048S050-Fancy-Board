
/**
 * @file ui_memory_panel.c
 * @brief Memory Monitoring Panel Implementation
 *
 * Creates the memory monitoring panel with usage bar and statistics
 * for the ESP32-S3-8048S050 system monitor dashboard.
 */

#include "ui_memory_panel.h"
#include "ui_config.h"
#include "ui_helpers.h"
#include <stdio.h>

// External references to global UI elements
extern lv_obj_t *mem_usage_bar;
extern lv_obj_t *mem_usage_label;
extern lv_obj_t *mem_info_label;

/**
 * @brief Create the memory monitoring panel
 * @param parent Parent screen object
 * @return Created memory panel
 */
lv_obj_t *create_memory_panel(lv_obj_t *parent)
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
 * @brief Update memory panel with new data
 * @param memory_data Pointer to memory monitoring data from system_data_t
 */
void update_memory_panel(const void *memory_data)
{
  if (!memory_data)
    return;

  // Cast void* to the actual struct type
  const struct memory_info *mem = (const struct memory_info *)memory_data;

  if (mem_usage_bar && mem_usage_label)
  {
    lv_bar_set_value(mem_usage_bar, mem->usage, LV_ANIM_OFF); // Disable animation for instant updates
    char usage_str[16];
    snprintf(usage_str, sizeof(usage_str), "%d%%", mem->usage);
    lv_label_set_text(mem_usage_label, usage_str);
  }

  if (mem_info_label)
  {
    char mem_str[32];
    snprintf(mem_str, sizeof(mem_str), "(%.1f GB / %.1f GB)", // Updated format to match new compact layout
             mem->used, mem->total);
    lv_label_set_text(mem_info_label, mem_str);
  }
}
