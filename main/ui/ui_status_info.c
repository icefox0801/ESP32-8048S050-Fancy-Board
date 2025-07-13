
/**
 * @file ui_status_info.c
 * @brief Status Information Panel Implementation
 *
 * Creates the status information panel with connection status and WiFi info
 * for the ESP32-S3-8048S050 system monitor dashboard.
 */

#include "ui_status_info.h"
#include "ui_config.h"
#include "ui_helpers.h"
#include "lvgl_setup.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

// Status and Info Elements
static lv_obj_t *timestamp_label = NULL;
static lv_obj_t *connection_status_label = NULL;
static lv_obj_t *wifi_status_label = NULL;

/**
 * @brief Create the status panel with connection info
 * @param parent Parent screen object
 * @return Created status panel
 */
lv_obj_t *create_status_info_panel(lv_obj_t *parent)
{
  lv_obj_t *status_panel = ui_create_status_panel(parent, 780, 50, 10, 410, 0x0f0f0f, 0x222222);

  // Serial connection status with last update time (left side)
  connection_status_label = lv_label_create(status_panel);
  lv_label_set_text(connection_status_label, "[SERIAL] Waiting... | Last: Never");
  lv_obj_set_style_text_font(connection_status_label, font_small, 0);
  lv_obj_set_style_text_color(connection_status_label, lv_color_hex(0xffaa00), 0);
  lv_obj_set_pos(connection_status_label, 10, 11);

  // WiFi status (right side)
  wifi_status_label = lv_label_create(status_panel);
  lv_label_set_text(wifi_status_label, "[WIFI] Connecting...");
  lv_obj_set_style_text_font(wifi_status_label, font_small, 0);
  lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0x00aaff), 0);
  lv_obj_align(wifi_status_label, LV_ALIGN_TOP_RIGHT, -10, 11);

  // Hidden timestamp label for internal timestamp tracking
  timestamp_label = lv_label_create(status_panel);
  lv_label_set_text(timestamp_label, "Last: Never");
  lv_obj_add_flag(timestamp_label, LV_OBJ_FLAG_HIDDEN); // Hide this element

  return status_panel;
}

// ══════════════════════════════════════════════════════════════════════════════�?
// CONNECTION STATUS MANAGEMENT
// ══════════════════════════════════════════════════════════════════════════════�?

/**
 * @brief Update connection status indicator
 * @param connected True if connection is active, false if lost
 * @note Changes color and text of status indicator based on connection state
 */
void system_monitor_ui_set_connection_status(bool connected)
{
  if (!connection_status_label)
    return;

  lvgl_lock_acquire();

  // Get current timestamp from the hidden timestamp label
  const char *current_timestamp = "Last: Never";
  if (timestamp_label)
  {
    current_timestamp = lv_label_get_text(timestamp_label);
  }

  char combined_status[128];
  if (connected)
  {
    snprintf(combined_status, sizeof(combined_status), "[SERIAL] Connected | %s", current_timestamp);
    lv_label_set_text(connection_status_label, combined_status);
    lv_obj_set_style_text_color(connection_status_label, lv_color_hex(0x00ff88), 0); // Green
  }
  else
  {
    snprintf(combined_status, sizeof(combined_status), "[SERIAL] Connection Lost | %s", current_timestamp);
    lv_label_set_text(connection_status_label, combined_status);
    lv_obj_set_style_text_color(connection_status_label, lv_color_hex(0xff4444), 0); // Red
  }

  lvgl_lock_release();
}

/**
 * @brief Update WiFi connection status in the status panel
 * @param status_text WiFi status message to display
 * @param connected True if WiFi is connected, false otherwise
 */
void system_monitor_ui_update_wifi_status(const char *status_text, bool connected)
{
  if (!wifi_status_label || !status_text)
    return;

  lvgl_lock_acquire();

  // Create formatted status message
  char wifi_msg[128];

  if (connected && strstr(status_text, "Connected:") != NULL)
  {
    // Extract SSID from "Connected: SSID (IP)" format
    const char *ssid_start = strstr(status_text, "Connected: ") + 11; // Skip "Connected: "
    const char *ssid_end = strchr(ssid_start, ' ');                   // Find space before (IP)

    if (ssid_end != NULL)
    {
      // Extract SSID
      size_t ssid_len = ssid_end - ssid_start;
      char ssid[64];
      strncpy(ssid, ssid_start, ssid_len);
      ssid[ssid_len] = '\0';

      snprintf(wifi_msg, sizeof(wifi_msg), "[WIFI:%s] Connected", ssid);
    }
    else
    {
      // Fallback if format is unexpected
      snprintf(wifi_msg, sizeof(wifi_msg), "[WIFI] %s", status_text);
    }
  }
  else
  {
    // For non-connected states, use normal format
    snprintf(wifi_msg, sizeof(wifi_msg), "[WIFI] %s", status_text);
  }

  lv_label_set_text(wifi_status_label, wifi_msg);

  // Set color based on connection status
  if (connected)
  {
    lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0x00ff88), 0); // Green
  }
  else
  {
    lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xff4444), 0); // Red
  }

  lvgl_lock_release();
}

/**
 * @brief Update serial connection status display
 * @param status_text Serial status message to display
 * @param connected True if serial is connected, false otherwise
 */
void status_info_update_serial_status(const char *status_text, bool connected)
{
  if (!connection_status_label || !status_text)
    return;

  lvgl_lock_acquire();

  // Get current timestamp from the hidden timestamp label
  const char *current_timestamp = "Last: Never";
  if (timestamp_label)
  {
    current_timestamp = lv_label_get_text(timestamp_label);
  }

  char combined_status[128];
  if (connected)
  {
    snprintf(combined_status, sizeof(combined_status), "[SERIAL] Connected | %s", current_timestamp);
    lv_label_set_text(connection_status_label, combined_status);
    lv_obj_set_style_text_color(connection_status_label, lv_color_hex(0x00ff88), 0); // Green
  }
  else
  {
    snprintf(combined_status, sizeof(combined_status), "[SERIAL] Connection Lost | %s", current_timestamp);
    lv_label_set_text(connection_status_label, combined_status);
    lv_obj_set_style_text_color(connection_status_label, lv_color_hex(0xff4444), 0); // Red
  }

  lvgl_lock_release();
}

void status_info_update_timestamp(uint64_t timestamp)
{
  if (timestamp_label)
  {
    time_t timestamp_sec = timestamp / 1000;
    struct tm *timeinfo = localtime(&timestamp_sec);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "Last: %H:%M:%S", timeinfo);
    lv_label_set_text(timestamp_label, time_str);
  }
}
