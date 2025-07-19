/**
 * @file ui_helpers.c
 * @brief Common UI Helper Functions Implementation
 *
 * Provides reusable UI creation functions for building consistent
 * panels, separators, switches, and other UI elements across the
 * system monitor dashboard.
 */

#include "ui_helpers.h"
#include "ui_config.h"

// ═══════════════════════════════════════════════════════════════════════════════
// FONT DEFINITIONS WITH FALLBACK
// ═══════════════════════════════════════════════════════════════════════════════
#ifdef CONFIG_LV_FONT_MONTSERRAT_28
const lv_font_t *font_title = &lv_font_montserrat_28; // Large title font (28px)
#else
const lv_font_t *font_title = &lv_font_montserrat_14; // Fallback to 14px
#endif

#ifdef CONFIG_LV_FONT_MONTSERRAT_16
const lv_font_t *font_normal = &lv_font_montserrat_16; // Normal text
#else
const lv_font_t *font_normal = &lv_font_montserrat_14; // Fallback to 14px
#endif

const lv_font_t *font_small = &lv_font_montserrat_14; // Small text

#ifdef CONFIG_LV_FONT_MONTSERRAT_32
const lv_font_t *font_big_numbers = &lv_font_montserrat_32; // Large numbers (32px)
#else
const lv_font_t *font_big_numbers = &lv_font_montserrat_14; // Fallback to 14px
#endif

// ═══════════════════════════════════════════════════════════════════════════════
// HELPER FUNCTIONS FOR UI CREATION
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Create a device name label with consistent styling
 * @param parent Parent panel
 * @param device_name Device name text (e.g., "ESP32-S3", "GPU")
 * @param x X position
 * @param font Font to use
 * @param color Color (hex)
 * @return Created device name label object
 */
lv_obj_t *ui_create_device_name(lv_obj_t *parent, const char *device_name, int x,
                                const lv_font_t *font, uint32_t color)
{
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, device_name);
  lv_obj_set_style_text_font(label, font, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
  lv_obj_set_pos(label, x, 8); // Fixed Y position of 8
  return label;
}

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
lv_obj_t *ui_create_panel(lv_obj_t *parent, int width, int height, int x, int y,
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
lv_obj_t *ui_create_title_with_separator(lv_obj_t *parent, const char *title,
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
lv_obj_t *ui_create_field(lv_obj_t *parent, const char *field_name, const char *default_value,
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
lv_obj_t *ui_create_vertical_separator(lv_obj_t *parent, int x, int y, int height, uint32_t color)
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
lv_obj_t *ui_create_centered_vertical_separator(lv_obj_t *parent, int x, int height, uint32_t color)
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
lv_obj_t *ui_create_switch_field(lv_obj_t *parent, const char *label_text, int x_offset)
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
lv_obj_t *ui_create_progress_bar(lv_obj_t *parent, int width, int height, int x, int y,
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
lv_obj_t *ui_create_status_panel(lv_obj_t *parent, int width, int height, int x, int y,
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
