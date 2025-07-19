
/**
 * @file ui_gpu_panel.c
 * @brief GPU Monitoring Panel Implementation
 *
 * Creates the GPU monitoring panel with usage, temperature, and memory
 * displays for the ESP32-S3-8048S050 system monitor dashboard.
 */

#include "ui_gpu_panel.h"
#include "ui_config.h"
#include "ui_helpers.h"
#include <stdio.h>

static lv_obj_t *gpu_name_label = NULL;
static lv_obj_t *gpu_usage_label = NULL;
static lv_obj_t *gpu_temp_label = NULL;
static lv_obj_t *gpu_mem_label = NULL;

/**
 * @brief Create the GPU monitoring panel
 * @param parent Parent screen object
 * @return Created GPU panel
 */
lv_obj_t *create_gpu_panel(lv_obj_t *parent)
{
  lv_obj_t *gpu_panel = ui_create_panel(parent, 385, 150, 405, 120, 0x1a2e1a, 0x2e4f2e);

  // GPU Title with separator
  ui_create_title_with_separator(gpu_panel, "GPU", 0x4caf50, 355);

  // GPU Name (positioned to the right of title, baseline aligned)
  gpu_name_label = ui_create_device_name(gpu_panel, "Unknown GPU", 80, font_small, 0x808080);

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
 * @brief Update GPU panel with new data
 * @param gpu_data Pointer to GPU monitoring data from system_data_t
 */
void update_gpu_panel(const void *gpu_data)
{
  if (!gpu_data)
    return;

  // Cast void* to the actual struct type
  const struct gpu_info *gpu = (const struct gpu_info *)gpu_data;

  if (gpu_name_label)
  {
    lv_label_set_text(gpu_name_label, gpu->name);
  }

  if (gpu_usage_label)
  {
    char usage_str[16];
    snprintf(usage_str, sizeof(usage_str), "%d%%", gpu->usage);
    lv_label_set_text(gpu_usage_label, usage_str);
  }

  if (gpu_temp_label)
  {
    char temp_str[16];
    snprintf(temp_str, sizeof(temp_str), "%d°C", gpu->temp);
    lv_label_set_text(gpu_temp_label, temp_str);
  }

  if (gpu_mem_label)
  {
    uint8_t mem_usage_pct = (gpu->mem_used * 100) / gpu->mem_total;
    char mem_str[32];
    snprintf(mem_str, sizeof(mem_str), "%d%%", mem_usage_pct);
    lv_label_set_text(gpu_mem_label, mem_str);
  }
}
