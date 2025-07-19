#include "wifi_manager.h"
#include "system_debug_utils.h"
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_http_client.h>
#include <nvs_flash.h>
#include <lwip/err.h>
#include <lwip/sys.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>

// Event group bits
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

// Retry settings
#define WIFI_MAXIMUM_RETRY_COUNT 5
#define WIFI_RECONNECT_DELAY_MS 10000

// Static variables
typedef struct
{
  bool initialized;
  wifi_status_t status;
  int retry_count;
  wifi_info_t connection_info;
  void (*ui_callback)(const char *status_text, bool is_connected);
  void (*ha_callback)(bool is_connected);
  wifi_status_callback_t status_callback;
  TaskHandle_t reconnect_task_handle;
} wifi_manager_internal_t;

static wifi_manager_internal_t s_wifi_manager = {
    .initialized = false,
    .status = WIFI_STATUS_DISCONNECTED,
    .retry_count = 0,
    .connection_info = {{0}},
    .status_callback = NULL,
    .reconnect_task_handle = NULL};

static EventGroupHandle_t s_wifi_event_group;

// Function declarations
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void wifi_update_connection_info(void);
static void wifi_set_status(wifi_status_t new_status);
static void wifi_reconnect_task(void *pvParameters);
static esp_err_t wifi_start_reconnect_task(void);
static void wifi_stop_reconnect_task(void);
static bool ping_host(const char *host, uint32_t timeout_ms);
static const char *wifi_status_to_text(wifi_status_t status, const wifi_info_t *info);

// Event handlers
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  switch (event_id)
  {
  case WIFI_EVENT_STA_START:
    wifi_set_status(WIFI_STATUS_CONNECTING);
    esp_wifi_connect();
    break;

  case WIFI_EVENT_STA_CONNECTED:
    wifi_update_connection_info();
    wifi_set_status(WIFI_STATUS_CONNECTED);
    wifi_stop_reconnect_task();
    s_wifi_manager.retry_count = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    debug_log_info(DEBUG_TAG_WIFI_MANAGER, "WiFi connected successfully");
    break;

  case WIFI_EVENT_STA_DISCONNECTED:
  {
    wifi_event_sta_disconnected_t *disconnected = (wifi_event_sta_disconnected_t *)event_data;
    debug_log_warning_f(DEBUG_TAG_WIFI_MANAGER, "WiFi disconnected (reason: %d)", disconnected->reason);

    wifi_set_status(WIFI_STATUS_DISCONNECTED);
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

    s_wifi_manager.retry_count++;
    if (s_wifi_manager.retry_count < WIFI_MAXIMUM_RETRY_COUNT)
    {
      debug_log_info_f(DEBUG_TAG_WIFI_MANAGER, "Retrying connection (attempt %d/%d)",
                       s_wifi_manager.retry_count, WIFI_MAXIMUM_RETRY_COUNT);
      esp_wifi_connect();
    }
    else
    {
      debug_log_error(DEBUG_TAG_WIFI_MANAGER, "Maximum retry attempts reached, starting background reconnection");
      wifi_start_reconnect_task(); // Start background reconnection
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
    break;
  }

  default:
    break;
  }
}

static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  switch (event_id)
  {
  case IP_EVENT_STA_GOT_IP:
  {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    snprintf(s_wifi_manager.connection_info.ip_address, sizeof(s_wifi_manager.connection_info.ip_address),
             IPSTR, IP2STR(&event->ip_info.ip));
    snprintf(s_wifi_manager.connection_info.netmask, sizeof(s_wifi_manager.connection_info.netmask),
             IPSTR, IP2STR(&event->ip_info.netmask));
    snprintf(s_wifi_manager.connection_info.gateway, sizeof(s_wifi_manager.connection_info.gateway),
             IPSTR, IP2STR(&event->ip_info.gw));

    debug_log_info_f(DEBUG_TAG_WIFI_MANAGER, "Got IP address: %s", s_wifi_manager.connection_info.ip_address);
    wifi_set_status(WIFI_STATUS_CONNECTED);
    wifi_stop_reconnect_task();
    s_wifi_manager.retry_count = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    break;
  }

  case IP_EVENT_STA_LOST_IP:
    debug_log_warning(DEBUG_TAG_WIFI_MANAGER, "Lost IP address");
    wifi_set_status(WIFI_STATUS_DISCONNECTED);
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    break;

  default:
    break;
  }
}

static void wifi_update_connection_info(void)
{
  wifi_ap_record_t ap_info;
  if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK)
  {
    strlcpy(s_wifi_manager.connection_info.ssid, (char *)ap_info.ssid,
            sizeof(s_wifi_manager.connection_info.ssid));
    s_wifi_manager.connection_info.rssi = ap_info.rssi;
    s_wifi_manager.connection_info.channel = ap_info.primary;

    debug_log_info_f(DEBUG_TAG_WIFI_MANAGER, "Connected to: %s, RSSI: %d dBm, Channel: %d",
                     s_wifi_manager.connection_info.ssid,
                     s_wifi_manager.connection_info.rssi,
                     s_wifi_manager.connection_info.channel);
  }
}

static const char *wifi_status_to_text(wifi_status_t status, const wifi_info_t *info)
{
  static char status_text[128];

  switch (status)
  {
  case WIFI_STATUS_DISCONNECTED:
    return "Disconnected";

  case WIFI_STATUS_CONNECTING:
    return "Connecting...";

  case WIFI_STATUS_CONNECTED:
    if (info && strlen(info->ssid) > 0)
    {
      snprintf(status_text, sizeof(status_text), "Connected to %s", info->ssid);
      return status_text;
    }
    return "Connected";

  case WIFI_STATUS_FAILED:
    return "Connection Failed";

  default:
    return "Unknown";
  }
}

static void wifi_set_status(wifi_status_t new_status)
{
  if (s_wifi_manager.status != new_status)
  {
    s_wifi_manager.status = new_status;

    const char *status_text = wifi_status_to_text(new_status, &s_wifi_manager.connection_info);
    bool is_connected = (new_status == WIFI_STATUS_CONNECTED);

    debug_log_info_f(DEBUG_TAG_WIFI_MANAGER, "WiFi status changed to: %s", status_text);

    // Notify callbacks
    if (s_wifi_manager.status_callback)
    {
      s_wifi_manager.status_callback(is_connected, status_text, new_status, &s_wifi_manager.connection_info);
    }
  }
}

static void wifi_reconnect_task(void *pvParameters)
{
  debug_log_info(DEBUG_TAG_WIFI_MANAGER, "WiFi reconnect task started");

  while (true)
  {
    vTaskDelay(pdMS_TO_TICKS(WIFI_RECONNECT_DELAY_MS));

    if (s_wifi_manager.status != WIFI_STATUS_CONNECTED)
    {
      debug_log_info(DEBUG_TAG_WIFI_MANAGER, "Attempting WiFi reconnection...");
      s_wifi_manager.retry_count = 0;
      esp_wifi_connect();
    }
    else
    {
      debug_log_info(DEBUG_TAG_WIFI_MANAGER, "WiFi reconnected successfully, stopping reconnect task");
      break;
    }
  }

  s_wifi_manager.reconnect_task_handle = NULL;
  vTaskDelete(NULL);
}

static esp_err_t wifi_start_reconnect_task(void)
{
  if (s_wifi_manager.reconnect_task_handle != NULL)
  {
    debug_log_warning(DEBUG_TAG_WIFI_MANAGER, "Reconnect task already running");
    return ESP_OK;
  }

  BaseType_t result = xTaskCreate(
      wifi_reconnect_task,
      "wifi_reconnect",
      4096,
      NULL,
      5,
      &s_wifi_manager.reconnect_task_handle);

  if (result != pdPASS)
  {
    debug_log_error(DEBUG_TAG_WIFI_MANAGER, "Failed to create WiFi reconnect task");
    return ESP_FAIL;
  }

  debug_log_info(DEBUG_TAG_WIFI_MANAGER, "WiFi reconnect task started");
  return ESP_OK;
}

static void wifi_stop_reconnect_task(void)
{
  if (s_wifi_manager.reconnect_task_handle != NULL)
  {
    vTaskDelete(s_wifi_manager.reconnect_task_handle);
    s_wifi_manager.reconnect_task_handle = NULL;
    debug_log_info(DEBUG_TAG_WIFI_MANAGER, "WiFi reconnect task stopped");
  }
}

static bool ping_host(const char *host, uint32_t timeout_ms)
{
  // Simplified ping - just return true for now
  // Full implementation would require ICMP socket programming
  (void)host;
  (void)timeout_ms;
  return true;
}

// Public API functions
esp_err_t wifi_manager_init(void)
{
  if (s_wifi_manager.initialized)
  {
    debug_log_warning(DEBUG_TAG_WIFI_MANAGER, "WiFi manager already initialized");
    return ESP_OK;
  }

  debug_log_startup(DEBUG_TAG_WIFI_MANAGER, "WiFi Manager");

  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Initialize TCP/IP stack
  ESP_ERROR_CHECK(esp_netif_init());

  // Create event loop
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // Create WiFi station interface
  esp_netif_create_default_wifi_sta();

  // Initialize WiFi with default config
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  // Create event group
  s_wifi_event_group = xEventGroupCreate();
  if (s_wifi_event_group == NULL)
  {
    debug_log_error(DEBUG_TAG_WIFI_MANAGER, "Failed to create WiFi event group");
    return ESP_FAIL;
  }

  // Register event handlers
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP, &ip_event_handler, NULL));

  // Set WiFi mode to station
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

  // Start WiFi
  ESP_ERROR_CHECK(esp_wifi_start());

  s_wifi_manager.initialized = true;
  wifi_set_status(WIFI_STATUS_DISCONNECTED);

  debug_log_info(DEBUG_TAG_WIFI_MANAGER, "WiFi manager initialized successfully");
  return ESP_OK;
}

esp_err_t wifi_manager_connect(const char *ssid, const char *password)
{
  if (!s_wifi_manager.initialized)
  {
    debug_log_error(DEBUG_TAG_WIFI_MANAGER, "WiFi manager not initialized");
    return ESP_ERR_INVALID_STATE;
  }

  if (!ssid || strlen(ssid) == 0)
  {
    debug_log_error(DEBUG_TAG_WIFI_MANAGER, "Invalid SSID provided");
    return ESP_ERR_INVALID_ARG;
  }

  debug_log_info_f(DEBUG_TAG_WIFI_MANAGER, "Connecting to WiFi network: %s", ssid);

  // Stop any existing reconnect task
  wifi_stop_reconnect_task();

  // Configure WiFi credentials
  wifi_config_t wifi_config = {0};
  strlcpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
  if (password)
  {
    strlcpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
  }
  wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
  wifi_config.sta.pmf_cfg.capable = true;
  wifi_config.sta.pmf_cfg.required = false;

  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

  // Reset retry count and connect
  s_wifi_manager.retry_count = 0;
  wifi_set_status(WIFI_STATUS_CONNECTING);

  esp_err_t result = esp_wifi_connect();
  if (result != ESP_OK)
  {
    debug_log_error_f(DEBUG_TAG_WIFI_MANAGER, "Failed to initiate WiFi connection: %s", esp_err_to_name(result));
    wifi_set_status(WIFI_STATUS_FAILED);
    return result;
  }

  return ESP_OK;
}

esp_err_t wifi_manager_disconnect(void)
{
  if (!s_wifi_manager.initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  wifi_stop_reconnect_task();
  esp_err_t result = esp_wifi_disconnect();
  wifi_set_status(WIFI_STATUS_DISCONNECTED);

  debug_log_info(DEBUG_TAG_WIFI_MANAGER, "WiFi disconnected");
  return result;
}

wifi_status_t wifi_manager_get_status(void)
{
  return s_wifi_manager.status;
}

esp_err_t wifi_manager_get_info(wifi_info_t *info)
{
  if (!info)
  {
    return ESP_ERR_INVALID_ARG;
  }

  memcpy(info, &s_wifi_manager.connection_info, sizeof(wifi_info_t));
  return ESP_OK;
}

esp_err_t wifi_manager_register_status_callback(wifi_status_callback_t callback)
{
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
  if (!s_wifi_manager.initialized || !ap_list || !found_aps)
  {
    return ESP_ERR_INVALID_ARG;
  }

  wifi_scan_config_t scan_config = {
      .ssid = NULL,
      .bssid = NULL,
      .channel = 0,
      .show_hidden = false};

  ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&max_aps, ap_list));
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(found_aps));

  return ESP_OK;
}

bool wifi_manager_check_internet(uint32_t timeout_ms)
{
  if (s_wifi_manager.status != WIFI_STATUS_CONNECTED)
  {
    return false;
  }

  return ping_host("8.8.8.8", timeout_ms);
}

const char *wifi_manager_get_signal_strength_desc(int8_t rssi)
{
  if (rssi >= -30)
    return "Excellent";
  if (rssi >= -67)
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

  debug_log_info(DEBUG_TAG_WIFI_MANAGER, "Manual WiFi reconnection requested");
  s_wifi_manager.retry_count = 0;
  wifi_set_status(WIFI_STATUS_CONNECTING);

  return esp_wifi_connect();
}

esp_err_t wifi_manager_deinit(void)
{
  if (!s_wifi_manager.initialized)
  {
    return ESP_OK;
  }

  wifi_stop_reconnect_task();
  esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);
  esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler);
  esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_LOST_IP, &ip_event_handler);

  esp_wifi_stop();
  esp_wifi_deinit();

  if (s_wifi_event_group)
  {
    vEventGroupDelete(s_wifi_event_group);
    s_wifi_event_group = NULL;
  }

  s_wifi_manager.initialized = false;
  debug_log_info(DEBUG_TAG_WIFI_MANAGER, "WiFi manager deinitialized");

  return ESP_OK;
}
