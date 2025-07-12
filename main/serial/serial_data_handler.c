/**
 * @file serial_data_handler.c
 * @brief Serial Data Handler Implementation for System Monitor Dashboard
 *
 * Provides UART communication and JSON parsing functionality for real-time
 * system monitoring data. Handles connection timeout detection and data
 * validation for reliable operation.
 *      }

      // Prevent buffer overflow32 System Monitor
 * @version 1.0
 * @date 2024
 */

// ═══════════════════════════════════════════════════════════════════════════════
// STANDARD INCLUDES
// ═══════════════════════════════════════════════════════════════════════════════

#include "serial_data_handler.h"

#include "cjson.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "system_monitor_ui.h"
#include <string.h>
#include <time.h>

// ═══════════════════════════════════════════════════════════════════════════════
// CONSTANTS AND CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════

static const char *TAG = "serial_data";

// UART Hardware Configuration
#define UART_PORT_NUM UART_NUM_0                ///< UART port number
#define UART_BAUD_RATE 115200                   ///< Communication baud rate
#define UART_DATA_BITS UART_DATA_8_BITS         ///< Data bits per frame
#define UART_PARITY UART_PARITY_DISABLE         ///< Parity checking
#define UART_STOP_BITS UART_STOP_BITS_1         ///< Stop bits per frame
#define UART_FLOW_CTRL UART_HW_FLOWCTRL_DISABLE ///< Flow control
#define UART_SOURCE_CLK UART_SCLK_DEFAULT       ///< Clock source

// Buffer Management
#define BUF_SIZE 2048         ///< UART receive buffer size
#define JSON_BUFFER_SIZE 1024 ///< JSON parsing buffer size

// Task Configuration
#define SERIAL_TASK_STACK_SIZE 8192 ///< Task stack size in bytes (reduced for memory optimization)
#define SERIAL_TASK_PRIORITY 2      ///< FreeRTOS task priority (lowered for LVGL priority)

// ═══════════════════════════════════════════════════════════════════════════════
// STATIC VARIABLES
// ═══════════════════════════════════════════════════════════════════════════════

static TaskHandle_t serial_task_handle = NULL; ///< Serial reception task handle
static bool serial_running = false;            ///< Task running state flag
static uint32_t last_data_time = 0;            ///< Last data reception timestamp

// Static task resources for SPIRAM allocation
static StaticTask_t serial_task_tcb;          ///< Task control block
static StackType_t *serial_task_stack = NULL; ///< Task stack allocated from SPIRAM
static uint32_t connection_timeout_ms = 5000; ///< Connection timeout (5 seconds)

// ═══════════════════════════════════════════════════════════════════════════════
// PRIVATE FUNCTION PROTOTYPES
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Parse JSON string and extract system monitoring data
 * @param json_str Input JSON string to parse
 * @param data Output system data structure
 * @return true if parsing successful, false otherwise
 */
static bool parse_json_data(const char *json_str, system_data_t *data);

/**
 * @brief Process a complete line of received data
 * @param line_buffer The line buffer containing the data
 * @param system_data System data structure to update
 */
static void process_received_line(const char *line_buffer, system_data_t *system_data);

/**
 * @brief Add a byte to the line buffer and handle line completion
 * @param byte The byte to add
 * @param line_buffer The line buffer
 * @param line_pos Pointer to the current line position
 * @param system_data System data structure to update
 * @return true if a complete line was processed, false otherwise
 */
static bool handle_incoming_byte(uint8_t byte, char *line_buffer, int *line_pos, system_data_t *system_data);

/**
 * @brief Check for connection timeout and update UI status
 * @param current_time Current timestamp
 */
static void check_connection_timeout(uint32_t current_time);

// ═══════════════════════════════════════════════════════════════════════════════
// PRIVATE FUNCTION IMPLEMENTATIONS
// ═══════════════════════════════════════════════════════════════════════════════
static bool parse_json_data(const char *json_str, system_data_t *data)
{
  cJSON *json = cJSON_Parse(json_str);
  if (json == NULL)
  {
    ESP_LOGW(TAG, "Invalid JSON format");
    return false;
  }

  // ─────────────────────────────────────────────────────────────────
  // Parse Timestamp
  // ─────────────────────────────────────────────────────────────────

  cJSON *ts = cJSON_GetObjectItem(json, "ts");
  if (cJSON_IsNumber(ts))
  {
    data->timestamp = (uint64_t)cJSON_GetNumberValue(ts);
  }
  else
  {
    data->timestamp = (uint64_t)time(NULL) * 1000; // Current time in ms
  }

  // Parse CPU data
  cJSON *cpu = cJSON_GetObjectItem(json, "cpu");
  if (cJSON_IsObject(cpu))
  {
    cJSON *cpu_usage = cJSON_GetObjectItem(cpu, "usage");
    cJSON *cpu_temp = cJSON_GetObjectItem(cpu, "temp");
    cJSON *cpu_fan = cJSON_GetObjectItem(cpu, "fan");
    cJSON *cpu_name = cJSON_GetObjectItem(cpu, "name");

    if (cJSON_IsNumber(cpu_usage))
    {
      data->cpu.usage = (uint8_t)cJSON_GetNumberValue(cpu_usage);
    }
    if (cJSON_IsNumber(cpu_temp))
    {
      data->cpu.temp = (uint8_t)cJSON_GetNumberValue(cpu_temp);
    }
    if (cJSON_IsNumber(cpu_fan))
    {
      data->cpu.fan = (uint16_t)cJSON_GetNumberValue(cpu_fan);
    }
    if (cJSON_IsString(cpu_name))
    {
      strncpy(data->cpu.name, cJSON_GetStringValue(cpu_name),
              sizeof(data->cpu.name) - 1);
      data->cpu.name[sizeof(data->cpu.name) - 1] = '\0';
    }
  }

  // Parse GPU data
  cJSON *gpu = cJSON_GetObjectItem(json, "gpu");
  if (cJSON_IsObject(gpu))
  {
    cJSON *gpu_usage = cJSON_GetObjectItem(gpu, "usage");
    cJSON *gpu_temp = cJSON_GetObjectItem(gpu, "temp");
    cJSON *gpu_name = cJSON_GetObjectItem(gpu, "name");
    cJSON *gpu_mem_used = cJSON_GetObjectItem(gpu, "mem_used");
    cJSON *gpu_mem_total = cJSON_GetObjectItem(gpu, "mem_total");

    if (cJSON_IsNumber(gpu_usage))
    {
      data->gpu.usage = (uint8_t)cJSON_GetNumberValue(gpu_usage);
    }
    if (cJSON_IsNumber(gpu_temp))
    {
      data->gpu.temp = (uint8_t)cJSON_GetNumberValue(gpu_temp);
    }
    if (cJSON_IsString(gpu_name))
    {
      strncpy(data->gpu.name, cJSON_GetStringValue(gpu_name),
              sizeof(data->gpu.name) - 1);
      data->gpu.name[sizeof(data->gpu.name) - 1] = '\0';
    }
    if (cJSON_IsNumber(gpu_mem_used))
    {
      data->gpu.mem_used = (uint32_t)cJSON_GetNumberValue(gpu_mem_used);
    }
    if (cJSON_IsNumber(gpu_mem_total))
    {
      data->gpu.mem_total = (uint32_t)cJSON_GetNumberValue(gpu_mem_total);
    }
  }

  // Parse memory data
  cJSON *mem = cJSON_GetObjectItem(json, "mem");
  if (cJSON_IsObject(mem))
  {
    cJSON *mem_usage = cJSON_GetObjectItem(mem, "usage");
    cJSON *mem_used = cJSON_GetObjectItem(mem, "used");
    cJSON *mem_total = cJSON_GetObjectItem(mem, "total");
    cJSON *mem_avail = cJSON_GetObjectItem(mem, "avail");

    if (cJSON_IsNumber(mem_usage))
    {
      data->mem.usage = (uint8_t)cJSON_GetNumberValue(mem_usage);
    }
    if (cJSON_IsNumber(mem_used))
    {
      data->mem.used = (float)cJSON_GetNumberValue(mem_used);
    }
    if (cJSON_IsNumber(mem_total))
    {
      data->mem.total = (float)cJSON_GetNumberValue(mem_total);
    }
    if (cJSON_IsNumber(mem_avail))
    {
      data->mem.avail = (float)cJSON_GetNumberValue(mem_avail);
    }
  }

  cJSON_Delete(json);
  return true;
}

/**
 * @brief Process a complete line of received data
 */
static void process_received_line(const char *line_buffer, system_data_t *system_data)
{
  // Skip empty lines
  if (strlen(line_buffer) < 5)
    return;

  // Check if this looks like JSON data (starts with { and ends with })
  const char *trimmed = line_buffer;
  while (*trimmed == ' ' || *trimmed == '\t')
    trimmed++; // Skip whitespace

  if (trimmed[0] == '{')
  {
    // Find the end of JSON
    size_t len = strlen(trimmed);
    const char *end = trimmed + len - 1;
    while (end > trimmed && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r'))
      end--;

    if (*end == '}')
    {
      // Parse and update UI (reduce logging frequency)
      if (parse_json_data(trimmed, system_data))
      {
        system_monitor_ui_update(system_data);

        // Log less frequently to avoid blocking serial processing
        static uint32_t success_counter = 0;
        if (++success_counter % 10 == 0) // Log every 10th successful parse
        {
          ESP_LOGI(TAG, "Successfully parsed and updated system data (total: %lu)", success_counter);
        }
      }
      else
      {
        ESP_LOGW(TAG, "Failed to parse JSON: %.50s...", trimmed);
      }
    }
    else
    {
      ESP_LOGW(TAG, "Incomplete JSON received: %.50s...", trimmed);
    }
  }
  else
  {
    // Log non-JSON debug messages from sender (less frequently)
    static uint32_t debug_counter = 0;
    if (++debug_counter % 10 == 0) // More frequent debug logging
    {
      ESP_LOGI(TAG, "[SENDER DEBUG] %s", line_buffer);
    }
  }
}

/**
 * @brief Add a byte to the line buffer and handle line completion
 */
static bool handle_incoming_byte(uint8_t byte, char *line_buffer, int *line_pos, system_data_t *system_data)
{
  // Check for end of line
  if (byte == '\n' || byte == '\r')
  {
    // Process line if we have data
    if (*line_pos > 0)
    {
      line_buffer[*line_pos] = '\0';
      process_received_line(line_buffer, system_data);
      *line_pos = 0; // Reset for next line
      return true;
    }
  }
  else if (byte >= 32 && byte <= 126) // Only accept printable ASCII characters
  {
    if (*line_pos < JSON_BUFFER_SIZE - 1)
    {
      // Add character to line buffer
      line_buffer[(*line_pos)++] = byte;
    }
    else
    {
      // Line too long, reset buffer
      ESP_LOGW(TAG, "Line buffer overflow, resetting");
      *line_pos = 0;
    }
  }

  return false;
}

/**
 * @brief Check for connection timeout and update UI status
 */
static void check_connection_timeout(uint32_t current_time)
{
  static bool timeout_logged = false;

  if (current_time - last_data_time > connection_timeout_ms)
  {
    if (!timeout_logged)
    {
      ESP_LOGW(TAG, "No data received for %d ms", connection_timeout_ms);
      timeout_logged = true;
    }
  }
  else
  {
    // Reset timeout flag when data is received
    timeout_logged = false;
  }
}

/**
 * @brief Serial data reception task
 */
static void serial_data_task(void *pvParameters)
{
  static char line_buffer[JSON_BUFFER_SIZE];
  static int line_pos = 0;

  system_data_t system_data = {0};
  uint32_t current_time;

  ESP_LOGI(TAG, "Serial data task started");

  while (serial_running)
  {
    // Read multiple bytes at once for better performance, but smaller buffer
    // to reduce risk of partial JSON objects
    uint8_t data_buffer[32];                                                                       // Reduced from 64 to 32 bytes
    int len = uart_read_bytes(UART_PORT_NUM, data_buffer, sizeof(data_buffer), pdMS_TO_TICKS(20)); // Reduced timeout

    current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

    if (len > 0)
    {
      last_data_time = current_time;

      // Process each received byte
      for (int i = 0; i < len; i++)
      {
        handle_incoming_byte(data_buffer[i], line_buffer, &line_pos, &system_data);
      }
    }

    // Check for connection timeout
    check_connection_timeout(current_time);

    vTaskDelay(pdMS_TO_TICKS(5)); // Reduced delay for faster response
  }

  ESP_LOGI(TAG, "Serial data task stopped");
  vTaskDelete(NULL);
}

esp_err_t serial_data_init(void)
{
  uart_config_t uart_config = {
      .baud_rate = UART_BAUD_RATE,
      .data_bits = UART_DATA_BITS,
      .parity = UART_PARITY,
      .stop_bits = UART_STOP_BITS,
      .flow_ctrl = UART_FLOW_CTRL,
      .source_clk = UART_SOURCE_CLK,
  };

  // Install UART driver
  ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));

  ESP_LOGI(TAG, "UART initialized on port %d at %d baud", UART_PORT_NUM, UART_BAUD_RATE);

  return ESP_OK;
}

void serial_data_start_task(void)
{
  if (!serial_running)
  {
    serial_running = true;
    last_data_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

    // Allocate task stack from SPIRAM for better internal RAM utilization
    serial_task_stack = (StackType_t *)heap_caps_malloc(
        SERIAL_TASK_STACK_SIZE, MALLOC_CAP_SPIRAM);

    if (serial_task_stack == NULL)
    {
      ESP_LOGE(TAG, "Failed to allocate serial task stack from SPIRAM");
      // Fallback to standard task creation
      xTaskCreatePinnedToCore(serial_data_task, "serial_data", SERIAL_TASK_STACK_SIZE,
                              NULL, SERIAL_TASK_PRIORITY, &serial_task_handle, 0);
      ESP_LOGW(TAG, "Serial task created with standard internal RAM stack");
    }
    else
    {
      // Create static task with SPIRAM stack
      serial_task_handle = xTaskCreateStatic(
          serial_data_task,                             // Task function
          "serial_data",                                // Task name
          SERIAL_TASK_STACK_SIZE / sizeof(StackType_t), // Stack size in words
          NULL,                                         // Parameters
          SERIAL_TASK_PRIORITY,                         // Priority
          serial_task_stack,                            // Stack buffer
          &serial_task_tcb                              // Task control block
      );

      ESP_LOGI(TAG, "Serial task created with SPIRAM stack at %p", serial_task_stack);
    }

    ESP_LOGI(TAG, "Serial data reception started on core 0");
  }
}

void serial_data_stop(void)
{
  if (serial_running)
  {
    serial_running = false;
    if (serial_task_handle)
    {
      vTaskDelete(serial_task_handle);
      serial_task_handle = NULL;

      // Free SPIRAM stack if it was allocated
      if (serial_task_stack)
      {
        heap_caps_free(serial_task_stack);
        serial_task_stack = NULL;
        ESP_LOGI(TAG, "Serial task SPIRAM stack freed");
      }
    }
    ESP_LOGI(TAG, "Serial data reception stopped");
  }
}
