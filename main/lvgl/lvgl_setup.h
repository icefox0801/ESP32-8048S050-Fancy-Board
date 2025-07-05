#pragma once

#include "esp_lcd_panel_ops.h"
#include "lvgl.h"
#include "sdkconfig.h"

// LCD panel configuration for ESP32-8048S050
// Refresh Rate = 20000000/(8+40+20+800)/(8+10+5+480) =
#define LCD_PIXEL_CLOCK_HZ (15 * 1000 * 1000)
#define LCD_H_RES 800
#define LCD_V_RES 480
#define LCD_HSYNC 8
#define LCD_HBP 40
#define LCD_HFP 20
#define LCD_VSYNC 8
#define LCD_VBP 10
#define LCD_VFP 5

// Backlight control
#define LCD_BK_LIGHT_ON_LEVEL 1
#define LCD_BK_LIGHT_OFF_LEVEL 0
#define PIN_NUM_BK_LIGHT 2 // ESP32-8048S050: GPIO2 for backlight PWM control
#define PIN_NUM_DISP_EN -1

// GPIO pin assignments using CONFIG constants
#define PIN_NUM_HSYNC CONFIG_EXAMPLE_LCD_HSYNC_GPIO
#define PIN_NUM_VSYNC CONFIG_EXAMPLE_LCD_VSYNC_GPIO
#define PIN_NUM_DE CONFIG_EXAMPLE_LCD_DE_GPIO
#define PIN_NUM_PCLK CONFIG_EXAMPLE_LCD_PCLK_GPIO

// RGB data pins using CONFIG constants
#define PIN_NUM_DATA0 CONFIG_EXAMPLE_LCD_DATA0_GPIO
#define PIN_NUM_DATA1 CONFIG_EXAMPLE_LCD_DATA1_GPIO
#define PIN_NUM_DATA2 CONFIG_EXAMPLE_LCD_DATA2_GPIO
#define PIN_NUM_DATA3 CONFIG_EXAMPLE_LCD_DATA3_GPIO
#define PIN_NUM_DATA4 CONFIG_EXAMPLE_LCD_DATA4_GPIO
#define PIN_NUM_DATA5 CONFIG_EXAMPLE_LCD_DATA5_GPIO
#define PIN_NUM_DATA6 CONFIG_EXAMPLE_LCD_DATA6_GPIO
#define PIN_NUM_DATA7 CONFIG_EXAMPLE_LCD_DATA7_GPIO
#define PIN_NUM_DATA8 CONFIG_EXAMPLE_LCD_DATA8_GPIO
#define PIN_NUM_DATA9 CONFIG_EXAMPLE_LCD_DATA9_GPIO
#define PIN_NUM_DATA10 CONFIG_EXAMPLE_LCD_DATA10_GPIO
#define PIN_NUM_DATA11 CONFIG_EXAMPLE_LCD_DATA11_GPIO
#define PIN_NUM_DATA12 CONFIG_EXAMPLE_LCD_DATA12_GPIO
#define PIN_NUM_DATA13 CONFIG_EXAMPLE_LCD_DATA13_GPIO
#define PIN_NUM_DATA14 CONFIG_EXAMPLE_LCD_DATA14_GPIO
#define PIN_NUM_DATA15 CONFIG_EXAMPLE_LCD_DATA15_GPIO

// Display configuration using CONFIG constants
#if CONFIG_EXAMPLE_LCD_DATA_LINES_16
#define LCD_DATA_BUS_WIDTH 16
#define LCD_PIXEL_SIZE 2
#define LCD_COLOR_FORMAT LV_COLOR_FORMAT_RGB565
#elif CONFIG_EXAMPLE_LCD_DATA_LINES_24
#define LCD_DATA_BUS_WIDTH 24
#define LCD_PIXEL_SIZE 3
#define LCD_COLOR_FORMAT LV_COLOR_FORMAT_RGB888
#endif

#if CONFIG_EXAMPLE_USE_DOUBLE_FB
#define LCD_NUM_FB 2
#else
#define LCD_NUM_FB 1
#endif

// LVGL configuration
#define LVGL_DRAW_BUF_LINES 20 // Reduced from 50 to 20 lines for better memory efficiency
#define LVGL_TICK_PERIOD_MS 2
#define LVGL_TASK_STACK_SIZE (12 * 1024) // Increased from 8KB to 12KB for stability
#define LVGL_TASK_PRIORITY 2

/**
 * @brief Initialize LVGL with LCD panel
 * @param panel_handle LCD panel handle
 * @return LVGL display handle
 */
lv_display_t *lvgl_setup_init(esp_lcd_panel_handle_t panel_handle);

/**
 * @brief Start LVGL task
 */
void lvgl_setup_start_task(void);

/**
 * @brief Initialize and turn on LCD backlight
 */
void lvgl_setup_init_backlight(void);

/**
 * @brief Set LCD backlight level
 * @param level Backlight level (0 = off, 1 = on)
 */
void lvgl_setup_set_backlight(uint32_t level);

/**
 * @brief Create and configure LCD RGB panel
 * @return LCD panel handle
 */
esp_lcd_panel_handle_t lvgl_setup_create_lcd_panel(void);

/**
 * @brief Initialize GT911 touch input device for LVGL
 * @return LVGL input device handle
 */
lv_indev_t *lvgl_setup_init_touch(void);

/**
 * @brief Create UI with thread safety and logging
 * @param display LVGL display handle
 * @param ui_create_func UI creation function to call
 */
void lvgl_setup_create_ui_safe(lv_display_t *display, void (*ui_create_func)(lv_display_t *));

/**
 * @brief Acquire LVGL API lock (for thread safety)
 */
void lvgl_lock_acquire(void);

/**
 * @brief Release LVGL API lock (for thread safety)
 */
void lvgl_lock_release(void);
