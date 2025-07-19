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
#define HA_ENTITY_A_ID "switch.your_switch_a_entity_id" // Switch A Control
#define HA_ENTITY_B_ID "switch.your_switch_b_entity_id" // Switch B Control
#define HA_ENTITY_C_ID "switch.your_switch_c_entity_id" // Switch C Control

// Scene Control
#define HA_ENTITY_D_ID "scene.your_scene_entity_id" // Scene trigger button

#define HA_ENTITY_A_LABEL "Switch A"
#define HA_ENTITY_B_LABEL "Switch B"
#define HA_ENTITY_C_LABEL "Switch C"
#define HA_ENTITY_D_LABEL "Scene"

// ═══════════════════════════════════════════════════════════════════════════════
// HTTP CLIENT CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════

// HTTP Request Configuration
#define HA_HTTP_TIMEOUT_MS 15000
#define HA_MAX_RESPONSE_SIZE 131072

// API Call Intervals
#define HA_STATUS_UPDATE_INTERVAL_MS 5000 // Status check every 5 seconds

// Sync Configuration
#define HA_SYNC_RETRY_COUNT 3 // Number of sync attempts before disabling

#endif // SMART_CONFIG_H
