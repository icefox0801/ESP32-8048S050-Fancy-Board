/**
 * @file wifi_manager.h
 * @brief WiFi connection and management for ESP32-S3 system monitor
 *
 * This module provides WiFi connection functionality with automatic reconnection,
 * connection status monitoring, and event handling for the system monitor dashboard.
 *
 * Features:
 * - Automatic WiFi connection with retry logic
 * - Connection status callbacks for UI updates
 * - WiFi credential configuration via menuconfig
 * - Signal strength monitoring
 * - Network time synchronization support
 *
 * @author System Monitor Dashboard
 * @date 2025-08-14
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

#ifdef __cplusplus
extern "C"
{
#endif

// ═══════════════════════════════════════════════════════════════════════════════
// CONSTANTS AND CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════

/** WiFi connection timeout in milliseconds */
#define WIFI_CONNECT_TIMEOUT_MS 30000

/** Maximum number of connection retry attempts */
#define WIFI_MAX_RETRY_COUNT 5

/** WiFi reconnection delay in milliseconds */
#define WIFI_RETRY_DELAY_MS 5000

/** WiFi scan timeout in milliseconds */
#define WIFI_SCAN_TIMEOUT_MS 10000

// Event group bits for WiFi status
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define WIFI_DISCONNECTED_BIT BIT2

  // ═══════════════════════════════════════════════════════════════════════════════
  // DATA STRUCTURES
  // ═══════════════════════════════════════════════════════════════════════════════

  /**
   * @brief WiFi connection status enumeration
   */
  typedef enum
  {
    WIFI_STATUS_DISCONNECTED = 0, ///< WiFi is disconnected
    WIFI_STATUS_CONNECTING,       ///< WiFi is attempting to connect
    WIFI_STATUS_CONNECTED,        ///< WiFi is connected successfully
    WIFI_STATUS_FAILED,           ///< WiFi connection failed
    WIFI_STATUS_RECONNECTING,     ///< WiFi is attempting to reconnect
    WIFI_STATUS_UNKNOWN           ///< WiFi status unknown
  } wifi_status_t;

  /**
   * @brief WiFi connection information structure
   */
  typedef struct
  {
    char ssid[33];              ///< Connected SSID (null-terminated)
    int8_t rssi;                ///< Signal strength in dBm
    wifi_auth_mode_t auth_mode; ///< Authentication mode
    uint8_t channel;            ///< WiFi channel
    char ip_address[16];        ///< Assigned IP address string
    char gateway[16];           ///< Gateway IP address string
    char netmask[16];           ///< Network mask string
    uint32_t connection_time;   ///< Connection timestamp (system ticks)
    bool has_internet;          ///< Internet connectivity status
  } wifi_info_t;

  /**
   * @brief WiFi status callback function type
   * @param is_connected Boolean indicating if WiFi is connected
   * @param status_text Human-readable status text
   * @param status Current WiFi status
   * @param info WiFi connection information (NULL if disconnected)
   */
  typedef void (*wifi_status_callback_t)(bool is_connected, const char *status_text, wifi_status_t status, const wifi_info_t *info);

  /**
   * @brief WiFi connected callback function type
   */
  typedef void (*wifi_connected_callback_t)(void);

  // ═══════════════════════════════════════════════════════════════════════════════
  // PUBLIC FUNCTION DECLARATIONS
  // ═══════════════════════════════════════════════════════════════════════════════

  /**
   * @brief Initialize WiFi manager and start connection process
   *
   * This function initializes the ESP32 WiFi stack, sets up event handlers,
   * and attempts to connect to the configured WiFi network.
   *
   * @return ESP_OK on successful initialization, error code otherwise
   */
  esp_err_t wifi_manager_init(void);

  /**
   * @brief Start WiFi connection with specified credentials
   *
   * @param ssid WiFi network SSID (max 32 characters)
   * @param password WiFi network password (max 63 characters)
   * @return ESP_OK on successful start, error code otherwise
   */
  esp_err_t wifi_manager_connect(const char *ssid, const char *password);

  /**
   * @brief Disconnect from WiFi and stop WiFi manager
   *
   * @return ESP_OK on successful disconnection, error code otherwise
   */
  esp_err_t wifi_manager_disconnect(void);

  /**
   * @brief Get current WiFi connection status
   *
   * @return Current WiFi status
   */
  wifi_status_t wifi_manager_get_status(void);

  /**
   * @brief Get detailed WiFi connection information
   *
   * @param info Pointer to wifi_info_t structure to fill
   * @return ESP_OK if connected and info retrieved, ESP_ERR_WIFI_NOT_CONNECT otherwise
   */
  esp_err_t wifi_manager_get_info(wifi_info_t *info);

  /**
   * @brief Register callback for WiFi status changes
   *
   * @param callback Function to call when WiFi status changes
   * @return ESP_OK on successful registration
   */
  esp_err_t wifi_manager_register_status_callback(wifi_status_callback_t callback);

  /**
   * @brief Unregister WiFi status change callback
   *
   * @return ESP_OK on successful unregistration
   */
  esp_err_t wifi_manager_unregister_callback(void);

  /**
   * @brief Register callback for WiFi connected event, only called once
   *
   * @param callback Function to call when WiFi is connected
   * @return ESP_OK on successful registration
   */
  esp_err_t wifi_manager_register_connected_callback(wifi_connected_callback_t callback);

  /**
   * @brief Unregister WiFi connected event callback
   *
   * @return ESP_OK on successful unregistration
   */
  esp_err_t wifi_manager_unregister_connected_callback(void);

  /**
   * @brief Deinitialize WiFi manager and cleanup resources
   *
   * @return ESP_OK on successful cleanup
   */
  esp_err_t wifi_manager_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_MANAGER_H
