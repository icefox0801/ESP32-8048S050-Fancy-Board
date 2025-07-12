/**
 * @file smart_config_template.h
 * @brief Smart Home Configuration Template
 *
 * This file contains Home Assistant API configuration template.
 * Copy this to smart_config.h and fill in your actual values.
 * Keep smart_config.h private and do not commit sensitive tokens to version control.
 */

#ifndef SMART_CONFIG_H
#define SMART_CONFIG_H

// ═══════════════════════════════════════════════════════════════════════════════
// HOME ASSISTANT CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════

// Home Assistant Server Settings
#define HA_SERVER_HOST_NAME "homeassistant"              // Your Home Assistant hostname (or IP)
#define HA_SERVER_PORT 8123                              // Home Assistant port (usually 8123)
#define HA_API_TOKEN "YOUR_LONG_LIVED_ACCESS_TOKEN_HERE" // Your HA long-lived access token

// Helper macros for URL construction
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// Home Assistant API Endpoints
#define HA_API_BASE_URL "http://" HA_SERVER_HOST_NAME ":" TOSTRING(HA_SERVER_PORT) "/api"
#define HA_API_STATES_URL HA_API_BASE_URL "/states"
#define HA_API_SERVICES_URL HA_API_BASE_URL "/services"

// ═══════════════════════════════════════════════════════════════════════════════
// SMART HOME ENTITY CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════

// Smart Switches (Replace with your actual entity IDs)
#define HA_ENTITY_A "switch.your_water_pump_switch" // Water Pump Control
#define HA_ENTITY_B "switch.your_wave_maker_switch" // Wave Maker Control
#define HA_ENTITY_C "switch.your_light_switch"      // Light Switch Control

// Scene Control
#define HA_ENTITY_D "scene.your_feed_scene" // Scene trigger button

// ═══════════════════════════════════════════════════════════════════════════════
// UI LABELS AND BUTTON TEXT CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════

// Switch Labels for UI Display
#define UI_LABEL_A "Water Pump"
#define UI_LABEL_B "Wave Maker"
#define UI_LABEL_C "Light Switch"
#define UI_LABEL_D "FEED"

// Button Text Configuration
#define UI_BUTTON_ON "ON"
#define UI_BUTTON_OFF "OFF"
#define UI_BUTTON_FEED "FEED"

// ═══════════════════════════════════════════════════════════════════════════════
// HTTP CLIENT CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════

// HTTP Request Configuration
#define HA_HTTP_TIMEOUT_MS 10000
#define HA_MAX_RESPONSE_SIZE 4096

// API Call Intervals
#define HA_STATUS_UPDATE_INTERVAL_MS 5000 // Status check every 5 seconds

// Sync Configuration
#define HA_SYNC_RETRY_COUNT 3          // Number of sync attempts before disabling
#define HA_SYNC_TIMEOUT_MS 5000        // Timeout for sync operations
#define HA_SYNC_CHECK_INTERVAL_MS 2000 // How often to check sync status

// ═══════════════════════════════════════════════════════════════════════════════
// SMART HOME AUTOMATION SETTINGS
// ═══════════════════════════════════════════════════════════════════════════════

// Automation Features
#define ENABLE_A_CONTROL 1 // Enable water pump control
#define ENABLE_B_CONTROL 1 // Enable wave maker control
#define ENABLE_C_CONTROL 1 // Enable light switch control
#define ENABLE_D_CONTROL 1 // Enable scene trigger button

// Device Control Preferences
#define A_AUTO_TURN_OFF_DELAY_MS 300000 // Auto turn off water pump after 5 minutes
#define B_AUTO_TURN_OFF_DELAY_MS 600000 // Auto turn off wave maker after 10 minutes
#define D_TRIGGER_COOLDOWN_MS 5000      // Prevent rapid scene triggering

#endif // SMART_CONFIG_H
