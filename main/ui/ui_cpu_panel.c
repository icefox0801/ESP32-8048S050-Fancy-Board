
/**
 * @file ui_cpu_panel.c
 * @brief CPU Monitoring Panel Implementation
 *
 * Creates the CPU monitoring panel with usage, temperature, and fan speed
 * displays for the ESP32-S3-8048S050 system monitor dashboard.
 */

#include "ui_cpu_panel.h"
#include "ui_config.h"
#include "ui_helpers.h"
#include <stdio.h>

static lv_obj_t *cpu_name_label = NULL;
static lv_obj_t *cpu_usage_label = NULL;
static lv_obj_t *cpu_temp_label = NULL;
static lv_obj_t *cpu_fan_label = NULL;

/**
 * @brief Create the CPU monitoring panel
 * @param parent Parent screen object
 * @return Created CPU panel
 */
lv_obj_t *create_cpu_panel(lv_obj_t *parent)
{
  lv_obj_t *cpu_panel = ui_create_panel(parent, 385, 150, 10, 120, 0x1a1a2e, 0x16213e);

  // CPU Title with separator
  ui_create_title_with_separator(cpu_panel, "CPU", 0x4fc3f7, 355);

  // CPU Name (positioned to the right of title, baseline aligned)
  cpu_name_label = ui_create_device_name(cpu_panel, "Unknown CPU", 80, font_small, 0x808080);

  // Create CPU fields - Temperature first
  cpu_temp_label = ui_create_field(cpu_panel, "Temp", "--°C", 10, font_normal, font_big_numbers, 0xaaaaaa, 0xff7043);
  cpu_usage_label = ui_create_field(cpu_panel, "Usage", "--%", 128, font_normal, font_big_numbers, 0xaaaaaa, 0x4fc3f7);
  cpu_fan_label = ui_create_field(cpu_panel, "Fan (RPM)", "--", 246, font_normal, font_big_numbers, 0xaaaaaa, 0x81c784);

  // Create vertical separators between fields
  ui_create_vertical_separator(cpu_panel, 118, 50, 60, 0x555555);
  ui_create_vertical_separator(cpu_panel, 236, 50, 60, 0x555555);

  return cpu_panel;
}

/**
 * @brief Update CPU panel with new data
 * @param cpu_data Pointer to CPU monitoring data from system_data_t
 */
void update_cpu_panel(const void *cpu_data)
{
  if (!cpu_data)
  {
    debug_log_warning("ui_cpu_panel", "⚠️ CPU panel update called with NULL data");
    return;
  }

  // Cast void* to the actual struct type
  const struct cpu_info *cpu = (const struct cpu_info *)cpu_data;

  debug_log_debug_f("ui_cpu_panel", "🔄 Updating CPU panel - Name: '%s', Usage: %d%%, Temp: %d°C, Fan: %d RPM", 
                    cpu->name, cpu->usage, cpu->temp, cpu->fan);

  if (cpu_name_label)
  {
    lv_label_set_text(cpu_name_label, cpu->name);
    debug_log_debug_f("ui_cpu_panel", "📝 CPU name set to: '%s'", cpu->name);
  }
  else
  {
    debug_log_error("ui_cpu_panel", "❌ CPU name label is NULL!");
  }

  if (cpu_usage_label)
  {
    char usage_str[16];
    snprintf(usage_str, sizeof(usage_str), "%d%%", cpu->usage);
    lv_label_set_text(cpu_usage_label, usage_str);
  }

  if (cpu_temp_label)
  {
    char temp_str[16];
    snprintf(temp_str, sizeof(temp_str), "%d°C", cpu->temp);
    lv_label_set_text(cpu_temp_label, temp_str);
  }

  if (cpu_fan_label)
  {
    char fan_str[16];
    snprintf(fan_str, sizeof(fan_str), "%d", cpu->fan);
    lv_label_set_text(cpu_fan_label, fan_str);
  }
  
  debug_log_debug("ui_cpu_panel", "✅ CPU panel update completed");
}

/**
 * @brief Reset CPU panel to default values (no connection)
 */
void reset_cpu_panel(void)
{
  if (cpu_name_label)
  {
    lv_label_set_text(cpu_name_label, "No Connection");
  }

  if (cpu_usage_label)
  {
    lv_label_set_text(cpu_usage_label, "--%");
  }

  if (cpu_temp_label)
  {
    lv_label_set_text(cpu_temp_label, "--°C");
  }

  if (cpu_fan_label)
  {
    lv_label_set_text(cpu_fan_label, "--");
  }
}
