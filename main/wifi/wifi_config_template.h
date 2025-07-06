/**
 * @file wifi_config_template.h
 * @brief WiFi Configuration Template
 *
 * This file is a template for WiFi network credentials and settings.
 * Copy this to wifi_config.h and fill in your actual WiFi credentials.
 * Keep wifi_config.h private and do not commit sensitive passwords to version control.
 */

#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

// ═══════════════════════════════════════════════════════════════════════════════
// REQUIRED WIFI CREDENTIALS
// ═══════════════════════════════════════════════════════════════════════════════

#define WIFI_SSID "YOUR_WIFI_SSID"         // Replace with your WiFi network name
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD" // Replace with your WiFi password

// ═══════════════════════════════════════════════════════════════════════════════
// OPTIONAL WIFI CONFIGURATION (uncomment and modify if needed)
// ═══════════════════════════════════════════════════════════════════════════════

// Connection behavior
// #define WIFI_MAXIMUM_RETRY 5              // Maximum connection retry attempts
// #define WIFI_CONNECT_TIMEOUT 30000        // Connection timeout in milliseconds
// #define ENABLE_WIFI_AUTO_CONNECT 1        // Auto-connect on startup (1=enable, 0=disable)

// Signal and scanning
// #define WIFI_SCAN_RSSI_THRESHOLD -127     // Minimum signal strength (dBm)
// #define WIFI_CONNECT_AP_BY_SIGNAL 0       // Connect to strongest AP with same SSID (1=enable, 0=disable)

// Advanced power management
// #define WIFI_LISTEN_INTERVAL 3            // Beacon listen interval (beacon periods)
// #define WIFI_BEACON_TIMEOUT 6             // Beacon timeout (beacon periods)
// #define WIFI_MAX_TX_POWER 84              // Max TX power in 0.25dBm units (21dBm = 84*0.25)

// Storage
// #define ENABLE_WIFI_NVS_FLASH 1           // Store WiFi settings in NVS (1=enable, 0=disable)

#endif // WIFI_CONFIG_H
