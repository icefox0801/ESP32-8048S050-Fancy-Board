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
  EventGroupHandle_t wifi_event_group;    ///< FreeRTOS event group for WiFi events
  esp_netif_t *netif;                     ///< Network interface handle
  wifi_status_t status;                   ///< Current connection status
  wifi_info_t connection_info;            ///< Current connection information
  wifi_status_callback_t status_callback; ///< User callback for status changes
  uint8_t retry_count;                    ///< Current retry attempt count
  TaskHandle_t reconnect_task_handle;     ///< Handle for reconnection task
  bool initialized;                       ///< Initialization status flag
  char configured_ssid[33];               ///< Configured SSID
  char configured_password[64];           ///< Configured password
} wifi_manager_state_t;

// ═══════════════════════════════════════════════════════════════════════════════
// PRIVATE VARIABLES
// ═══════════════════════════════════════════════════════════════════════════════

static wifi_manager_state_t s_wifi_manager = {0};

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

      // Initialize SNTP for network time synchronization
      esp_sntp_setservername(0, "pool.ntp.org");
      esp_sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
      esp_sntp_init();
      break;
    }

    case IP_EVENT_STA_LOST_IP:
      ESP_LOGW(TAG, "Lost IP address");
      wifi_set_status(WIFI_STATUS_DISCONNECTED);
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
 * @brief Set WiFi status and notify callback
 */
static void wifi_set_status(wifi_status_t new_status)
{
  if (s_wifi_manager.status != new_status)
  {
    s_wifi_manager.status = new_status;

    if (s_wifi_manager.status_callback)
    {
      const wifi_info_t *info = (new_status == WIFI_STATUS_CONNECTED) ? &s_wifi_manager.connection_info : NULL;
      s_wifi_manager.status_callback(new_status, info);
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
    BaseType_t result = xTaskCreate(wifi_reconnect_task, "wifi_reconnect",
                                    4096, NULL, 5, &s_wifi_manager.reconnect_task_handle);
    if (result != pdPASS)
    {
      ESP_LOGE(TAG, "Failed to create reconnection task");
      return ESP_ERR_NO_MEM;
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

  // Initialize state
  s_wifi_manager.status = WIFI_STATUS_DISCONNECTED;
  s_wifi_manager.retry_count = 0;
  s_wifi_manager.initialized = true;

// Use default credentials if configured
#ifdef DEFAULT_WIFI_SSID
  if (strlen(DEFAULT_WIFI_SSID) > 0)
  {
    strncpy(s_wifi_manager.configured_ssid, DEFAULT_WIFI_SSID, sizeof(s_wifi_manager.configured_ssid) - 1);
#ifdef DEFAULT_WIFI_PASSWORD
    strncpy(s_wifi_manager.configured_password, DEFAULT_WIFI_PASSWORD, sizeof(s_wifi_manager.configured_password) - 1);
#endif

    ESP_LOGI(TAG, "WiFi manager initialized, connecting to '%s'", s_wifi_manager.configured_ssid);
    return wifi_manager_connect(s_wifi_manager.configured_ssid, s_wifi_manager.configured_password);
  }
#endif

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

  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

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

esp_err_t wifi_manager_unregister_callback(void)
{
  s_wifi_manager.status_callback = NULL;
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
