/**
 * @file smart_config.h
 * @brief Smart Home Configuration Template
 *
 * This file contains Home Assistant API configuration and credentials.
 * Copy this to smart_config.h and fill in your actual values.
 * Keep smart_config.h private and do not commit sensitive tokens to version control.
 */

#ifndef SMART_CONFIG_H
#define SMART_CONFIG_H

// Home Assistant Configuration
#define HA_SERVER_IP "192.168.1.100"                     // Your Home Assistant server IP
#define HA_SERVER_PORT 8123                              // Home Assistant port (usually 8123)
#define HA_API_TOKEN "YOUR_LONG_LIVED_ACCESS_TOKEN_HERE" // Your HA long-lived access token

// Home Assistant API Endpoints
#define HA_API_BASE_URL "http://" HA_SERVER_IP ":" STRINGIFY(HA_SERVER_PORT) "/api"
#define HA_API_STATES_URL HA_API_BASE_URL "/states"
#define HA_API_SERVICES_URL HA_API_BASE_URL "/services"

// Helper macro to stringify port number
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// Home Assistant Entity IDs (customize for your setup)
#define HA_ENTITY_PUMP_SWITCH "switch.your_pump_switch"   // Replace with your pump switch entity ID
#define HA_ENTITY_TEMPERATURE_SENSOR "sensor.temperature" // Replace with your temperature sensor
#define HA_ENTITY_HUMIDITY_SENSOR "sensor.humidity"       // Replace with your humidity sensor
#define HA_ENTITY_PRESSURE_SENSOR "sensor.pressure"       // Replace with your pressure sensor

// HTTP Request Configuration
#define HA_HTTP_TIMEOUT_MS 10000
#define HA_MAX_RESPONSE_SIZE 4096
#define HA_REQUEST_RETRY_COUNT 3

// Smart Home Device Control Settings
#define HA_UPDATE_INTERVAL_MS 30000 // How often to update sensor data (30 seconds)
#define HA_RECONNECT_DELAY_MS 5000  // Delay before retrying failed connections

#endif // SMART_CONFIG_H
