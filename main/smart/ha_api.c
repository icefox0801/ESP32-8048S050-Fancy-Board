/**
 * @file ha_api.c
 * @brief Home Assistant REST API Client Implementation
 *
 * This module implements HTTP client functionality for Home Assistant REST API.
 *
 * @author System Monitor Dashboard
 * @date 2025-08-14
 */

#include "ha_api.h"
#include "smart_config.h"
#include "../utils/system_debug_utils.h"
#include <esp_http_client.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_task_wdt.h>
#include <lwip/netdb.h>
#include <string.h>
#include <stdio.h>

/** HTTP User-Agent string */
#define USER_AGENT "ESP32-SystemMonitor/1.0"

/** HTTP headers template */
#define AUTH_HEADER_TEMPLATE "Bearer %s"
#define CONTENT_TYPE_JSON "application/json"

// ═══════════════════════════════════════════════════════════════════════════════
// PRIVATE VARIABLES
// ═══════════════════════════════════════════════════════════════════════════════

static bool ha_api_initialized = false;
static char auth_header[256];

// ═══════════════════════════════════════════════════════════════════════════════
// PRIVATE FUNCTION DECLARATIONS
// ═══════════════════════════════════════════════════════════════════════════════

static esp_err_t http_event_handler(esp_http_client_event_t *evt);
static esp_http_client_handle_t create_http_client(const char *url);
static esp_err_t perform_http_request(const char *url, const char *method, const char *post_data, ha_api_response_t *response);

// ═══════════════════════════════════════════════════════════════════════════════
// PRIVATE FUNCTION IMPLEMENTATIONS
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Check network connectivity to Home Assistant server
 * @return true if server is reachable, false otherwise
 */
static bool check_network_connectivity(void)
{
  debug_log_info(DEBUG_TAG_HA_API, "🌐 Checking network connectivity to HA server");

  // Check WiFi connection first
  wifi_ap_record_t ap_info;
  esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
  if (ret != ESP_OK)
  {
    debug_log_error(DEBUG_TAG_HA_API, "❌ WiFi not connected");
    return false;
  }

  debug_log_info(DEBUG_TAG_HA_API, "✅ Network connectivity OK");
  return true;
}

/**
 * @brief HTTP event handler for response data collection
 */
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
  ha_api_response_t *response = (ha_api_response_t *)evt->user_data;

  switch (evt->event_id)
  {
  case HTTP_EVENT_ERROR:
    if (response)
    {
      snprintf(response->error_message, sizeof(response->error_message), "HTTP error occurred");
      response->success = false;
    }
    break;

  case HTTP_EVENT_ON_CONNECTED:
    // Connection established, no action needed
    break;

  case HTTP_EVENT_HEADER_SENT:
    // Headers sent, no action needed
    break;

  case HTTP_EVENT_ON_HEADER:
    // Receiving headers, no action needed
    break;

  case HTTP_EVENT_ON_DATA:
    if (response && evt->data_len > 0)
    {
      if (response->response_data == NULL)
      {
        response->response_data = malloc(HA_MAX_RESPONSE_SIZE);
        response->response_len = 0;
      }

      if (response->response_data && (response->response_len + evt->data_len) < HA_MAX_RESPONSE_SIZE)
      {
        memcpy(response->response_data + response->response_len, evt->data, evt->data_len);
        response->response_len += evt->data_len;
        response->response_data[response->response_len] = '\0';
      }

      // Feed the task watchdog to prevent timeout during large responses
      esp_task_wdt_reset();
    }
    break;

  case HTTP_EVENT_ON_FINISH:
    if (response)
    {
      response->status_code = esp_http_client_get_status_code((esp_http_client_handle_t)evt->client);
      response->success = (response->status_code >= 200 && response->status_code < 300);
    }
    break;

  case HTTP_EVENT_DISCONNECTED:
    // Connection closed, no action needed
    break;

  case HTTP_EVENT_REDIRECT:
    // Redirect occurred, no action needed (client handles automatically)
    break;

  default:
    // Handle any other events silently
    break;
  }

  return ESP_OK;
}

/**
 * @brief Create and configure HTTP client
 */
static esp_http_client_handle_t create_http_client(const char *url)
{
  esp_http_client_config_t config = {
      .url = url,
      .event_handler = http_event_handler,
      .timeout_ms = HA_HTTP_TIMEOUT_MS,
      .user_agent = USER_AGENT,
      .buffer_size = HA_MAX_RESPONSE_SIZE,
      .buffer_size_tx = 512,     // Reduced TX buffer for individual requests
      .keep_alive_enable = true, // Enable keep-alive for multiple requests
      .keep_alive_idle = 5,      // Keep connection alive for 5 seconds
      .keep_alive_interval = 5,
      .keep_alive_count = 3,
  };

  return esp_http_client_init(&config);
}

/**
 * @brief Perform HTTP request with retry logic
 */
static esp_err_t perform_http_request(const char *url, const char *method, const char *post_data, ha_api_response_t *response)
{
  if (!ha_api_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  // Check network connectivity before attempting HTTP request
  if (!check_network_connectivity())
  {
    debug_log_error(DEBUG_TAG_HA_API, "❌ Network connectivity check failed, skipping HTTP request");
    if (response)
    {
      response->success = false;
      response->status_code = 0;
    }
    return ESP_ERR_NOT_FOUND;
  }

  debug_log_info(DEBUG_TAG_HA_API, "=== HTTP REQUEST START ===");
  debug_log_info_f(DEBUG_TAG_HA_API, "Method: %s", method);
  debug_log_info_f(DEBUG_TAG_HA_API, "URL: %s", url);
  if (post_data)
  {
    debug_log_info_f(DEBUG_TAG_HA_API, "POST Data: %s", post_data);
  }

  esp_err_t err = ESP_FAIL;
  int status_code = 0;

  for (int retry = 0; retry < HA_SYNC_RETRY_COUNT; retry++)
  {
    esp_http_client_handle_t client = create_http_client(url);
    if (client == NULL)
    {
      debug_log_error(DEBUG_TAG_HA_API, "Failed to create HTTP client");
      continue;
    }

    // Set headers
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_header(client, "Content-Type", CONTENT_TYPE_JSON);

    // Set method
    if (strcmp(method, "POST") == 0)
    {
      esp_http_client_set_method(client, HTTP_METHOD_POST);
      if (post_data)
      {
        esp_http_client_set_post_field(client, post_data, strlen(post_data));
      }
    }
    else
    {
      esp_http_client_set_method(client, HTTP_METHOD_GET);
    }

    // Set user data for event handler
    if (response)
    {
      memset(response, 0, sizeof(ha_api_response_t));
      esp_http_client_set_user_data(client, response);
    }

    // Perform request
    debug_log_info_f(DEBUG_TAG_HA_API, "Sending HTTP request (attempt %d/%d)...", retry + 1, HA_SYNC_RETRY_COUNT);
    err = esp_http_client_perform(client);

    // Get status code for logging
    status_code = esp_http_client_get_status_code(client);
    debug_log_info_f(DEBUG_TAG_HA_API, "HTTP Status Code: %d", status_code);

    esp_http_client_cleanup(client);

    if (err == ESP_OK)
    {
      debug_log_info_f(DEBUG_TAG_HA_API, "HTTP request successful (attempt %d)", retry + 1);
      debug_log_info(DEBUG_TAG_HA_API, "=== HTTP REQUEST SUCCESS ===");
      break;
    }
    else
    {
      debug_log_warning_f(DEBUG_TAG_HA_API, "HTTP request failed (attempt %d/%d): %s (status: %d)",
                          retry + 1, HA_SYNC_RETRY_COUNT, esp_err_to_name(err), status_code);
      if (response)
      {
        snprintf(response->error_message, sizeof(response->error_message),
                 "HTTP request failed: %s (status: %d)", esp_err_to_name(err), status_code);
      }
    }

    // Wait before retry
    if (retry < HA_SYNC_RETRY_COUNT - 1)
    {
      debug_log_info_f(DEBUG_TAG_HA_API, "Waiting %d seconds before retry...", retry + 1);
      vTaskDelay(pdMS_TO_TICKS(1000 * (retry + 1))); // Progressive backoff
    }
  }

  if (err != ESP_OK)
  {
    debug_log_error_f(DEBUG_TAG_HA_API, "=== HTTP REQUEST FAILED === (Final status: %d, Error: %s)", status_code, esp_err_to_name(err));
  }

  return err;
}

// ═══════════════════════════════════════════════════════════════════════════════
// PUBLIC FUNCTION IMPLEMENTATIONS
// ═══════════════════════════════════════════════════════════════════════════════

esp_err_t ha_api_init(void)
{
  if (ha_api_initialized)
  {
    debug_log_warning(DEBUG_TAG_HA_API, "Home Assistant API already initialized");
    return ESP_OK;
  }

  debug_log_info(DEBUG_TAG_HA_API, "Initializing Home Assistant API client...");

  // Check if constants are properly defined
  if (HA_API_TOKEN == NULL || strlen(HA_API_TOKEN) == 0)
  {
    debug_log_error(DEBUG_TAG_HA_API, "HA API Token is not defined or empty");
    return ESP_ERR_INVALID_ARG;
  }

  if (HA_SERVER_HOST_NAME == NULL || strlen(HA_SERVER_HOST_NAME) == 0)
  {
    debug_log_error(DEBUG_TAG_HA_API, "HA Server Host Name is not defined or empty");
    return ESP_ERR_INVALID_ARG;
  }

  debug_log_info_f(DEBUG_TAG_HA_API, "HA Server: %s:%d", HA_SERVER_HOST_NAME, HA_SERVER_PORT);
  debug_log_info_f(DEBUG_TAG_HA_API, "Token length: %d", strlen(HA_API_TOKEN));

  // Format authorization header with error checking
  int ret = snprintf(auth_header, sizeof(auth_header), AUTH_HEADER_TEMPLATE, HA_API_TOKEN);
  if (ret < 0 || ret >= sizeof(auth_header))
  {
    debug_log_error_f(DEBUG_TAG_HA_API, "Failed to format authorization header: %d", ret);
    return ESP_ERR_NO_MEM;
  }

  debug_log_info(DEBUG_TAG_HA_API, "Authorization header formatted successfully");

  ha_api_initialized = true;

  debug_log_info_f(DEBUG_TAG_HA_API, "Home Assistant API client initialized (Server: %s:%d)",
                   HA_SERVER_HOST_NAME, HA_SERVER_PORT);
  debug_log_info_f(DEBUG_TAG_HA_API, "Base URL: %s", HA_API_BASE_URL);

  return ESP_OK;
}

esp_err_t ha_api_deinit(void)
{
  if (!ha_api_initialized)
  {
    return ESP_OK;
  }

  debug_log_info(DEBUG_TAG_HA_API, "Deinitializing Home Assistant API client");

  ha_api_initialized = false;
  memset(auth_header, 0, sizeof(auth_header));

  return ESP_OK;
}

esp_err_t ha_api_get_entity_state(const char *entity_id, ha_entity_state_t *state)
{
  if (!entity_id || !state)
  {
    return ESP_ERR_INVALID_ARG;
  }

  char url[256];
  snprintf(url, sizeof(url), "%s/%s", HA_API_STATES_URL, entity_id);

  ha_api_response_t response;
  esp_err_t err = perform_http_request(url, "GET", NULL, &response);

  if (err == ESP_OK && response.success)
  {
    err = ha_api_parse_entity_state(response.response_data, state);
  }

  ha_api_free_response(&response);
  return err;
}

esp_err_t ha_api_get_multiple_entity_states(const char **entity_ids, int entity_count, ha_entity_state_t *states)
{
  if (!entity_ids || !states || entity_count <= 0)
  {
    return ESP_ERR_INVALID_ARG;
  }

  debug_log_info_f(DEBUG_TAG_HA_API, "Fetching %d entity states individually", entity_count);

  // Clear all states first
  memset(states, 0, sizeof(ha_entity_state_t) * entity_count);

  esp_err_t overall_result = ESP_OK;
  int success_count = 0;

  // Fetch each entity state individually to avoid watchdog timeout
  for (int i = 0; i < entity_count; i++)
  {
    debug_log_info_f(DEBUG_TAG_HA_API, "Fetching entity %d/%d: %s", i + 1, entity_count, entity_ids[i]);

    esp_err_t result = ha_api_get_entity_state(entity_ids[i], &states[i]);
    if (result == ESP_OK)
    {
      success_count++;
      debug_log_info_f(DEBUG_TAG_HA_API, "✅ Entity %s state: %s", entity_ids[i], states[i].state);
    }
    else
    {
      debug_log_warning_f(DEBUG_TAG_HA_API, "❌ Failed to fetch entity %s: %s", entity_ids[i], esp_err_to_name(result));
      overall_result = result; // Keep track of last error
    }

    // Add small delay between requests to prevent overwhelming the server
    vTaskDelay(pdMS_TO_TICKS(100));
  }

  if (success_count == entity_count)
  {
    debug_log_info_f(DEBUG_TAG_HA_API, "Successfully fetched all %d entity states", entity_count);
    return ESP_OK;
  }
  else if (success_count > 0)
  {
    debug_log_warning_f(DEBUG_TAG_HA_API, "Fetched %d/%d entity states", success_count, entity_count);
    return ESP_ERR_NOT_FOUND;
  }
  else
  {
    debug_log_error(DEBUG_TAG_HA_API, "Failed to fetch any entity states");
    return overall_result;
  }
}

esp_err_t ha_api_call_service(const ha_service_call_t *service_call, ha_api_response_t *response)
{
  if (!service_call)
  {
    return ESP_ERR_INVALID_ARG;
  }

  char url[256];
  snprintf(url, sizeof(url), "%s/%s/%s", HA_API_SERVICES_URL, service_call->domain, service_call->service);

  // Create service data JSON
  cJSON *json = cJSON_CreateObject();
  cJSON *entity_id = cJSON_CreateString(service_call->entity_id);
  cJSON_AddItemToObject(json, "entity_id", entity_id);

  // Add additional service data if provided
  if (service_call->service_data)
  {
    cJSON *data_item = NULL;
    cJSON_ArrayForEach(data_item, service_call->service_data)
    {
      cJSON_AddItemToObject(json, data_item->string, cJSON_Duplicate(data_item, true));
    }
  }

  char *json_string = cJSON_Print(json);

  debug_log_info(DEBUG_TAG_HA_API, "=== SERVICE CALL START ===");
  debug_log_info_f(DEBUG_TAG_HA_API, "Service: %s.%s", service_call->domain, service_call->service);
  debug_log_info_f(DEBUG_TAG_HA_API, "Entity: %s", service_call->entity_id);
  debug_log_info_f(DEBUG_TAG_HA_API, "Service data: %s", json_string);

  ha_api_response_t local_response;
  ha_api_response_t *resp = response ? response : &local_response;

  esp_err_t err = perform_http_request(url, "POST", json_string, resp);

  if (err == ESP_OK && resp->success)
  {
    debug_log_info(DEBUG_TAG_HA_API, "=== SERVICE CALL SUCCESS ===");
    debug_log_info_f(DEBUG_TAG_HA_API, "Service %s.%s executed successfully for %s",
                     service_call->domain, service_call->service, service_call->entity_id);
  }
  else
  {
    debug_log_error(DEBUG_TAG_HA_API, "=== SERVICE CALL FAILED ===");
    debug_log_error_f(DEBUG_TAG_HA_API, "Service %s.%s failed for %s: %s",
                      service_call->domain, service_call->service, service_call->entity_id,
                      resp->error_message[0] ? resp->error_message : "Unknown error");
  }

  // Cleanup
  free(json_string);
  cJSON_Delete(json);

  if (!response)
  {
    ha_api_free_response(&local_response);
  }

  return err;
}

esp_err_t ha_api_turn_on_switch(const char *entity_id)
{
  debug_log_info_f(DEBUG_TAG_HA_API, ">>> TURN ON SWITCH: %s", entity_id);

  ha_service_call_t service_call = {
      .domain = "switch",
      .service = "turn_on",
      .service_data = NULL};
  strncpy(service_call.entity_id, entity_id, sizeof(service_call.entity_id) - 1);

  ha_api_response_t response;
  esp_err_t result = ha_api_call_service(&service_call, &response);

  if (result == ESP_OK)
  {
    debug_log_info_f(DEBUG_TAG_HA_API, "<<< TURN ON SUCCESS: %s", entity_id);
  }
  else
  {
    debug_log_error_f(DEBUG_TAG_HA_API, "<<< TURN ON FAILED: %s (Error: %s)", entity_id, esp_err_to_name(result));
  }

  ha_api_free_response(&response);
  return result;
}

esp_err_t ha_api_turn_off_switch(const char *entity_id)
{
  debug_log_info_f(DEBUG_TAG_HA_API, ">>> TURN OFF SWITCH: %s", entity_id);

  ha_service_call_t service_call = {
      .domain = "switch",
      .service = "turn_off",
      .service_data = NULL};
  strncpy(service_call.entity_id, entity_id, sizeof(service_call.entity_id) - 1);

  ha_api_response_t response;
  esp_err_t result = ha_api_call_service(&service_call, &response);

  if (result == ESP_OK)
  {
    debug_log_info_f(DEBUG_TAG_HA_API, "<<< TURN OFF SUCCESS: %s", entity_id);
  }
  else
  {
    debug_log_error_f(DEBUG_TAG_HA_API, "<<< TURN OFF FAILED: %s (Error: %s)", entity_id, esp_err_to_name(result));
  }

  ha_api_free_response(&response);
  return result;
}

esp_err_t ha_api_parse_entity_state(const char *json_str, ha_entity_state_t *state)
{
  if (!json_str || !state)
  {
    return ESP_ERR_INVALID_ARG;
  }

  memset(state, 0, sizeof(ha_entity_state_t));

  cJSON *json = cJSON_Parse(json_str);
  if (json == NULL)
  {
    debug_log_error(DEBUG_TAG_HA_API, "Failed to parse JSON response");
    return ESP_ERR_INVALID_RESPONSE;
  }

  // Parse entity ID
  cJSON *entity_id = cJSON_GetObjectItem(json, "entity_id");
  if (cJSON_IsString(entity_id))
  {
    strncpy(state->entity_id, entity_id->valuestring, sizeof(state->entity_id) - 1);
  }

  // Parse state
  cJSON *state_item = cJSON_GetObjectItem(json, "state");
  if (cJSON_IsString(state_item))
  {
    strncpy(state->state, state_item->valuestring, sizeof(state->state) - 1);
  }

  // Parse friendly name from attributes
  cJSON *attributes = cJSON_GetObjectItem(json, "attributes");
  if (cJSON_IsObject(attributes))
  {
    cJSON *friendly_name = cJSON_GetObjectItem(attributes, "friendly_name");
    if (cJSON_IsString(friendly_name))
    {
      strncpy(state->friendly_name, friendly_name->valuestring, sizeof(state->friendly_name) - 1);
    }
  }

  // Parse last_updated timestamp
  cJSON *last_updated = cJSON_GetObjectItem(json, "last_updated");
  if (cJSON_IsString(last_updated))
  {
    // Convert ISO timestamp to Unix time if needed
    // For now, just set to current time
    state->last_updated = time(NULL);
  }

  cJSON_Delete(json);
  return ESP_OK;
}

void ha_api_free_response(ha_api_response_t *response)
{
  if (response && response->response_data)
  {
    free(response->response_data);
    response->response_data = NULL;
    response->response_len = 0;
  }
}
