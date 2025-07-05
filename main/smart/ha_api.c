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
#include <esp_log.h>
#include <esp_http_client.h>
#include <string.h>
#include <stdio.h>

// ═══════════════════════════════════════════════════════════════════════════════
// CONSTANTS AND CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════

static const char *TAG = "HA_API";

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
 * @brief HTTP event handler for response data collection
 */
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
  ha_api_response_t *response = (ha_api_response_t *)evt->user_data;

  switch (evt->event_id)
  {
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
    }
    break;

  case HTTP_EVENT_ON_FINISH:
    if (response)
    {
      response->status_code = esp_http_client_get_status_code((esp_http_client_handle_t)evt->client);
      response->success = (response->status_code >= 200 && response->status_code < 300);
    }
    break;

  case HTTP_EVENT_ERROR:
    if (response)
    {
      snprintf(response->error_message, sizeof(response->error_message), "HTTP error occurred");
      response->success = false;
    }
    break;

  default:
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
      .buffer_size_tx = 1024,
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

  esp_err_t err = ESP_FAIL;

  for (int retry = 0; retry < HA_SYNC_RETRY_COUNT; retry++)
  {
    esp_http_client_handle_t client = create_http_client(url);
    if (client == NULL)
    {
      ESP_LOGE(TAG, "Failed to create HTTP client");
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
    err = esp_http_client_perform(client);

    esp_http_client_cleanup(client);

    if (err == ESP_OK)
    {
      ESP_LOGD(TAG, "HTTP request successful (attempt %d)", retry + 1);
      break;
    }
    else
    {
      ESP_LOGW(TAG, "HTTP request failed (attempt %d/%d): %s",
               retry + 1, HA_SYNC_RETRY_COUNT, esp_err_to_name(err));
      if (response)
      {
        snprintf(response->error_message, sizeof(response->error_message),
                 "HTTP request failed: %s", esp_err_to_name(err));
      }
    }

    // Wait before retry
    if (retry < HA_SYNC_RETRY_COUNT - 1)
    {
      vTaskDelay(pdMS_TO_TICKS(1000 * (retry + 1))); // Progressive backoff
    }
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
    ESP_LOGW(TAG, "Home Assistant API already initialized");
    return ESP_OK;
  }

  ESP_LOGI(TAG, "Initializing Home Assistant API client...");

  // Format authorization header
  snprintf(auth_header, sizeof(auth_header), AUTH_HEADER_TEMPLATE, HA_API_TOKEN);

  ha_api_initialized = true;

  ESP_LOGI(TAG, "Home Assistant API client initialized (Server: %s:%d)",
           HA_SERVER_IP, HA_SERVER_PORT);

  return ESP_OK;
}

esp_err_t ha_api_deinit(void)
{
  if (!ha_api_initialized)
  {
    return ESP_OK;
  }

  ESP_LOGI(TAG, "Deinitializing Home Assistant API client");

  ha_api_initialized = false;
  memset(auth_header, 0, sizeof(auth_header));

  return ESP_OK;
}

esp_err_t ha_api_test_connection(void)
{
  ESP_LOGI(TAG, "Testing connection to Home Assistant...");

  ha_api_response_t response;
  esp_err_t err = perform_http_request(HA_API_BASE_URL, "GET", NULL, &response);

  if (err == ESP_OK && response.success)
  {
    ESP_LOGI(TAG, "Connection test successful (Status: %d)", response.status_code);
  }
  else
  {
    ESP_LOGE(TAG, "Connection test failed: %s", response.error_message);
  }

  ha_api_free_response(&response);
  return err;
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

  ESP_LOGD(TAG, "Calling service %s.%s for entity %s",
           service_call->domain, service_call->service, service_call->entity_id);
  ESP_LOGD(TAG, "Service data: %s", json_string);

  ha_api_response_t local_response;
  ha_api_response_t *resp = response ? response : &local_response;

  esp_err_t err = perform_http_request(url, "POST", json_string, resp);

  if (err == ESP_OK && resp->success)
  {
    ESP_LOGI(TAG, "Service call successful");
  }
  else
  {
    ESP_LOGE(TAG, "Service call failed: %s", resp->error_message);
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

esp_err_t ha_api_toggle_switch(const char *entity_id)
{
  ha_service_call_t service_call = {
      .domain = "switch",
      .service = "toggle",
      .service_data = NULL};
  strncpy(service_call.entity_id, entity_id, sizeof(service_call.entity_id) - 1);

  return ha_api_call_service(&service_call, NULL);
}

esp_err_t ha_api_turn_on_switch(const char *entity_id)
{
  ha_service_call_t service_call = {
      .domain = "switch",
      .service = "turn_on",
      .service_data = NULL};
  strncpy(service_call.entity_id, entity_id, sizeof(service_call.entity_id) - 1);

  return ha_api_call_service(&service_call, NULL);
}

esp_err_t ha_api_turn_off_switch(const char *entity_id)
{
  ha_service_call_t service_call = {
      .domain = "switch",
      .service = "turn_off",
      .service_data = NULL};
  strncpy(service_call.entity_id, entity_id, sizeof(service_call.entity_id) - 1);

  return ha_api_call_service(&service_call, NULL);
}

esp_err_t ha_api_get_sensor_value(const char *entity_id, float *value)
{
  if (!entity_id || !value)
  {
    return ESP_ERR_INVALID_ARG;
  }

  ha_entity_state_t state;
  esp_err_t err = ha_api_get_entity_state(entity_id, &state);

  if (err == ESP_OK)
  {
    *value = atof(state.state);
  }

  return err;
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
    ESP_LOGE(TAG, "Failed to parse JSON response");
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

  // Parse attributes
  cJSON *attributes = cJSON_GetObjectItem(json, "attributes");
  if (cJSON_IsObject(attributes))
  {
    cJSON *friendly_name = cJSON_GetObjectItem(attributes, "friendly_name");
    if (cJSON_IsString(friendly_name))
    {
      strncpy(state->friendly_name, friendly_name->valuestring, sizeof(state->friendly_name) - 1);
    }

    // Parse other attributes
    cJSON *attribute = NULL;
    state->attribute_count = 0;
    cJSON_ArrayForEach(attribute, attributes)
    {
      if (state->attribute_count < HA_MAX_ATTRIBUTES)
      {
        strncpy(state->attributes[state->attribute_count].key, attribute->string, 31);
        if (cJSON_IsString(attribute))
        {
          strncpy(state->attributes[state->attribute_count].value, attribute->valuestring, HA_MAX_ATTRIBUTE_LEN - 1);
        }
        else
        {
          char *attr_str = cJSON_Print(attribute);
          strncpy(state->attributes[state->attribute_count].value, attr_str, HA_MAX_ATTRIBUTE_LEN - 1);
          free(attr_str);
        }
        state->attribute_count++;
      }
    }
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

const char *ha_api_get_error_string(esp_err_t error_code)
{
  switch (error_code)
  {
  case ESP_OK:
    return "Success";
  case ESP_ERR_INVALID_ARG:
    return "Invalid argument";
  case ESP_ERR_INVALID_STATE:
    return "API not initialized";
  case ESP_ERR_NO_MEM:
    return "Out of memory";
  case ESP_ERR_TIMEOUT:
    return "Request timeout";
  case ESP_ERR_INVALID_RESPONSE:
    return "Invalid response format";
  default:
    return esp_err_to_name(error_code);
  }
}
