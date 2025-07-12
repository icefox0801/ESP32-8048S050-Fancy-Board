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

  ESP_LOGI(TAG, "=== HTTP REQUEST START ===");
  ESP_LOGI(TAG, "Method: %s", method);
  ESP_LOGI(TAG, "URL: %s", url);
  if (post_data)
  {
    ESP_LOGI(TAG, "POST Data: %s", post_data);
  }

  esp_err_t err = ESP_FAIL;
  int status_code = 0;

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
    ESP_LOGI(TAG, "Sending HTTP request (attempt %d/%d)...", retry + 1, HA_SYNC_RETRY_COUNT);
    err = esp_http_client_perform(client);

    // Get status code for logging
    status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP Status Code: %d", status_code);

    esp_http_client_cleanup(client);

    if (err == ESP_OK)
    {
      ESP_LOGI(TAG, "HTTP request successful (attempt %d)", retry + 1);
      ESP_LOGI(TAG, "=== HTTP REQUEST SUCCESS ===");
      break;
    }
    else
    {
      ESP_LOGW(TAG, "HTTP request failed (attempt %d/%d): %s (status: %d)",
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
      ESP_LOGI(TAG, "Waiting %d seconds before retry...", retry + 1);
      vTaskDelay(pdMS_TO_TICKS(1000 * (retry + 1))); // Progressive backoff
    }
  }

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "=== HTTP REQUEST FAILED === (Final status: %d, Error: %s)", status_code, esp_err_to_name(err));
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

  // Check if constants are properly defined
  if (HA_API_TOKEN == NULL || strlen(HA_API_TOKEN) == 0)
  {
    ESP_LOGE(TAG, "HA API Token is not defined or empty");
    return ESP_ERR_INVALID_ARG;
  }

  if (HA_SERVER_HOST_NAME == NULL || strlen(HA_SERVER_HOST_NAME) == 0)
  {
    ESP_LOGE(TAG, "HA Server Host Name is not defined or empty");
    return ESP_ERR_INVALID_ARG;
  }

  ESP_LOGI(TAG, "HA Server: %s:%d", HA_SERVER_HOST_NAME, HA_SERVER_PORT);
  ESP_LOGI(TAG, "Token length: %d", strlen(HA_API_TOKEN));

  // Format authorization header with error checking
  int ret = snprintf(auth_header, sizeof(auth_header), AUTH_HEADER_TEMPLATE, HA_API_TOKEN);
  if (ret < 0 || ret >= sizeof(auth_header))
  {
    ESP_LOGE(TAG, "Failed to format authorization header: %d", ret);
    return ESP_ERR_NO_MEM;
  }

  ESP_LOGI(TAG, "Authorization header formatted successfully");

  ha_api_initialized = true;

  ESP_LOGI(TAG, "Home Assistant API client initialized (Server: %s:%d)",
           HA_SERVER_HOST_NAME, HA_SERVER_PORT);
  ESP_LOGI(TAG, "Base URL: %s", HA_API_BASE_URL);

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

esp_err_t ha_api_get_multiple_entity_states(const char **entity_ids, int entity_count, ha_entity_state_t *states)
{
  if (!entity_ids || !states || entity_count <= 0)
  {
    return ESP_ERR_INVALID_ARG;
  }

  ESP_LOGI(TAG, "Fetching %d entity states in bulk", entity_count);

  // Use the bulk states API endpoint
  char url[128];
  snprintf(url, sizeof(url), "%s", HA_API_STATES_URL);

  ha_api_response_t response;
  esp_err_t err = perform_http_request(url, "GET", NULL, &response);

  if (err == ESP_OK && response.success)
  {
    // Parse JSON response to extract our entities
    cJSON *json = cJSON_Parse(response.response_data);
    if (json)
    {
      // Clear all states first
      memset(states, 0, sizeof(ha_entity_state_t) * entity_count);

      int found_count = 0;
      int processed_count = 0;
      const int MAX_ENTITIES_TO_PROCESS = 100; // Safety limit to prevent memory issues

      // Iterate through the response array to find our entities
      cJSON *entity_json = NULL;
      cJSON_ArrayForEach(entity_json, json)
      {
        // Safety check to prevent processing too many entities
        if (++processed_count > MAX_ENTITIES_TO_PROCESS)
        {
          ESP_LOGW(TAG, "Reached maximum entity processing limit (%d), stopping search", MAX_ENTITIES_TO_PROCESS);
          break;
        }
        cJSON *entity_id_json = cJSON_GetObjectItem(entity_json, "entity_id");
        if (cJSON_IsString(entity_id_json))
        {
          const char *entity_id = entity_id_json->valuestring;

          // Check if this entity is in our requested list
          for (int i = 0; i < entity_count; i++)
          {
            if (strcmp(entity_id, entity_ids[i]) == 0)
            {
              // Parse entity state directly from JSON object to avoid memory allocation
              cJSON *state_json = cJSON_GetObjectItem(entity_json, "state");
              if (cJSON_IsString(state_json))
              {
                // Clear the state structure
                memset(&states[i], 0, sizeof(ha_entity_state_t));

                // Copy entity ID and state
                strncpy(states[i].entity_id, entity_id, HA_MAX_ENTITY_ID_LEN - 1);
                strncpy(states[i].state, state_json->valuestring, HA_MAX_STATE_LEN - 1);
                states[i].entity_id[HA_MAX_ENTITY_ID_LEN - 1] = '\0';
                states[i].state[HA_MAX_STATE_LEN - 1] = '\0';

                found_count++;
                ESP_LOGD(TAG, "Found state for %s: %s", entity_id, states[i].state);
              }
              else
              {
                ESP_LOGW(TAG, "Entity %s has no valid state", entity_id);
              }
              break;
            }
          }

          // Early exit if we found all entities
          if (found_count >= entity_count)
          {
            ESP_LOGI(TAG, "Found all %d entities, stopping search early", entity_count);
            break;
          }
        }
      }

      cJSON_Delete(json);

      if (found_count == entity_count)
      {
        ESP_LOGI(TAG, "Successfully fetched all %d entity states", entity_count);
        err = ESP_OK;
      }
      else if (found_count > 0)
      {
        ESP_LOGW(TAG, "Found only %d/%d entity states", found_count, entity_count);
        err = ESP_ERR_NOT_FOUND;
      }
      else
      {
        ESP_LOGE(TAG, "No matching entities found");
        err = ESP_ERR_NOT_FOUND;
      }
    }
    else
    {
      ESP_LOGE(TAG, "Failed to parse bulk states response");
      err = ESP_ERR_INVALID_RESPONSE;
    }
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

  ESP_LOGI(TAG, "=== SERVICE CALL START ===");
  ESP_LOGI(TAG, "Service: %s.%s", service_call->domain, service_call->service);
  ESP_LOGI(TAG, "Entity: %s", service_call->entity_id);
  ESP_LOGI(TAG, "Service data: %s", json_string);

  ha_api_response_t local_response;
  ha_api_response_t *resp = response ? response : &local_response;

  esp_err_t err = perform_http_request(url, "POST", json_string, resp);

  if (err == ESP_OK && resp->success)
  {
    ESP_LOGI(TAG, "=== SERVICE CALL SUCCESS ===");
    ESP_LOGI(TAG, "Service %s.%s executed successfully for %s",
             service_call->domain, service_call->service, service_call->entity_id);
  }
  else
  {
    ESP_LOGE(TAG, "=== SERVICE CALL FAILED ===");
    ESP_LOGE(TAG, "Service %s.%s failed for %s: %s",
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
  ESP_LOGI(TAG, ">>> TURN ON SWITCH: %s", entity_id);

  ha_service_call_t service_call = {
      .domain = "switch",
      .service = "turn_on",
      .service_data = NULL};
  strncpy(service_call.entity_id, entity_id, sizeof(service_call.entity_id) - 1);

  ha_api_response_t response;
  esp_err_t result = ha_api_call_service(&service_call, &response);

  if (result == ESP_OK)
  {
    ESP_LOGI(TAG, "<<< TURN ON SUCCESS: %s", entity_id);
  }
  else
  {
    ESP_LOGE(TAG, "<<< TURN ON FAILED: %s (Error: %s)", entity_id, esp_err_to_name(result));
  }

  ha_api_free_response(&response);
  return result;
}

esp_err_t ha_api_turn_off_switch(const char *entity_id)
{
  ESP_LOGI(TAG, ">>> TURN OFF SWITCH: %s", entity_id);

  ha_service_call_t service_call = {
      .domain = "switch",
      .service = "turn_off",
      .service_data = NULL};
  strncpy(service_call.entity_id, entity_id, sizeof(service_call.entity_id) - 1);

  ha_api_response_t response;
  esp_err_t result = ha_api_call_service(&service_call, &response);

  if (result == ESP_OK)
  {
    ESP_LOGI(TAG, "<<< TURN OFF SUCCESS: %s", entity_id);
  }
  else
  {
    ESP_LOGE(TAG, "<<< TURN OFF FAILED: %s (Error: %s)", entity_id, esp_err_to_name(result));
  }

  ha_api_free_response(&response);
  return result;
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
