/**
 * @file wifi_config_template.h
 * @brief WiFi Configuration Template
 *
 * This file is a template for WiFi network credentials and settings.
 * Copy this to wifi_config.h and fill in your actual WiFi credentials.
 * Keep wifi_config.h private and do not commit sensitive passwords to version control.
 *
 */

#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#define WIFI_SSID "YOUR_WIFI_SSID"         // Replace with your WiFi network name
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD" // Replace with your WiFi password

// Event group bits (used by wifi_manager.c)
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

// Connection behavior (used by wifi_manager.c)
#define WIFI_MAXIMUM_RETRY_COUNT 5    // Maximum connection retry attempts
#define WIFI_RECONNECT_DELAY_MS 10000 // Reconnection delay in milliseconds

#endif // WIFI_CONFIG_H
