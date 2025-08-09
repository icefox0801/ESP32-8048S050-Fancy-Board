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
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "gt911_touch.h"
#include "utils/system_debug_utils.h"
#include <stdio.h>
#include <string.h>
#include <sys/lock.h>
#include <unistd.h>

static SemaphoreHandle_t lvgl_timeout_mutex = NULL;

// Memory debugging helper function
static void log_memory_status(const char *context)
{
  size_t dram_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  size_t spiram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  debug_log_info_f(DEBUG_TAG_LVGL_SETUP, "%s - DRAM free: %zu bytes, SPIRAM free: %zu bytes",
                   context, dram_free, spiram_free);
}

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
    debug_log_info(DEBUG_TAG_LVGL_SETUP, "LCD backlight turned ON");
  }
  else if (level == LCD_BK_LIGHT_OFF_LEVEL)
  {
    debug_log_info(DEBUG_TAG_LVGL_SETUP, "LCD backlight turned OFF");
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

  log_memory_status("After LCD panel creation");

  // Calculate and log memory usage
  size_t frame_buffer_size = LCD_H_RES * LCD_V_RES * LCD_PIXEL_SIZE * LCD_NUM_FB;
  size_t draw_buffer_size = LCD_H_RES * LVGL_DRAW_BUF_LINES * LCD_PIXEL_SIZE;
  debug_log_info_f(DEBUG_TAG_LVGL_SETUP, "Memory allocation - Frame buffer: %zu KB (%d buffers), Draw buffer: %zu KB",
                   frame_buffer_size / 1024, LCD_NUM_FB, draw_buffer_size / 1024);
  debug_log_info(DEBUG_TAG_LVGL_SETUP, "LCD RGB panel created with frame buffer in SPIRAM");

  return panel_handle;
}

// 3. LVGL initialization (called third)
lv_display_t *lvgl_setup_init(esp_lcd_panel_handle_t panel_handle)
{
  debug_log_event(DEBUG_TAG_LVGL_SETUP, "Initializing LVGL display");

  /*
   * CRITICAL RENDERING STABILITY FIX:
   * - Using single frame buffer mode in SPIRAM
   * - Frame buffer in SPIRAM (~768KB), avoiding bounce buffer complexity
   * - LVGL draw buffer in DRAM (48KB)
   * - This provides stability while conserving DRAM (48KB vs 816KB)
   */

  lv_init();

  // Initialize timeout-capable mutex for LVGL locking
  if (lvgl_timeout_mutex == NULL)
  {
    lvgl_timeout_mutex = xSemaphoreCreateMutex();
    if (lvgl_timeout_mutex == NULL)
    {
      debug_log_error(DEBUG_TAG_LVGL_SETUP, "Failed to create LVGL timeout mutex");
      return NULL;
    }
    debug_log_info(DEBUG_TAG_LVGL_SETUP, "LVGL timeout mutex created successfully");
  }

  lv_display_t *display = lv_display_create(LCD_H_RES, LCD_V_RES);
  if (!display)
  {
    debug_log_error(DEBUG_TAG_LVGL_SETUP, "Failed to create LVGL display");
    return NULL;
  }

  lv_display_set_user_data(display, panel_handle);
  lv_display_set_color_format(display, LCD_COLOR_FORMAT);

  // CRITICAL FIX: Ensure proper display buffer configuration
  // Setup display buffers
  void *buf1 = NULL;
  void *buf2 = NULL;

#if CONFIG_EXAMPLE_USE_DOUBLE_FB
  ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 2, &buf1, &buf2));
  lv_display_set_buffers(display, buf1, buf2, LCD_H_RES * LCD_V_RES * LVGL_DRAW_BUF_LINES, LV_DISPLAY_RENDER_MODE_PARTIAL);
  debug_log_info(DEBUG_TAG_LVGL_SETUP, "Using double framebuffer mode");
#else
  size_t draw_buffer_sz = LCD_H_RES * LVGL_DRAW_BUF_LINES * LCD_PIXEL_SIZE;
  // CRITICAL FIX: Use internal DRAM for draw buffer to avoid rendering issues
  buf1 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!buf1)
  {
    debug_log_warning(DEBUG_TAG_LVGL_SETUP, "Failed to allocate LVGL draw buffer in DRAM, trying SPIRAM fallback");
    // Fallback to SPIRAM with smaller buffer to reduce memory pressure
    size_t fallback_buffer_sz = LCD_H_RES * (LVGL_DRAW_BUF_LINES / 2) * LCD_PIXEL_SIZE; // Half the lines for fallback
    buf1 = heap_caps_malloc(fallback_buffer_sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf1)
    {
      debug_log_error(DEBUG_TAG_LVGL_SETUP, "Failed to allocate LVGL draw buffer in both DRAM and SPIRAM");
      return NULL;
    }
    draw_buffer_sz = fallback_buffer_sz;
    debug_log_info_f(DEBUG_TAG_LVGL_SETUP, "Using fallback SPIRAM buffer: %zu bytes", draw_buffer_sz);
  }
  else
  {
    debug_log_info_f(DEBUG_TAG_LVGL_SETUP, "LVGL draw buffer allocated in DRAM: %zu bytes", draw_buffer_sz);
  }

  log_memory_status("After draw buffer allocation");

  debug_log_info_f(DEBUG_TAG_LVGL_SETUP, "LVGL draw buffer allocated: %zu bytes", draw_buffer_sz);
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
  debug_log_event(DEBUG_TAG_LVGL_SETUP, "Starting LVGL task on core 1");
  debug_log_info_f(DEBUG_TAG_LVGL_SETUP, "Creating LVGL task with priority %d, stack size %d", LVGL_TASK_PRIORITY, LVGL_TASK_STACK_SIZE);
  BaseType_t result = xTaskCreatePinnedToCore(lvgl_port_task, "LVGL", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL, 1);
  if (result == pdPASS)
  {
    debug_log_event(DEBUG_TAG_LVGL_SETUP, "LVGL task created successfully");
  }
  else
  {
    debug_log_error_f(DEBUG_TAG_LVGL_SETUP, "Failed to create LVGL task, result: %d", result);
  }
}

// 5. UI creation helper (called fifth)
void lvgl_setup_create_ui_safe(lv_display_t *display, void (*ui_create_func)(lv_display_t *))
{
  if (!display || !ui_create_func)
  {
    return;
  }

  // Use the new mutex system for consistency
  if (lvgl_port_lock(0)) // No timeout for UI creation
  {
    ui_create_func(display);
    lvgl_port_unlock();
  }
  else
  {
    debug_log_error(DEBUG_TAG_LVGL_SETUP, "Failed to acquire LVGL lock for UI creation");
  }
}

// LVGL port locking with timeout
bool lvgl_port_lock(int timeout_ms)
{
  if (lvgl_timeout_mutex == NULL)
  {
    debug_log_error(DEBUG_TAG_LVGL_SETUP, "LVGL timeout mutex not initialized!");
    return false;
  }

  if (timeout_ms <= 0)
  {
    // Use blocking semaphore for timeout <= 0
    if (xSemaphoreTake(lvgl_timeout_mutex, portMAX_DELAY) == pdTRUE)
    {
      return true;
    }
    return false;
  }

  // Use timeout-capable semaphore
  TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);
  if (xSemaphoreTake(lvgl_timeout_mutex, timeout_ticks) == pdTRUE)
  {
    return true;
  }

  debug_log_warning_f(DEBUG_TAG_LVGL_SETUP, "LVGL lock timeout after %d ms", timeout_ms);
  return false; // Timeout reached
}

void lvgl_port_unlock(void)
{
  if (lvgl_timeout_mutex != NULL)
  {
    xSemaphoreGive(lvgl_timeout_mutex);
  }
  else
  {
    debug_log_error(DEBUG_TAG_LVGL_SETUP, "LVGL timeout mutex not initialized for unlock!");
  }
}

// Static functions (implementation details)
static esp_err_t init_panel_config(esp_lcd_rgb_panel_config_t *panel_config)
{
  log_memory_status("Before panel config init");

  memset(panel_config, 0, sizeof(esp_lcd_rgb_panel_config_t));

  panel_config->data_width = LCD_DATA_BUS_WIDTH;
  panel_config->dma_burst_size = 64;
  panel_config->num_fbs = LCD_NUM_FB;
  panel_config->clk_src = LCD_CLK_SRC_DEFAULT;
  // CRITICAL FIX: Single frame buffer mode - frame buffer in SPIRAM
  panel_config->flags.fb_in_psram = true; // Frame buffer in SPIRAM for stability

#if CONFIG_EXAMPLE_USE_BOUNCE_BUFFER
  panel_config->bounce_buffer_size_px = 10 * LCD_H_RES; // Reduced from 20 to 10 lines for DRAM conservation
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

  // Timing - adjusted for ESP32-8048S050 stability with WiFi coexistence
  panel_config->timings.pclk_hz = LCD_PIXEL_CLOCK_HZ;
  panel_config->timings.h_res = LCD_H_RES;
  panel_config->timings.v_res = LCD_V_RES;
  panel_config->timings.hsync_back_porch = LCD_HBP;
  panel_config->timings.hsync_front_porch = LCD_HFP;
  panel_config->timings.hsync_pulse_width = LCD_HSYNC;
  panel_config->timings.vsync_back_porch = LCD_VBP;
  panel_config->timings.vsync_front_porch = LCD_VFP;
  panel_config->timings.vsync_pulse_width = LCD_VSYNC;

  return ESP_OK;
}

static bool IRAM_ATTR lvgl_notify_flush_ready(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_ctx)
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
    // Use the same mutex system as other UI components to prevent deadlocks
    if (lvgl_port_lock(0)) // No timeout for the main LVGL task
    {
      time_till_next_ms = lv_timer_handler();
      lvgl_port_unlock();
    }
    else
    {
      // If we can't get the lock immediately, just wait a bit and try again
      time_till_next_ms = 10;
    }

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

void *lvgl_setup_init_touch(void)
{
  debug_log_event(DEBUG_TAG_GT911_TOUCH, "Initializing GT911 touch controller");

  // Initialize GT911 hardware
  esp_err_t ret = gt911_init();
  if (ret != ESP_OK)
  {
    debug_log_error_f(DEBUG_TAG_GT911_TOUCH, "GT911 initialization failed: %s", esp_err_to_name(ret));
    return NULL;
  }

  // Create LVGL input device for touch
  lv_indev_t *indev = lv_indev_create();
  if (!indev)
  {
    debug_log_error(DEBUG_TAG_GT911_TOUCH, "Failed to create LVGL input device");
    gt911_deinit();
    return NULL;
  }

  // Configure input device
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, gt911_lvgl_read);

  debug_log_event(DEBUG_TAG_GT911_TOUCH, "Touch controller initialized successfully");

  if (indev == NULL)
  {
    debug_log_error(DEBUG_TAG_GT911_TOUCH, "Failed to initialize touch");
  }

  return indev;
}
