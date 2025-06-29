/**
 * @file gt911_touch.c
 * @brief GT911 Capacitive Touch Controller Driver for ESP32-S3-8048S050
 *
 * This driver provides complete GT911 touch controller support including:
 * - I2C communication and initialization
 * - Multi-touch point reading (up to 5 points)
 * - LVGL integration with input device callback
 * - Touch coordinate calibration
 * - Hardware reset and configuration
 */

#include "gt911_touch.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "gt911_touch";

// Static variables
static bool gt911_initialized = false;
static gt911_touch_data_t last_touch_data = {0};
static uint8_t gt911_i2c_addr = GT911_I2C_ADDR_1; // Default address

// ═══════════════════════════════════════════════════════════════════════════════
// PRIVATE FUNCTION PROTOTYPES
// ═══════════════════════════════════════════════════════════════════════════════

static esp_err_t gt911_i2c_init(void);
static esp_err_t gt911_i2c_write_reg(uint16_t reg_addr, uint8_t *data, size_t len);
static esp_err_t gt911_i2c_read_reg(uint16_t reg_addr, uint8_t *data, size_t len);
static esp_err_t gt911_hardware_reset(void);
static esp_err_t gt911_detect_i2c_address(void);
static void gt911_parse_touch_data(uint8_t *raw_data, gt911_touch_data_t *touch_data);

// ═══════════════════════════════════════════════════════════════════════════════
// I2C COMMUNICATION FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Initialize I2C for GT911 communication
 */
static esp_err_t gt911_i2c_init(void)
{
  i2c_config_t conf = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = GT911_SDA_GPIO,
      .scl_io_num = GT911_SCL_GPIO,
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master.clk_speed = GT911_I2C_FREQ_HZ,
  };

  esp_err_t ret = i2c_param_config(GT911_I2C_NUM, &conf);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(ret));
    return ret;
  }

  ret = i2c_driver_install(GT911_I2C_NUM, conf.mode, 0, 0, 0);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGI(TAG, "I2C initialized successfully");
  return ESP_OK;
}

/**
 * @brief Write data to GT911 register
 */
static esp_err_t gt911_i2c_write_reg(uint16_t reg_addr, uint8_t *data, size_t len)
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (gt911_i2c_addr << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, (reg_addr >> 8) & 0xFF, true); // Register address high byte
  i2c_master_write_byte(cmd, reg_addr & 0xFF, true);        // Register address low byte

  if (data && len > 0)
  {
    i2c_master_write(cmd, data, len, true);
  }

  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(GT911_I2C_NUM, cmd, pdMS_TO_TICKS(GT911_I2C_TIMEOUT_MS));
  i2c_cmd_link_delete(cmd);

  if (ret != ESP_OK)
  {
    ESP_LOGW(TAG, "I2C write failed: %s", esp_err_to_name(ret));
  }

  return ret;
}

/**
 * @brief Read data from GT911 register
 */
static esp_err_t gt911_i2c_read_reg(uint16_t reg_addr, uint8_t *data, size_t len)
{
  if (!data || len == 0)
  {
    return ESP_ERR_INVALID_ARG;
  }

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (gt911_i2c_addr << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, (reg_addr >> 8) & 0xFF, true); // Register address high byte
  i2c_master_write_byte(cmd, reg_addr & 0xFF, true);        // Register address low byte

  i2c_master_start(cmd); // Repeated start
  i2c_master_write_byte(cmd, (gt911_i2c_addr << 1) | I2C_MASTER_READ, true);

  if (len > 1)
  {
    i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
  }
  i2c_master_read_byte(cmd, &data[len - 1], I2C_MASTER_NACK);

  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(GT911_I2C_NUM, cmd, pdMS_TO_TICKS(GT911_I2C_TIMEOUT_MS));
  i2c_cmd_link_delete(cmd);

  if (ret != ESP_OK)
  {
    ESP_LOGW(TAG, "I2C read failed: %s", esp_err_to_name(ret));
  }

  return ret;
}

// ═══════════════════════════════════════════════════════════════════════════════
// HARDWARE CONTROL FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Perform hardware reset of GT911
 */
static esp_err_t gt911_hardware_reset(void)
{
  // Configure reset and interrupt pins
  gpio_config_t gpio_conf = {
      .pin_bit_mask = (1ULL << GT911_RST_GPIO) | (1ULL << GT911_INT_GPIO),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&gpio_conf));

  // Reset sequence to set I2C address
  // INT pin determines the I2C address during reset
  gpio_set_level(GT911_INT_GPIO, 0); // Set INT low for address 0x5D
  gpio_set_level(GT911_RST_GPIO, 0); // Assert reset
  vTaskDelay(pdMS_TO_TICKS(10));     // Hold reset for 10ms

  gpio_set_level(GT911_RST_GPIO, 1); // Release reset
  vTaskDelay(pdMS_TO_TICKS(10));     // Wait for controller to boot

  // Configure INT pin as input after reset
  gpio_conf.pin_bit_mask = (1ULL << GT911_INT_GPIO);
  gpio_conf.mode = GPIO_MODE_INPUT;
  gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  ESP_ERROR_CHECK(gpio_config(&gpio_conf));

  vTaskDelay(pdMS_TO_TICKS(50)); // Additional stabilization time

  ESP_LOGI(TAG, "GT911 hardware reset completed");
  return ESP_OK;
}

/**
 * @brief Detect GT911 I2C address
 */
static esp_err_t gt911_detect_i2c_address(void)
{
  uint8_t test_data;

  // Try first address (0x5D - INT low during reset)
  gt911_i2c_addr = GT911_I2C_ADDR_1;
  if (gt911_i2c_read_reg(GT911_REG_ID, &test_data, 1) == ESP_OK)
  {
    ESP_LOGI(TAG, "GT911 detected at address 0x%02X", gt911_i2c_addr);
    return ESP_OK;
  }

  // Try second address (0x14 - INT high during reset)
  gt911_i2c_addr = GT911_I2C_ADDR_2;
  if (gt911_i2c_read_reg(GT911_REG_ID, &test_data, 1) == ESP_OK)
  {
    ESP_LOGI(TAG, "GT911 detected at address 0x%02X", gt911_i2c_addr);
    return ESP_OK;
  }

  ESP_LOGE(TAG, "GT911 not found at any address");
  return ESP_FAIL;
}

// ═══════════════════════════════════════════════════════════════════════════════
// TOUCH DATA PROCESSING
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Parse raw touch data from GT911
 */
static void gt911_parse_touch_data(uint8_t *raw_data, gt911_touch_data_t *touch_data)
{
  // Clear previous data
  memset(touch_data, 0, sizeof(gt911_touch_data_t));

  // Parse touch count (bits 0-3 of status register)
  touch_data->touch_count = raw_data[0] & 0x0F;

  if (touch_data->touch_count > GT911_MAX_TOUCH_POINTS)
  {
    touch_data->touch_count = GT911_MAX_TOUCH_POINTS;
  }

  // Parse individual touch points
  for (int i = 0; i < touch_data->touch_count; i++)
  {
    uint8_t *point_data = &raw_data[1 + i * 8]; // Each point is 8 bytes

    touch_data->points[i].track_id = point_data[0];
    touch_data->points[i].x = ((uint16_t)point_data[2] << 8) | point_data[1];
    touch_data->points[i].y = ((uint16_t)point_data[4] << 8) | point_data[3];
    touch_data->points[i].size = ((uint16_t)point_data[6] << 8) | point_data[5];
    touch_data->points[i].pressed = true;

    // Apply coordinate calibration
    gt911_calibrate_coords(touch_data->points[i].x, touch_data->points[i].y,
                           &touch_data->points[i].x, &touch_data->points[i].y);
  }

  touch_data->data_ready = (touch_data->touch_count > 0);
}

// ═══════════════════════════════════════════════════════════════════════════════
// PUBLIC API FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════

esp_err_t gt911_init(void)
{
  if (gt911_initialized)
  {
    ESP_LOGW(TAG, "GT911 already initialized");
    return ESP_OK;
  }

  ESP_LOGI(TAG, "Initializing GT911 touch controller...");

  // Initialize I2C
  esp_err_t ret = gt911_i2c_init();
  if (ret != ESP_OK)
  {
    return ret;
  }

  // Perform hardware reset
  ret = gt911_hardware_reset();
  if (ret != ESP_OK)
  {
    return ret;
  }

  // Detect I2C address
  ret = gt911_detect_i2c_address();
  if (ret != ESP_OK)
  {
    return ret;
  }

  // Get and display product ID
  char product_id[5] = {0};
  if (gt911_get_product_id(product_id) == ESP_OK)
  {
    ESP_LOGI(TAG, "GT911 Product ID: %s", product_id);
  }

  // Clear any pending touch data
  uint8_t clear_cmd = 0;
  gt911_i2c_write_reg(GT911_REG_STATUS, &clear_cmd, 1);

  gt911_initialized = true;
  ESP_LOGI(TAG, "GT911 initialization completed successfully");

  return ESP_OK;
}

esp_err_t gt911_deinit(void)
{
  if (!gt911_initialized)
  {
    return ESP_OK;
  }

  i2c_driver_delete(GT911_I2C_NUM);
  gt911_initialized = false;

  ESP_LOGI(TAG, "GT911 deinitialized");
  return ESP_OK;
}

esp_err_t gt911_read_touch(gt911_touch_data_t *touch_data)
{
  if (!gt911_initialized || !touch_data)
  {
    return ESP_ERR_INVALID_STATE;
  }

  uint8_t status;
  esp_err_t ret = gt911_i2c_read_reg(GT911_REG_STATUS, &status, 1);
  if (ret != ESP_OK)
  {
    return ret;
  }

  // Check if new touch data is available
  if (!(status & 0x80))
  {
    // No new data, return previous state
    *touch_data = last_touch_data;
    return ESP_OK;
  }

  // Read touch point data (up to 5 points, 8 bytes each + 1 status byte = 41 bytes)
  uint8_t touch_raw_data[41];
  ret = gt911_i2c_read_reg(GT911_REG_STATUS, touch_raw_data, sizeof(touch_raw_data));
  if (ret != ESP_OK)
  {
    return ret;
  }

  // Parse the touch data
  gt911_parse_touch_data(touch_raw_data, touch_data);

  // Store as last known state
  last_touch_data = *touch_data;

  // Clear the status register to acknowledge data read
  uint8_t clear_cmd = 0;
  gt911_i2c_write_reg(GT911_REG_STATUS, &clear_cmd, 1);

  return ESP_OK;
}

void gt911_lvgl_read(lv_indev_t *indev, lv_indev_data_t *data)
{
  static gt911_touch_data_t touch_data;

  if (gt911_read_touch(&touch_data) != ESP_OK)
  {
    data->state = LV_INDEV_STATE_RELEASED;
    return;
  }

  if (touch_data.data_ready && touch_data.touch_count > 0)
  {
    // Use first touch point for LVGL
    data->point.x = touch_data.points[0].x;
    data->point.y = touch_data.points[0].y;
    data->state = LV_INDEV_STATE_PRESSED;

    // Enhanced debugging for touch events
    ESP_LOGI(TAG, "Touch: Count=%d, X=%d, Y=%d, TrackID=%d",
             touch_data.touch_count, data->point.x, data->point.y,
             touch_data.points[0].track_id);
  }
  else
  {
    data->state = LV_INDEV_STATE_RELEASED;

    // Debug log for touch release (only when transitioning from pressed)
    static bool was_pressed = false;
    if (was_pressed && touch_data.data_ready)
    {
      ESP_LOGI(TAG, "Touch released");
    }
    was_pressed = (data->state == LV_INDEV_STATE_PRESSED);
  }
}

esp_err_t gt911_get_product_id(char *product_id)
{
  if (!gt911_initialized || !product_id)
  {
    return ESP_ERR_INVALID_STATE;
  }

  uint8_t id_data[4];
  esp_err_t ret = gt911_i2c_read_reg(GT911_REG_ID, id_data, 4);
  if (ret != ESP_OK)
  {
    return ret;
  }

  // GT911 sends product ID in reverse order
  product_id[0] = id_data[3];
  product_id[1] = id_data[2];
  product_id[2] = id_data[1];
  product_id[3] = id_data[0];
  product_id[4] = '\0';

  return ESP_OK;
}

esp_err_t gt911_soft_reset(void)
{
  if (!gt911_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  uint8_t reset_cmd = 0x02;
  esp_err_t ret = gt911_i2c_write_reg(GT911_REG_CMD, &reset_cmd, 1);
  if (ret == ESP_OK)
  {
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for reset to complete
    ESP_LOGI(TAG, "GT911 soft reset completed");
  }

  return ret;
}

void gt911_calibrate_coords(uint16_t raw_x, uint16_t raw_y, uint16_t *cal_x, uint16_t *cal_y)
{
  // Basic coordinate mapping - adjust these values based on your screen orientation
  // and calibration requirements

  // For ESP32-S3-8048S050 (800x480 display)
  // Assuming GT911 coordinates need to be mapped to screen coordinates

  *cal_x = raw_x;
  *cal_y = raw_y;

  // Bounds checking
  if (*cal_x >= TOUCH_SCREEN_WIDTH)
  {
    *cal_x = TOUCH_SCREEN_WIDTH - 1;
  }
  if (*cal_y >= TOUCH_SCREEN_HEIGHT)
  {
    *cal_y = TOUCH_SCREEN_HEIGHT - 1;
  }

  // Optional: Apply rotation/mirroring transformations here
  // Example for 180-degree rotation:
  // *cal_x = TOUCH_SCREEN_WIDTH - 1 - *cal_x;
  // *cal_y = TOUCH_SCREEN_HEIGHT - 1 - *cal_y;
}
