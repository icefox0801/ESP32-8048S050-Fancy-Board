/**
 * @file wifi_config_example.h
 * @brief WiFi Configuration Template
 *
 * Copy this file to wifi_config.h and update with your WiFi credentials.
 * This file is included in .gitignore to keep your credentials private.
 *
 * Alternatively, you can configure WiFi settings using:
 * idf.py menuconfig -> WiFi Configuration
 */

#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

// WiFi Network Credentials
// Replace these with your actual WiFi network settings
#define WIFI_SSID "Your_WiFi_Network_Name"
#define WIFI_PASSWORD "Your_WiFi_Password"

// Advanced WiFi Settings (optional)
#define WIFI_MAXIMUM_RETRY 5
#define WIFI_CONNECT_TIMEOUT_MS 30000
#define WIFI_SCAN_RSSI_THRESHOLD -80

// WiFi Features
#define ENABLE_WIFI_AUTO_CONNECT 1
#define ENABLE_WIFI_RECONNECT 1
#define ENABLE_WIFI_STATUS_LOG 1

#endif // WIFI_CONFIG_H
