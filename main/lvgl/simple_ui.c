#include "simple_ui.h"

#include "esp_log.h"
#include "lvgl.h"
#include "lvgl_setup.h"

static const lv_font_t *font_normal = &lv_font_montserrat_14;

void simple_ui_create(lv_display_t *disp)
{
  // Initialize default theme
  lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                        LV_THEME_DEFAULT_DARK, font_normal);

  lv_obj_t *screen = lv_display_get_screen_active(disp);

  // Create a title label
  lv_obj_t *title_label = lv_label_create(screen);
  lv_label_set_text(title_label, "ESP32-S3 Simple UI");
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(title_label, lv_color_white(), 0);
  lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 20);

  // Create status label
  lv_obj_t *status_label = lv_label_create(screen);
  lv_label_set_text(status_label, "System Ready");
  lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(status_label, lv_color_hex(0x00ff00), 0);
  lv_obj_align(status_label, LV_ALIGN_CENTER, 0, -50);

  // Create info panel
  lv_obj_t *info_panel = lv_obj_create(screen);
  lv_obj_set_size(info_panel, 300, 150);
  lv_obj_align(info_panel, LV_ALIGN_CENTER, 0, 50);
  lv_obj_set_style_bg_color(info_panel, lv_color_hex(0x2e2e2e), 0);
  lv_obj_set_style_border_color(info_panel, lv_color_hex(0x555555), 0);
  lv_obj_set_style_border_width(info_panel, 2, 0);
  lv_obj_set_style_radius(info_panel, 10, 0);

  // Add info text
  lv_obj_t *info_label = lv_label_create(info_panel);
  lv_label_set_text(info_label,
                    "Display: 800x480\n"
                    "Color: 16-bit RGB\n"
                    "Touch: GT911 Capacitive\n"
                    "MCU: ESP32-S3");
  lv_obj_set_style_text_color(info_label, lv_color_white(), 0);
  lv_obj_center(info_label);

  // Create progress bar
  lv_obj_t *progress_bar = lv_bar_create(screen);
  lv_obj_set_size(progress_bar, 200, 20);
  lv_obj_align(progress_bar, LV_ALIGN_BOTTOM_MID, 0, -30);
  lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x333333), LV_PART_MAIN);
  lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x00aaff), LV_PART_INDICATOR);

  // Set initial value
  lv_bar_set_value(progress_bar, 75, LV_ANIM_ON);

  ESP_LOGI("UI", "Simple UI created successfully");
}
