/**
 * @file ui_helpers.h
 * @brief Common UI Helper Functions for ESP32-S3-8048S050 Dashboard
 *
 * This header provides reusable UI creation functions for building
 * consistent panels, separators, switches, and other UI elements
 * across the system monitor dashboard.
 */

#pragma once

#include "lvgl.h"
#include <stdint.h>

// =======================================================================
// UI HELPER FUNCTION DECLARATIONS
// =======================================================================

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
                                const lv_font_t *font, uint32_t color);

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
                          uint32_t bg_color, uint32_t border_color);

/**
 * @brief Create a title label with separator line
 * @param parent Parent panel
 * @param title Title text
 * @param title_color Title color (hex)
 * @param separator_width Width of separator line
 * @return Created title label object
 */
lv_obj_t *ui_create_title_with_separator(lv_obj_t *parent, const char *title,
                                         uint32_t title_color, int separator_width);

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
                          uint32_t label_color, uint32_t value_color);

/**
 * @brief Create a vertical separator line
 * @param parent Parent panel
 * @param x X position
 * @param y Y position
 * @param height Height of separator
 * @param color Color (hex)
 * @return Created separator object
 */
lv_obj_t *ui_create_vertical_separator(lv_obj_t *parent, int x, int y, int height, uint32_t color);

/**
 * @brief Create a vertically centered separator using alignment API
 * @param parent Parent panel
 * @param x X position
 * @param height Separator height
 * @param color Separator color (hex)
 * @return Created separator object
 */
lv_obj_t *ui_create_centered_vertical_separator(lv_obj_t *parent, int x, int height, uint32_t color);

/**
 * @brief Create a switch field with label above it, positioned and aligned together
 * @param parent Parent panel
 * @param label_text Text for the label above the switch
 * @param x_offset X position offset from left edge
 * @return Created switch object
 */
lv_obj_t *ui_create_switch_field(lv_obj_t *parent, const char *label_text, int x_offset);

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
                                 uint32_t bg_color, uint32_t indicator_color, int radius);

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
                                 uint32_t bg_color, uint32_t border_color);
