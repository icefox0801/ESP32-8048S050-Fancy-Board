#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// STANDARD INCLUDES
// ═══════════════════════════════════════════════════════════════════════════════

#include "driver/i2c.h"
#include "esp_err.h"
#include "lvgl.h"

// ═══════════════════════════════════════════════════════════════════════════════
// GT911 TOUCH CONTROLLER CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════

// GT911 I2C Configuration
#define GT911_I2C_ADDR_1 0x5D // I2C address when INT pin is LOW during reset
#define GT911_I2C_ADDR_2 0x14 // I2C address when INT pin is HIGH during reset

// GT911 Register Addresses
#define GT911_REG_CMD 0x8040     // Command register
#define GT911_REG_STATUS 0x814E  // Touch status register
#define GT911_REG_ID 0x8140      // Product ID register
#define GT911_REG_POINT_1 0x814F // First touch point data
#define GT911_REG_POINT_2 0x8157 // Second touch point data
#define GT911_REG_POINT_3 0x815F // Third touch point data
#define GT911_REG_POINT_4 0x8167 // Fourth touch point data
#define GT911_REG_POINT_5 0x816F // Fifth touch point data

// Hardware pin definitions (ESP32-S3-8048S050 specific)
#define GT911_SDA_GPIO 19 // I2C SDA pin
#define GT911_SCL_GPIO 20 // I2C SCL pin
#define GT911_INT_GPIO 21 // Interrupt pin
#define GT911_RST_GPIO 38 // Reset pin

// I2C Configuration
#define GT911_I2C_NUM I2C_NUM_0
#define GT911_I2C_FREQ_HZ 400000 // 400kHz
#define GT911_I2C_TIMEOUT_MS 100

// Touch Configuration
#define GT911_MAX_TOUCH_POINTS 5 // Maximum simultaneous touch points
#define TOUCH_SCREEN_WIDTH 800   // Screen width in pixels
#define TOUCH_SCREEN_HEIGHT 480  // Screen height in pixels

// ═══════════════════════════════════════════════════════════════════════════════
// DATA STRUCTURES
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Touch point data structure
 */
typedef struct
{
  uint16_t x;       // X coordinate
  uint16_t y;       // Y coordinate
  uint16_t size;    // Touch area size
  uint8_t track_id; // Touch track ID
  bool pressed;     // Touch press status
} gt911_touch_point_t;

/**
 * @brief Complete touch data structure
 */
typedef struct
{
  uint8_t touch_count;                                // Number of active touch points
  gt911_touch_point_t points[GT911_MAX_TOUCH_POINTS]; // Touch point data array
  bool data_ready;                                    // Data ready flag
} gt911_touch_data_t;

// ═══════════════════════════════════════════════════════════════════════════════
// FUNCTION DECLARATIONS
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Initialize GT911 touch controller
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t gt911_init(void);

/**
 * @brief Deinitialize GT911 touch controller
 * @return ESP_OK on success
 */
esp_err_t gt911_deinit(void);

/**
 * @brief Read touch data from GT911
 * @param touch_data Pointer to touch data structure
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t gt911_read_touch(gt911_touch_data_t *touch_data);

/**
 * @brief LVGL input device read callback for GT911
 * @param indev LVGL input device
 * @param data LVGL input data structure
 */
void gt911_lvgl_read(lv_indev_t *indev, lv_indev_data_t *data);

/**
 * @brief Get GT911 product ID
 * @param product_id Buffer to store product ID (4 bytes)
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t gt911_get_product_id(char *product_id);

/**
 * @brief Software reset GT911 controller
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t gt911_soft_reset(void);

/**
 * @brief Calibrate GT911 touch coordinates
 * @param raw_x Raw X coordinate from GT911
 * @param raw_y Raw Y coordinate from GT911
 * @param cal_x Pointer to store calibrated X coordinate
 * @param cal_y Pointer to store calibrated Y coordinate
 */
void gt911_calibrate_coords(uint16_t raw_x, uint16_t raw_y, uint16_t *cal_x, uint16_t *cal_y);
