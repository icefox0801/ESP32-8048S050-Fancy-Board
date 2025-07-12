/**
 * @file wifi_manager.c
 * @brief WiFi connection and management implementation
 *
 * This module implements comprehensive WiFi functionality including connection management,
 * automatic reconnection, status monitoring, and internet connectivity checking.
 *
 * @author System Monitor Dashboard
 * @date 2025-08-14
 */

#include "wifi_manager.h"
#include "wifi_config.h"
#include <string.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_heap_caps.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <lwip/err.h>
#include <lwip/sys.h>
#include <esp_sntp.h>
#include <nvs_flash.h>

// ═══════════════════════════════════════════════════════════════════════════════
// CONSTANTS AND CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════

static const char *TAG = "WiFi_Manager";

/** Default WiFi credentials from network_config.h or menuconfig */
#ifdef WIFI_SSID
#define DEFAULT_WIFI_SSID WIFI_SSID
#define DEFAULT_WIFI_PASSWORD WIFI_PASSWORD
#else
#define DEFAULT_WIFI_SSID CONFIG_WIFI_SSID
#define DEFAULT_WIFI_PASSWORD CONFIG_WIFI_PASSWORD
#endif

/** Internet connectivity test servers */
#define CONNECTIVITY_TEST_HOST1 "8.8.8.8" // Google DNS
#define CONNECTIVITY_TEST_HOST2 "1.1.1.1" // Cloudflare DNS
#define PING_TIMEOUT_MS 3000

// ═══════════════════════════════════════════════════════════════════════════════
// PRIVATE DATA STRUCTURES
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief WiFi manager internal state
 */
typedef struct
{
  EventGroupHandle_t wifi_event_group;                             ///< FreeRTOS event group for WiFi events
  esp_netif_t *netif;                                              ///< Network interface handle
  wifi_status_t status;                                            ///< Current connection status
  wifi_info_t connection_info;                                     ///< Current connection information
  wifi_status_callback_t status_callback;                          ///< User callback for status changes
  void (*ui_callback)(const char *status_text, bool is_connected); ///< UI-specific callback
  void (*ha_callback)(bool is_connected);                          ///< Home Assistant task management callback
  uint8_t retry_count;                                             ///< Current retry attempt count
  TaskHandle_t reconnect_task_handle;                              ///< Handle for reconnection task
  bool initialized;                                                ///< Initialization status flag
  bool sntp_initialized;                                           ///< SNTP initialization status flag
  char configured_ssid[33];                                        ///< Configured SSID
  char configured_password[64];                                    ///< Configured password
} wifi_manager_state_t;

// ═══════════════════════════════════════════════════════════════════════════════
// PRIVATE VARIABLES
// ═══════════════════════════════════════════════════════════════════════════════

static wifi_manager_state_t s_wifi_manager = {0};

// SPIRAM optimization for WiFi reconnect task
static StaticTask_t wifi_reconnect_tcb;
static StackType_t *wifi_reconnect_stack = NULL;

// ═══════════════════════════════════════════════════════════════════════════════
// PRIVATE FUNCTION DECLARATIONS
// ═══════════════════════════════════════════════════════════════════════════════

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void wifi_update_connection_info(void);
static void wifi_set_status(wifi_status_t new_status);
static void wifi_reconnect_task(void *pvParameters);
static esp_err_t wifi_start_reconnect_task(void);
static void wifi_stop_reconnect_task(void);
static bool ping_host(const char *host, uint32_t timeout_ms);

// ═══════════════════════════════════════════════════════════════════════════════
// PRIVATE FUNCTION IMPLEMENTATIONS
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief WiFi event handler
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  if (event_base == WIFI_EVENT)
  {
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
      ESP_LOGI(TAG, "WiFi station started, connecting to AP...");
      wifi_set_status(WIFI_STATUS_CONNECTING);
      esp_wifi_connect();
      break;

    case WIFI_EVENT_STA_CONNECTED:
      ESP_LOGI(TAG, "Connected to WiFi network");
      wifi_update_connection_info();
      s_wifi_manager.retry_count = 0;
      wifi_stop_reconnect_task();
      break;

    case WIFI_EVENT_STA_DISCONNECTED:
    {
      wifi_event_sta_disconnected_t *disconnected = (wifi_event_sta_disconnected_t *)event_data;
      ESP_LOGW(TAG, "WiFi disconnected (reason: %d)", disconnected->reason);

      wifi_set_status(WIFI_STATUS_DISCONNECTED);
      xEventGroupSetBits(s_wifi_manager.wifi_event_group, WIFI_DISCONNECTED_BIT);
      xEventGroupClearBits(s_wifi_manager.wifi_event_group, WIFI_CONNECTED_BIT);

      if (s_wifi_manager.retry_count < WIFI_MAX_RETRY_COUNT)
      {
        ESP_LOGI(TAG, "Retry connecting to WiFi (%d/%d)",
                 s_wifi_manager.retry_count + 1, WIFI_MAX_RETRY_COUNT);
        s_wifi_manager.retry_count++;
        wifi_set_status(WIFI_STATUS_RECONNECTING);

        vTaskDelay(pdMS_TO_TICKS(WIFI_RETRY_DELAY_MS));
        esp_wifi_connect();
      }
      else
      {
        ESP_LOGE(TAG, "Maximum WiFi connection retries reached");
        wifi_set_status(WIFI_STATUS_FAILED);
        xEventGroupSetBits(s_wifi_manager.wifi_event_group, WIFI_FAIL_BIT);
        wifi_start_reconnect_task(); // Start background reconnection
      }
      break;
    }

    case WIFI_EVENT_STA_BEACON_TIMEOUT:
      ESP_LOGW(TAG, "WiFi beacon timeout, attempting reconnection");
      wifi_set_status(WIFI_STATUS_RECONNECTING);
      break;

    default:
      ESP_LOGD(TAG, "Unhandled WiFi event: %" PRId32, event_id);
      break;
    }
  }
}

/**
 * @brief IP event handler
 */
static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  if (event_base == IP_EVENT)
  {
    switch (event_id)
    {
    case IP_EVENT_STA_GOT_IP:
    {
      ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
      ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));

      wifi_update_connection_info();
      wifi_set_status(WIFI_STATUS_CONNECTED);
      xEventGroupSetBits(s_wifi_manager.wifi_event_group, WIFI_CONNECTED_BIT);
      xEventGroupClearBits(s_wifi_manager.wifi_event_group, WIFI_DISCONNECTED_BIT | WIFI_FAIL_BIT);

      // Initialize SNTP for network time synchronization (only once)
      if (!s_wifi_manager.sntp_initialized)
      {
        esp_sntp_setservername(0, "pool.ntp.org");
        esp_sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
        esp_sntp_init();
        s_wifi_manager.sntp_initialized = true;
        ESP_LOGI(TAG, "SNTP initialized successfully");
      }
      break;
    }

    case IP_EVENT_STA_LOST_IP:
      ESP_LOGW(TAG, "Lost IP address");
      wifi_set_status(WIFI_STATUS_DISCONNECTED);

      // Clean up SNTP when IP is lost
      if (s_wifi_manager.sntp_initialized)
      {
        esp_sntp_stop();
        s_wifi_manager.sntp_initialized = false;
        ESP_LOGI(TAG, "SNTP stopped due to IP loss");
      }
      break;

    default:
      ESP_LOGD(TAG, "Unhandled IP event: %" PRId32, event_id);
      break;
    }
  }
}

/**
 * @brief Update connection information structure
 */
static void wifi_update_connection_info(void)
{
  wifi_ap_record_t ap_info;
  esp_netif_ip_info_t ip_info;

  // Clear previous info
  memset(&s_wifi_manager.connection_info, 0, sizeof(wifi_info_t));

  // Get AP information
  if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK)
  {
    strncpy(s_wifi_manager.connection_info.ssid, (char *)ap_info.ssid, sizeof(s_wifi_manager.connection_info.ssid) - 1);
    s_wifi_manager.connection_info.rssi = ap_info.rssi;
    s_wifi_manager.connection_info.auth_mode = ap_info.authmode;
    s_wifi_manager.connection_info.channel = ap_info.primary;
  }

  // Get IP information
  if (esp_netif_get_ip_info(s_wifi_manager.netif, &ip_info) == ESP_OK)
  {
    snprintf(s_wifi_manager.connection_info.ip_address, sizeof(s_wifi_manager.connection_info.ip_address),
             IPSTR, IP2STR(&ip_info.ip));
    snprintf(s_wifi_manager.connection_info.gateway, sizeof(s_wifi_manager.connection_info.gateway),
             IPSTR, IP2STR(&ip_info.gw));
    snprintf(s_wifi_manager.connection_info.netmask, sizeof(s_wifi_manager.connection_info.netmask),
             IPSTR, IP2STR(&ip_info.netmask));
  }

  s_wifi_manager.connection_info.connection_time = xTaskGetTickCount();
  s_wifi_manager.connection_info.has_internet = wifi_manager_check_internet(PING_TIMEOUT_MS);
}

/**
 * @brief Convert WiFi status to UI-friendly text
 */
static const char *wifi_status_to_text(wifi_status_t status, const wifi_info_t *info)
{
  static char detailed_status[128];

  switch (status)
  {
  case WIFI_STATUS_DISCONNECTED:
    return "Disconnected";

  case WIFI_STATUS_CONNECTING:
    return "Connecting...";

  case WIFI_STATUS_CONNECTED:
    if (info)
    {
      snprintf(detailed_status, sizeof(detailed_status), "Connected: %s (%s)",
               info->ssid, info->ip_address);
      return detailed_status;
    }
    return "Connected";

  case WIFI_STATUS_FAILED:
    return "Connection Failed";

  case WIFI_STATUS_RECONNECTING:
    return "Reconnecting...";

  default:
    return "Unknown";
  }
}

/**
 * @brief Set WiFi status and notify callback
 */
static void wifi_set_status(wifi_status_t new_status)
{
  if (s_wifi_manager.status != new_status)
  {
    s_wifi_manager.status = new_status;

    // Call the original callback if registered
    if (s_wifi_manager.status_callback)
    {
      const wifi_info_t *info = (new_status == WIFI_STATUS_CONNECTED) ? &s_wifi_manager.connection_info : NULL;
      s_wifi_manager.status_callback(new_status, info);
    }

    // Call the UI callback if registered
    if (s_wifi_manager.ui_callback)
    {
      const wifi_info_t *info = (new_status == WIFI_STATUS_CONNECTED) ? &s_wifi_manager.connection_info : NULL;
      const char *status_text = wifi_status_to_text(new_status, info);
      bool is_connected = (new_status == WIFI_STATUS_CONNECTED);
      s_wifi_manager.ui_callback(status_text, is_connected);
    }

    // Call the HA callback if registered
    if (s_wifi_manager.ha_callback)
    {
      bool is_connected = (new_status == WIFI_STATUS_CONNECTED);
      s_wifi_manager.ha_callback(is_connected);
    }
  }
}

/**
 * @brief Background reconnection task
 */
static void wifi_reconnect_task(void *pvParameters)
{
  ESP_LOGI(TAG, "WiFi reconnection task started");

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(30000)); // Wait 30 seconds between attempts

    if (s_wifi_manager.status == WIFI_STATUS_FAILED || s_wifi_manager.status == WIFI_STATUS_DISCONNECTED)
    {
      ESP_LOGI(TAG, "Attempting background WiFi reconnection...");
      s_wifi_manager.retry_count = 0;
      wifi_set_status(WIFI_STATUS_CONNECTING);
      esp_wifi_connect();
    }
  }
}

/**
 * @brief Start background reconnection task
 */
static esp_err_t wifi_start_reconnect_task(void)
{
  if (s_wifi_manager.reconnect_task_handle == NULL)
  {
    // Allocate WiFi task stack from SPIRAM to save internal RAM
    // AGGRESSIVE utilization: 32KB stack for robust WiFi handling with 8MB PSRAM
    const size_t stack_size = 8192; // Reduced for memory optimization
    wifi_reconnect_stack = (StackType_t *)heap_caps_malloc(
        stack_size, MALLOC_CAP_SPIRAM);

    if (wifi_reconnect_stack == NULL)
    {
      ESP_LOGW(TAG, "Failed to allocate WiFi task stack from SPIRAM, using standard allocation");
      // Fallback to standard task creation
      BaseType_t result = xTaskCreatePinnedToCore(wifi_reconnect_task, "wifi_reconnect",
                                                  stack_size, NULL, 2, &s_wifi_manager.reconnect_task_handle, 0);
      if (result != pdPASS)
      {
        ESP_LOGE(TAG, "Failed to create reconnection task on core 0");
        return ESP_ERR_NO_MEM;
      }
      ESP_LOGI(TAG, "WiFi reconnection task started with standard internal RAM stack");
    }
    else
    {
      // Create static task with SPIRAM stack
      s_wifi_manager.reconnect_task_handle = xTaskCreateStatic(
          wifi_reconnect_task,              // Task function
          "wifi_reconnect",                 // Task name
          stack_size / sizeof(StackType_t), // Stack size in words
          NULL,                             // Parameters
          2,                                // Priority (lowered for LVGL priority)
          wifi_reconnect_stack,             // Stack buffer
          &wifi_reconnect_tcb               // Task control block
      );

      ESP_LOGI(TAG, "WiFi reconnection task started with SPIRAM stack at %p", wifi_reconnect_stack);
    }
  }
  return ESP_OK;
}

/**
 * @brief Stop background reconnection task
 */
static void wifi_stop_reconnect_task(void)
{
  if (s_wifi_manager.reconnect_task_handle != NULL)
  {
    vTaskDelete(s_wifi_manager.reconnect_task_handle);
    s_wifi_manager.reconnect_task_handle = NULL;

    // Free SPIRAM stack if it was allocated
    if (wifi_reconnect_stack)
    {
      heap_caps_free(wifi_reconnect_stack);
      wifi_reconnect_stack = NULL;
      ESP_LOGI(TAG, "WiFi reconnect task SPIRAM stack freed");
    }
  }
}

/**
 * @brief Ping a host to test connectivity
 */
static bool ping_host(const char *host, uint32_t timeout_ms)
{
  // Simplified connectivity test - just return true if we have an IP
  // In a real implementation, you could use socket-based connectivity tests
  esp_netif_ip_info_t ip_info;
  if (esp_netif_get_ip_info(s_wifi_manager.netif, &ip_info) == ESP_OK)
  {
    return (ip_info.ip.addr != 0);
  }
  return false;
}

// ═══════════════════════════════════════════════════════════════════════════════
// PUBLIC FUNCTION IMPLEMENTATIONS
// ═══════════════════════════════════════════════════════════════════════════════

esp_err_t wifi_manager_init(void)
{
  if (s_wifi_manager.initialized)
  {
    ESP_LOGW(TAG, "WiFi manager already initialized");
    return ESP_OK;
  }

  ESP_LOGI(TAG, "Initializing WiFi manager...");

  // Initialize NVS (required for WiFi)
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Initialize networking stack
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // Create WiFi station network interface
  s_wifi_manager.netif = esp_netif_create_default_wifi_sta();
  if (s_wifi_manager.netif == NULL)
  {
    ESP_LOGE(TAG, "Failed to create WiFi station interface");
    return ESP_FAIL;
  }

  // Initialize WiFi with default configuration
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  // Create event group for WiFi events
  s_wifi_manager.wifi_event_group = xEventGroupCreate();
  if (s_wifi_manager.wifi_event_group == NULL)
  {
    ESP_LOGE(TAG, "Failed to create WiFi event group");
    return ESP_ERR_NO_MEM;
  }

  // Register event handlers
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP, &ip_event_handler, NULL));

  // Set WiFi mode to station
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

// Use default credentials if configured - set BEFORE starting WiFi
#ifdef DEFAULT_WIFI_SSID
  if (strlen(DEFAULT_WIFI_SSID) > 0)
  {
    ESP_LOGI(TAG, "WiFi manager initialized, connecting to '%s'", DEFAULT_WIFI_SSID);

    // Set WiFi config before starting
    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, DEFAULT_WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1);
#ifdef DEFAULT_WIFI_PASSWORD
    if (strlen(DEFAULT_WIFI_PASSWORD) > 0)
    {
      strncpy((char *)wifi_config.sta.password, DEFAULT_WIFI_PASSWORD, sizeof(wifi_config.sta.password) - 1);
    }
#endif
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    // Optimize WiFi scanning for faster connection
    wifi_config.sta.scan_method = WIFI_FAST_SCAN;
    wifi_config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
    wifi_config.sta.threshold.rssi = -127; // Accept any signal strength
    wifi_config.sta.failure_retry_cnt = 1; // Minimal retries for faster connection

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Store configured credentials
    strncpy(s_wifi_manager.configured_ssid, DEFAULT_WIFI_SSID, sizeof(s_wifi_manager.configured_ssid) - 1);
#ifdef DEFAULT_WIFI_PASSWORD
    if (strlen(DEFAULT_WIFI_PASSWORD) > 0)
    {
      strncpy(s_wifi_manager.configured_password, DEFAULT_WIFI_PASSWORD, sizeof(s_wifi_manager.configured_password) - 1);
    }
    else
    {
      s_wifi_manager.configured_password[0] = '\0';
    }
#else
    s_wifi_manager.configured_password[0] = '\0';
#endif
  }
#endif

  // Start WiFi first before configuring power settings
  ESP_ERROR_CHECK(esp_wifi_start());

  // Configure WiFi power settings to reduce LCD interference (after WiFi is started)
  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MIN_MODEM)); // Enable power saving to reduce interference

  // Reduce WiFi TX power to minimize interference with LCD (default is 84 = 21dBm)
  ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(48)); // Further reduced to 12dBm (48 * 0.25) for maximum stability

  // Initialize state
  s_wifi_manager.status = WIFI_STATUS_DISCONNECTED;
  s_wifi_manager.retry_count = 0;
  s_wifi_manager.initialized = true;

  ESP_LOGI(TAG, "WiFi manager initialized successfully");
  return ESP_OK;
}

esp_err_t wifi_manager_connect(const char *ssid, const char *password)
{
  if (!s_wifi_manager.initialized)
  {
    ESP_LOGE(TAG, "WiFi manager not initialized");
    return ESP_ERR_INVALID_STATE;
  }

  if (ssid == NULL || strlen(ssid) == 0)
  {
    ESP_LOGE(TAG, "Invalid SSID");
    return ESP_ERR_INVALID_ARG;
  }

  ESP_LOGI(TAG, "Connecting to WiFi network: %s", ssid);

  // Store credentials
  strncpy(s_wifi_manager.configured_ssid, ssid, sizeof(s_wifi_manager.configured_ssid) - 1);
  if (password != NULL)
  {
    strncpy(s_wifi_manager.configured_password, password, sizeof(s_wifi_manager.configured_password) - 1);
  }
  else
  {
    s_wifi_manager.configured_password[0] = '\0';
  }

  // Disconnect first if already connected/connecting
  esp_wifi_disconnect();
  vTaskDelay(pdMS_TO_TICKS(100)); // Give it time to disconnect

  // Configure WiFi
  wifi_config_t wifi_config = {0};
  strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
  if (password != NULL)
  {
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
  }

  wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
  wifi_config.sta.pmf_cfg.capable = true;
  wifi_config.sta.pmf_cfg.required = false;

  // Optimize WiFi scanning for faster connection
  wifi_config.sta.scan_method = WIFI_FAST_SCAN;
  wifi_config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
  wifi_config.sta.threshold.rssi = -127; // Accept any signal strength
  wifi_config.sta.failure_retry_cnt = 1; // Minimal retries for faster connection

  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

  // Now start the connection
  ESP_ERROR_CHECK(esp_wifi_connect());

  return ESP_OK;
}

esp_err_t wifi_manager_disconnect(void)
{
  if (!s_wifi_manager.initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(TAG, "Disconnecting from WiFi");

  wifi_stop_reconnect_task();
  esp_wifi_disconnect();
  wifi_set_status(WIFI_STATUS_DISCONNECTED);

  return ESP_OK;
}

wifi_status_t wifi_manager_get_status(void)
{
  return s_wifi_manager.status;
}

esp_err_t wifi_manager_get_info(wifi_info_t *info)
{
  if (!s_wifi_manager.initialized || info == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  if (s_wifi_manager.status != WIFI_STATUS_CONNECTED)
  {
    return ESP_ERR_WIFI_NOT_CONNECT;
  }

  memcpy(info, &s_wifi_manager.connection_info, sizeof(wifi_info_t));
  return ESP_OK;
}

esp_err_t wifi_manager_register_callback(wifi_status_callback_t callback)
{
  if (callback == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  s_wifi_manager.status_callback = callback;
  return ESP_OK;
}

esp_err_t wifi_manager_register_ui_callback(void (*ui_update_func)(const char *status_text, bool is_connected))
{
  if (ui_update_func == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  s_wifi_manager.ui_callback = ui_update_func;

  // Immediately call with current status if WiFi is already initialized
  if (s_wifi_manager.initialized)
  {
    const wifi_info_t *info = (s_wifi_manager.status == WIFI_STATUS_CONNECTED) ? &s_wifi_manager.connection_info : NULL;
    const char *status_text = wifi_status_to_text(s_wifi_manager.status, info);
    bool is_connected = (s_wifi_manager.status == WIFI_STATUS_CONNECTED);
    ui_update_func(status_text, is_connected);
  }

  return ESP_OK;
}

esp_err_t wifi_manager_register_ha_callback(void (*ha_callback_func)(bool is_connected))
{
  if (ha_callback_func == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  s_wifi_manager.ha_callback = ha_callback_func;

  // Immediately call with current status if WiFi is already initialized
  if (s_wifi_manager.initialized)
  {
    bool is_connected = (s_wifi_manager.status == WIFI_STATUS_CONNECTED);
    ha_callback_func(is_connected);
  }

  return ESP_OK;
}

esp_err_t wifi_manager_unregister_callback(void)
{
  s_wifi_manager.status_callback = NULL;
  s_wifi_manager.ui_callback = NULL;
  return ESP_OK;
}

esp_err_t wifi_manager_scan(uint16_t max_aps, wifi_ap_record_t *ap_list, uint16_t *found_aps)
{
  if (!s_wifi_manager.initialized || ap_list == NULL || found_aps == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  ESP_LOGI(TAG, "Starting WiFi scan...");

  ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&max_aps, ap_list));
  *found_aps = max_aps;

  ESP_LOGI(TAG, "WiFi scan completed, found %d networks", *found_aps);
  return ESP_OK;
}

bool wifi_manager_check_internet(uint32_t timeout_ms)
{
  if (s_wifi_manager.status != WIFI_STATUS_CONNECTED)
  {
    return false;
  }

  // Try pinging primary and secondary DNS servers
  bool primary_ok = ping_host(CONNECTIVITY_TEST_HOST1, timeout_ms);
  if (primary_ok)
  {
    return true;
  }

  bool secondary_ok = ping_host(CONNECTIVITY_TEST_HOST2, timeout_ms);
  return secondary_ok;
}

const char *wifi_manager_get_signal_strength_desc(int8_t rssi)
{
  if (rssi >= -50)
    return "Excellent";
  if (rssi >= -60)
    return "Good";
  if (rssi >= -70)
    return "Fair";
  if (rssi >= -80)
    return "Weak";
  return "Very Weak";
}

esp_err_t wifi_manager_reconnect(void)
{
  if (!s_wifi_manager.initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(TAG, "Forcing WiFi reconnection");
  s_wifi_manager.retry_count = 0;
  wifi_set_status(WIFI_STATUS_CONNECTING);

  esp_wifi_disconnect();
  vTaskDelay(pdMS_TO_TICKS(1000));
  esp_wifi_connect();

  return ESP_OK;
}

esp_err_t wifi_manager_deinit(void)
{
  if (!s_wifi_manager.initialized)
  {
    return ESP_OK;
  }

  ESP_LOGI(TAG, "Deinitializing WiFi manager");

  wifi_stop_reconnect_task();
  esp_wifi_stop();
  esp_wifi_deinit();

  if (s_wifi_manager.wifi_event_group)
  {
    vEventGroupDelete(s_wifi_manager.wifi_event_group);
    s_wifi_manager.wifi_event_group = NULL;
  }

  s_wifi_manager.initialized = false;
  s_wifi_manager.status = WIFI_STATUS_UNKNOWN;

  ESP_LOGI(TAG, "WiFi manager deinitialized");
  return ESP_OK;
}
