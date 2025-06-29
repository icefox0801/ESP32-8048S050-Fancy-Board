/**
 * @file lvgl_setup.c
 * @brief LVGL setup and LCD panel initialization for ESP32-S3
 */

#include "lvgl_setup.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gt911_touch.h"
#include <stdio.h>
#include <string.h>
#include <sys/lock.h>
#include <unistd.h>

static const char *TAG = "lvgl_setup";
static _lock_t lvgl_api_lock;

// Static function prototypes (ordered by call sequence)
static esp_err_t init_panel_config(esp_lcd_rgb_panel_config_t *panel_config);
static bool lvgl_notify_flush_ready(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_ctx);
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
static void lvgl_increase_tick(void *arg);
static void lvgl_port_task(void *arg);

// 1. Backlight functions (called first)
void lvgl_setup_init_backlight(void)
{
#if PIN_NUM_BK_LIGHT >= 0
  gpio_config_t bk_gpio_config = {
      .mode = GPIO_MODE_OUTPUT,
      .pin_bit_mask = 1ULL << PIN_NUM_BK_LIGHT};
  ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
#endif
}

void lvgl_setup_set_backlight(uint32_t level)
{
#if PIN_NUM_BK_LIGHT >= 0
  gpio_set_level(PIN_NUM_BK_LIGHT, level);
  if (level == LCD_BK_LIGHT_ON_LEVEL)
  {
    ESP_LOGI(TAG, "LCD backlight turned ON");
  }
  else if (level == LCD_BK_LIGHT_OFF_LEVEL)
  {
    ESP_LOGI(TAG, "LCD backlight turned OFF");
  }
#endif
}

// 2. LCD Panel creation (called second)
esp_lcd_panel_handle_t lvgl_setup_create_lcd_panel(void)
{
  esp_lcd_panel_handle_t panel_handle = NULL;
  esp_lcd_rgb_panel_config_t panel_config;

  ESP_ERROR_CHECK(init_panel_config(&panel_config));
  ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

  return panel_handle;
}

// 3. LVGL initialization (called third)
lv_display_t *lvgl_setup_init(esp_lcd_panel_handle_t panel_handle)
{
  lv_init();

  lv_display_t *display = lv_display_create(LCD_H_RES, LCD_V_RES);
  if (!display)
  {
    ESP_LOGE(TAG, "Failed to create LVGL display");
    return NULL;
  }

  lv_display_set_user_data(display, panel_handle);
  lv_display_set_color_format(display, LCD_COLOR_FORMAT);

  // Setup display buffers
  void *buf1 = NULL;
  void *buf2 = NULL;

#if CONFIG_EXAMPLE_USE_DOUBLE_FB
  ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 2, &buf1, &buf2));
  lv_display_set_buffers(display, buf1, buf2, LCD_H_RES * LCD_V_RES * LCD_PIXEL_SIZE, LV_DISPLAY_RENDER_MODE_DIRECT);
#else
  size_t draw_buffer_sz = LCD_H_RES * LVGL_DRAW_BUF_LINES * LCD_PIXEL_SIZE;
  buf1 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!buf1)
  {
    ESP_LOGE(TAG, "Failed to allocate LVGL draw buffer");
    return NULL;
  }

  ESP_LOGI(TAG, "LVGL draw buffer allocated: %zu bytes at %p", draw_buffer_sz, buf1);
  lv_display_set_buffers(display, buf1, buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
#endif

  lv_display_set_flush_cb(display, lvgl_flush_cb);

  // Register callbacks
  esp_lcd_rgb_panel_event_callbacks_t cbs = {
      .on_color_trans_done = lvgl_notify_flush_ready,
  };
  ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, display));

  // Setup tick timer
  const esp_timer_create_args_t lvgl_tick_timer_args = {
      .callback = &lvgl_increase_tick,
      .name = "lvgl_tick"};
  esp_timer_handle_t lvgl_tick_timer = NULL;
  ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

  return display;
}

// 4. Task management (called fourth)
void lvgl_setup_start_task(void)
{
  xTaskCreate(lvgl_port_task, "LVGL", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL);
}

// 5. UI creation helper (called fifth)
void lvgl_setup_create_ui_safe(lv_display_t *display, void (*ui_create_func)(lv_display_t *))
{
  if (!display || !ui_create_func)
  {
    return;
  }

  _lock_acquire(&lvgl_api_lock);
  ui_create_func(display);
  _lock_release(&lvgl_api_lock);
}

// Thread safety functions
void lvgl_lock_acquire(void)
{
  _lock_acquire(&lvgl_api_lock);
}

void lvgl_lock_release(void)
{
  _lock_release(&lvgl_api_lock);
}

// Static functions (implementation details)
static esp_err_t init_panel_config(esp_lcd_rgb_panel_config_t *panel_config)
{
  memset(panel_config, 0, sizeof(esp_lcd_rgb_panel_config_t));

  panel_config->data_width = LCD_DATA_BUS_WIDTH;
  panel_config->dma_burst_size = 64;
  panel_config->num_fbs = LCD_NUM_FB;
  panel_config->clk_src = LCD_CLK_SRC_DEFAULT;
  panel_config->flags.fb_in_psram = true;

#if CONFIG_EXAMPLE_USE_BOUNCE_BUFFER
  panel_config->bounce_buffer_size_px = 20 * LCD_H_RES;
#endif

  // GPIO pins
  panel_config->disp_gpio_num = PIN_NUM_DISP_EN;
  panel_config->pclk_gpio_num = PIN_NUM_PCLK;
  panel_config->vsync_gpio_num = PIN_NUM_VSYNC;
  panel_config->hsync_gpio_num = PIN_NUM_HSYNC;
  panel_config->de_gpio_num = PIN_NUM_DE;

  // Data pins
  panel_config->data_gpio_nums[0] = PIN_NUM_DATA0;
  panel_config->data_gpio_nums[1] = PIN_NUM_DATA1;
  panel_config->data_gpio_nums[2] = PIN_NUM_DATA2;
  panel_config->data_gpio_nums[3] = PIN_NUM_DATA3;
  panel_config->data_gpio_nums[4] = PIN_NUM_DATA4;
  panel_config->data_gpio_nums[5] = PIN_NUM_DATA5;
  panel_config->data_gpio_nums[6] = PIN_NUM_DATA6;
  panel_config->data_gpio_nums[7] = PIN_NUM_DATA7;
  panel_config->data_gpio_nums[8] = PIN_NUM_DATA8;
  panel_config->data_gpio_nums[9] = PIN_NUM_DATA9;
  panel_config->data_gpio_nums[10] = PIN_NUM_DATA10;
  panel_config->data_gpio_nums[11] = PIN_NUM_DATA11;
  panel_config->data_gpio_nums[12] = PIN_NUM_DATA12;
  panel_config->data_gpio_nums[13] = PIN_NUM_DATA13;
  panel_config->data_gpio_nums[14] = PIN_NUM_DATA14;
  panel_config->data_gpio_nums[15] = PIN_NUM_DATA15;

#if CONFIG_EXAMPLE_LCD_DATA_LINES > 16
  panel_config->data_gpio_nums[16] = CONFIG_EXAMPLE_LCD_DATA16_GPIO;
  panel_config->data_gpio_nums[17] = CONFIG_EXAMPLE_LCD_DATA17_GPIO;
  panel_config->data_gpio_nums[18] = CONFIG_EXAMPLE_LCD_DATA18_GPIO;
  panel_config->data_gpio_nums[19] = CONFIG_EXAMPLE_LCD_DATA19_GPIO;
  panel_config->data_gpio_nums[20] = CONFIG_EXAMPLE_LCD_DATA20_GPIO;
  panel_config->data_gpio_nums[21] = CONFIG_EXAMPLE_LCD_DATA21_GPIO;
  panel_config->data_gpio_nums[22] = CONFIG_EXAMPLE_LCD_DATA22_GPIO;
  panel_config->data_gpio_nums[23] = CONFIG_EXAMPLE_LCD_DATA23_GPIO;
#endif

  // Timing - adjusted for ESP32-8048S050 stability
  panel_config->timings.pclk_hz = LCD_PIXEL_CLOCK_HZ;
  panel_config->timings.h_res = LCD_H_RES;
  panel_config->timings.v_res = LCD_V_RES;
  panel_config->timings.hsync_back_porch = LCD_HBP;
  panel_config->timings.hsync_front_porch = LCD_HFP;
  panel_config->timings.hsync_pulse_width = LCD_HSYNC;
  panel_config->timings.vsync_back_porch = LCD_VBP;
  panel_config->timings.vsync_front_porch = LCD_VFP;
  panel_config->timings.vsync_pulse_width = LCD_VSYNC;

  // Critical: Set correct pixel clock polarity for this display
  panel_config->timings.flags.pclk_active_neg = true;

  return ESP_OK;
}

static bool lvgl_notify_flush_ready(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_ctx)
{
  lv_display_t *disp = (lv_display_t *)user_ctx;
  lv_display_flush_ready(disp);
  return false;
}

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
  esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);
  esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map);
}

static void lvgl_increase_tick(void *arg)
{
  lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

static void lvgl_port_task(void *arg)
{
  uint32_t time_till_next_ms = 0;
  while (1)
  {
    _lock_acquire(&lvgl_api_lock);
    time_till_next_ms = lv_timer_handler();
    _lock_release(&lvgl_api_lock);

    if (time_till_next_ms < 10)
    {
      time_till_next_ms = 10;
    }

    usleep(1000 * time_till_next_ms);
  }
}

// ═══════════════════════════════════════════════════════════════════════════════
// TOUCH INPUT DEVICE SETUP
// ═══════════════════════════════════════════════════════════════════════════════

lv_indev_t *lvgl_setup_init_touch(void)
{
  ESP_LOGI(TAG, "Initializing GT911 touch controller...");

  // Initialize GT911 hardware
  esp_err_t ret = gt911_init();
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "GT911 initialization failed: %s", esp_err_to_name(ret));
    return NULL;
  }

  // Create LVGL input device for touch
  lv_indev_t *indev = lv_indev_create();
  if (!indev)
  {
    ESP_LOGE(TAG, "Failed to create LVGL input device");
    gt911_deinit();
    return NULL;
  }

  // Configure input device
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, gt911_lvgl_read);

  ESP_LOGI(TAG, "GT911 touch controller initialized successfully");
  return indev;
}
