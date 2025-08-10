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
#include "ha_status.h"
#include "smart_config.h"
#include "entity_states_parser.h"
#include "system_debug_utils.h"
#include <esp_http_client.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_task_wdt.h>
#include <esp_timer.h>
#include <cJSON.h>
#include <lwip/netdb.h>
#include <string.h>
#include <stdio.h>

/** HTTP User-Agent string */
#define USER_AGENT "ESP32-SystemMonitor/1.0"

/** HTTP headers template */
#define AUTH_HEADER_TEMPLATE "Bearer %s"
#define CONTENT_TYPE_JSON "application/json"

// =======================================================================
// PRIVATE VARIABLES
// =======================================================================

static bool ha_api_initialized = false;
static char auth_header[256];
static esp_http_client_handle_t persistent_client = NULL;
static char current_base_url[256] = {0};
static bool task_watchdog_subscribed = false;

// =======================================================================
// PRIVATE FUNCTION DECLARATIONS
// =======================================================================

static esp_err_t http_event_handler(esp_http_client_event_t *evt);
static esp_http_client_handle_t create_http_client(const char *url);
static esp_http_client_handle_t get_persistent_client(const char *url);
static void cleanup_persistent_client(void);
static esp_err_t perform_http_request(const char *url, const char *method, const char *post_data, ha_api_response_t *response);

// =======================================================================
// PRIVATE FUNCTION IMPLEMENTATIONS
// =======================================================================

/**
 * @brief Check network connectivity to Home Assistant server
 * @return true if server is reachable, false otherwise
 */
static bool check_network_connectivity(void)
{
  // Check WiFi connection first
  wifi_ap_record_t ap_info;
  esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
  if (ret != ESP_OK)
  {
    debug_log_error(DEBUG_TAG_HA_API, "WiFi not connected");
    return false;
  }

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
    debug_log_error(DEBUG_TAG_HA_API, "HTTP_EVENT_ERROR occurred");
    if (response)
    {
      snprintf(response->error_message, sizeof(response->error_message), "HTTP error occurred");
      response->success = false;
    }
    break;

  case HTTP_EVENT_ON_CONNECTED:
  case HTTP_EVENT_HEADER_SENT:
  case HTTP_EVENT_ON_HEADER:
    break;

  case HTTP_EVENT_ON_DATA:
    if (response && evt->data_len > 0)
    {
      if (response->response_data == NULL)
      {
        response->response_data = malloc(HA_MAX_RESPONSE_SIZE);
        if (response->response_data == NULL)
        {
          debug_log_error(DEBUG_TAG_HA_API, "Failed to allocate response buffer");
          return ESP_FAIL;
        }
        response->response_len = 0;
        response->response_data[0] = '\0'; // Initialize with null terminator
      }

      if (response->response_data && (response->response_len + evt->data_len) <= (HA_MAX_RESPONSE_SIZE - 1))
      {
        memcpy(response->response_data + response->response_len, evt->data, evt->data_len);
        response->response_len += evt->data_len;
        response->response_data[response->response_len] = '\0';

        // Log progress for large responses (every 10KB)
        if (response->response_len > 0 && (response->response_len % 10240) == 0)
        {
          // Progress logging skipped for performance
        }
      }
      else if (response->response_data)
      {
        // Log if we're hitting buffer size limits
        debug_log_warning_f(DEBUG_TAG_HA_API, "Response buffer limit reached: %zu + %d >= %d bytes",
                            response->response_len, evt->data_len, HA_MAX_RESPONSE_SIZE - 1);
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
    break;

  case HTTP_EVENT_REDIRECT:
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
      .timeout_ms = HA_HTTP_TIMEOUT_MS, // 8 seconds total timeout
      .user_agent = USER_AGENT,
      .buffer_size = HA_MAX_RESPONSE_SIZE,
      .buffer_size_tx = 2048,              // Increased TX buffer for better throughput
      .keep_alive_enable = true,           // Enable keep-alive for multiple requests
      .keep_alive_idle = 15,               // Keep connection alive for 15 seconds
      .keep_alive_interval = 5,            // Send keep-alive every 5 seconds
      .keep_alive_count = 3,               // Send up to 3 keep-alive probes
      .disable_auto_redirect = false,      // Enable redirects
      .max_redirection_count = 3,          // Allow up to 3 redirects
      .max_authorization_retries = 1,      // Limit auth retries
      .use_global_ca_store = false,        // Skip SSL verification for faster connections
      .skip_cert_common_name_check = true, // Skip SSL cert name check
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client)
  {
    // Client created successfully
  }
  else
  {
    debug_log_error(DEBUG_TAG_HA_API, "Failed to create HTTP client");
  }

  return client;
}

/**
 * @brief Get or create persistent HTTP client
 */
static esp_http_client_handle_t get_persistent_client(const char *url)
{
  // Extract base URL (scheme + host + port)
  char base_url[256] = {0};
  const char *path_start = strstr(url, "://");
  if (path_start)
  {
    path_start += 3; // Skip "://"
    const char *path_end = strchr(path_start, '/');
    if (path_end)
    {
      size_t base_len = path_end - url;
      if (base_len < sizeof(base_url))
      {
        strncpy(base_url, url, base_len);
        base_url[base_len] = '\0';
      }
    }
    else
    {
      strncpy(base_url, url, sizeof(base_url) - 1);
    }
  }

  // Check if we need to create a new client or can reuse existing one
  if (persistent_client == NULL || strcmp(current_base_url, base_url) != 0)
  {
    // Clean up existing client if base URL changed
    if (persistent_client != NULL)
    {
      // Base URL changed, cleaning up old client
      cleanup_persistent_client();
    }

    // Creating new persistent HTTP client
    persistent_client = create_http_client(base_url);
    if (persistent_client)
    {
      strncpy(current_base_url, base_url, sizeof(current_base_url) - 1);
      current_base_url[sizeof(current_base_url) - 1] = '\0';
    }
  }

  return persistent_client;
}

/**
 * @brief Clean up persistent HTTP client
 */
static void cleanup_persistent_client(void)
{
  if (persistent_client != NULL)
  {
    esp_http_client_cleanup(persistent_client);
    persistent_client = NULL;
    current_base_url[0] = '\0';
  }
}

/**
 * @brief Perform HTTP request with retry logic
 */
static esp_err_t perform_http_request(const char *url, const char *method, const char *post_data, ha_api_response_t *response)
{
  if (post_data)
  {
    // Large POST data logging skipped for performance
  }

  // Parameter validation
  if (!ha_api_initialized)
  {
    debug_log_error(DEBUG_TAG_HA_API, "HA API not initialized");
    return ESP_ERR_INVALID_STATE;
  }

  // Notify that we're starting a request (syncing)
  ha_status_change(HA_STATUS_SYNCING);

  // Check network connectivity before attempting HTTP request
  if (!check_network_connectivity())
  {
    debug_log_error(DEBUG_TAG_HA_API, "Network connectivity check failed, skipping HTTP request");
    ha_status_change(HA_STATUS_SYNC_FAILED);
    if (response)
    {
      response->success = false;
      response->status_code = 0;
    }
    return ESP_ERR_NOT_FOUND;
  }

  esp_err_t err = ESP_FAIL;
  int status_code = 0;

  // Subscribe to watchdog only once per task lifecycle
  if (!task_watchdog_subscribed)
  {
    esp_err_t wdt_err = esp_task_wdt_add(xTaskGetCurrentTaskHandle());
    if (wdt_err == ESP_OK)
    {
      task_watchdog_subscribed = true;
    }
    else if (wdt_err == ESP_ERR_INVALID_ARG)
    {
      // Task already subscribed (probably by calling sync task) - this is OK
      debug_log_info(DEBUG_TAG_HA_API, "Task already subscribed to watchdog by caller");
      task_watchdog_subscribed = true; // Mark as subscribed so we can reset it
    }
    else
    {
      debug_log_warning_f(DEBUG_TAG_HA_API, "Failed to subscribe to watchdog: %s", esp_err_to_name(wdt_err));
    }
  }

  for (int retry = 0; retry < HA_SYNC_RETRY_COUNT; retry++)
  {
    // Feed watchdog before each retry
    if (task_watchdog_subscribed)
    {
      esp_task_wdt_reset();
    }

    esp_http_client_handle_t client;
    bool use_fresh_client = (strcmp(method, "POST") == 0);

    if (use_fresh_client)
    {
      client = create_http_client(url);
      if (client == NULL)
      {
        debug_log_error(DEBUG_TAG_HA_API, "Failed to create fresh HTTP client");
        vTaskDelay(pdMS_TO_TICKS(1000));
        continue;
      }
    }
    else
    {
      client = get_persistent_client(url);
      if (client == NULL)
      {
        debug_log_error(DEBUG_TAG_HA_API, "Failed to get HTTP client");
        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait before retry
        continue;
      }
    }

    // Set URL for this specific request (in case of persistent client)
    esp_http_client_set_url(client, url);

    // Set headers
    esp_http_client_set_header(client, "Authorization", auth_header);

    if (strcmp(method, "POST") == 0)
    {
      esp_http_client_set_header(client, "Content-Type", CONTENT_TYPE_JSON);
    }

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

    // Perform request with timeout tracking
    int64_t request_start_time = esp_timer_get_time();

    // Perform the HTTP request
    err = esp_http_client_perform(client);

    // Record completion time
    int64_t request_end_time = esp_timer_get_time();
    int64_t request_duration = (request_end_time - request_start_time) / 1000; // Convert to milliseconds

    // Log timeout-specific information
    if (err == ESP_ERR_TIMEOUT)
    {
      debug_log_error_f(DEBUG_TAG_HA_API, "HTTP request timed out after %lld ms (timeout limit: %d ms)",
                        request_duration, HA_HTTP_TIMEOUT_MS);
    }
    else if (request_duration > (HA_HTTP_TIMEOUT_MS / 2))
    {
      debug_log_warning_f(DEBUG_TAG_HA_API, "Slow HTTP request: %lld ms (more than half timeout)", request_duration);
    }

    // Reset watchdog immediately after HTTP operation to prevent timeout
    if (task_watchdog_subscribed)
    {
      esp_task_wdt_reset();
    }

    // Get status code for logging
    status_code = esp_http_client_get_status_code(client);

    // Handle client cleanup/connection management
    if (use_fresh_client)
    {
      esp_http_client_cleanup(client);
    }
    else
    {
      // For persistent client, DON'T close the connection - keep it alive for reuse
      // esp_http_client_close(client); // REMOVED - this was breaking keep-alive
    }

    if (err == ESP_OK)
    {
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
      // Notify retry status
      {
        ha_status_change(HA_STATUS_SYNCING);
      }

      vTaskDelay(pdMS_TO_TICKS(500)); // Fixed 500ms delay for faster retries
    }
  }

  if (err != ESP_OK)
  {
    debug_log_error_f(DEBUG_TAG_HA_API, "HTTP request failed (Final status: %d, Error: %s)", status_code, esp_err_to_name(err));
    ha_status_change(HA_STATUS_SYNC_FAILED);
  }

  return err;
}

// =======================================================================
// PUBLIC FUNCTION IMPLEMENTATIONS
// =======================================================================

esp_err_t ha_api_init(void)
{
  if (ha_api_initialized)
  {
    debug_log_warning(DEBUG_TAG_HA_API, "Home Assistant API already initialized");
    return ESP_OK;
  }

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

  // Format authorization header with error checking
  int ret = snprintf(auth_header, sizeof(auth_header), AUTH_HEADER_TEMPLATE, HA_API_TOKEN);
  if (ret < 0 || ret >= sizeof(auth_header))
  {
    debug_log_error_f(DEBUG_TAG_HA_API, "Failed to format authorization header: %d", ret);
    return ESP_ERR_NO_MEM;
  }

  // Initialize async entity states parser
  esp_err_t parser_err = entity_states_parser_init();
  if (parser_err != ESP_OK)
  {
    debug_log_error_f(DEBUG_TAG_HA_API, "Failed to initialize entity states parser: %s", esp_err_to_name(parser_err));
    return parser_err;
  }

  ha_api_initialized = true;

  // Notify status callback about initialization
  ha_status_change(HA_STATUS_READY);

  return ESP_OK;
}

esp_err_t ha_api_deinit(void)
{
  if (!ha_api_initialized)
  {
    return ESP_OK;
  }

  // Deinitialize async entity states parser
  entity_states_parser_deinit();

  // Clean up persistent HTTP client
  cleanup_persistent_client();

  // Clean up watchdog subscription if active
  if (task_watchdog_subscribed)
  {
    esp_err_t wdt_err = esp_task_wdt_delete(xTaskGetCurrentTaskHandle());
    if (wdt_err == ESP_OK)
    {
      task_watchdog_subscribed = false;
      // Task unsubscribed from watchdog
    }
    else
    {
      debug_log_warning_f(DEBUG_TAG_HA_API, "Failed to unsubscribe from watchdog: %s", esp_err_to_name(wdt_err));
    }
  }

  ha_api_initialized = false;
  memset(auth_header, 0, sizeof(auth_header));

  // Notify status callback about deinitialization
  ha_status_change(HA_STATUS_OFFLINE);

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

  // Notify that we're starting a sync operation
  ha_status_change(HA_STATUS_SYNCING);

  // Clear all states first
  memset(states, 0, sizeof(ha_entity_state_t) * entity_count);

  esp_err_t overall_result = ESP_OK;
  int success_count = 0;
  int consecutive_failures = 0;

  // Fetch each entity state individually but with early exit on consecutive failures
  for (int i = 0; i < entity_count; i++)
  {
    // Reset watchdog during long sync operations
    if (task_watchdog_subscribed)
    {
      esp_task_wdt_reset();
    }

    esp_err_t result = ha_api_get_entity_state(entity_ids[i], &states[i]);
    if (result == ESP_OK)
    {
      success_count++;
      consecutive_failures = 0; // Reset failure counter on success
      // Entity state fetched successfully
    }
    else
    {
      consecutive_failures++;
      debug_log_warning_f(DEBUG_TAG_HA_API, "Failed to fetch entity %s: %s", entity_ids[i], esp_err_to_name(result));
      overall_result = result; // Keep track of last error

      // Early exit with more intelligent failure detection
      if (consecutive_failures >= 2)
      {
        // Stop for persistent connection issues
        if (result == ESP_ERR_HTTP_CONNECT || result == ESP_ERR_HTTP_EAGAIN)
        {
          debug_log_error(DEBUG_TAG_HA_API, "Multiple consecutive connection failures, aborting sync to prevent timeout");
          break;
        }
        // Also stop for timeout errors that indicate network issues
        else if (result == ESP_ERR_TIMEOUT)
        {
          debug_log_error(DEBUG_TAG_HA_API, "Multiple consecutive timeouts, network appears unstable");
          break;
        }
      }
    }

    // Adaptive delay between requests based on failure status
    if (i < entity_count - 1) // Don't delay after the last request
    {
      if (consecutive_failures > 0)
      {
        // Longer delay after failures to let network recover
        vTaskDelay(pdMS_TO_TICKS(250)); // 250ms delay after failures
      }
      else
      {
        // Normal delay for successful requests
        vTaskDelay(pdMS_TO_TICKS(100)); // 100ms delay between successful requests
      }
    }
  }

  if (success_count == entity_count)
  {

    // Notify successful sync completion
    ha_status_change(HA_STATUS_STATES_SYNCED);

    return ESP_OK;
  }
  else if (success_count > 0)
  {
    debug_log_warning_f(DEBUG_TAG_HA_API, "Fetched %d/%d entity states", success_count, entity_count);

    // Notify partial sync
    ha_status_change(HA_STATUS_PARTIAL_SYNC);

    return ESP_ERR_NOT_FOUND;
  }
  else
  {
    debug_log_error(DEBUG_TAG_HA_API, "Failed to fetch any entity states");

    // Notify sync failure
    ha_status_change(HA_STATUS_SYNC_FAILED);

    return overall_result;
  }
}

/**
 * @brief Get multiple entity states using bulk API request
 * This method fetches ALL states and filters the requested entities
 * Much faster than individual requests but larger response payload
 */
esp_err_t ha_api_get_multiple_entity_states_bulk(const char **entity_ids, int entity_count, ha_entity_state_t *states)
{
  if (!entity_ids || !states || entity_count <= 0)
  {
    return ESP_ERR_INVALID_ARG;
  }

  // Notify that we're starting a sync operation
  ha_status_change(HA_STATUS_SYNCING);

  // Clear all states first
  memset(states, 0, sizeof(ha_entity_state_t) * entity_count);

  // Measure timing
  int64_t start_time = esp_timer_get_time();
  int64_t request_time = 0; // Will be set after HTTP request completes

  // Make single bulk request to get ALL states
  ha_api_response_t response = {0};
  esp_err_t err = perform_http_request(HA_API_STATES_URL, "GET", NULL, &response);

  request_time = esp_timer_get_time() - start_time;

  debug_log_debug_f(DEBUG_TAG_HA_API, "Bulk request completed in %lld ms", request_time / 1000);

  if (err != ESP_OK)
  {
    debug_log_error_f(DEBUG_TAG_HA_API, "Bulk request failed: %s", esp_err_to_name(err));

    // Provide more specific error information
    if (err == ESP_ERR_TIMEOUT)
    {
      debug_log_error(DEBUG_TAG_HA_API, "Request timed out - Home Assistant may be slow or response too large");
    }
    else if (err == ESP_ERR_NOT_FOUND)
    {
      debug_log_error(DEBUG_TAG_HA_API, "Network connectivity issue - check Home Assistant server");
    }

    ha_status_change(HA_STATUS_SYNC_FAILED);
    return err;
  }

  // Check response size and completeness
  size_t response_size = response.response_data ? strlen(response.response_data) : 0;

  if (response_size == 0)
  {
    debug_log_error(DEBUG_TAG_HA_API, "Empty bulk response received");
    ha_api_free_response(&response);
    ha_status_change(HA_STATUS_SYNC_FAILED);
    return ESP_ERR_INVALID_RESPONSE;
  }

  // Check if response looks complete (should be JSON array ending with ']')
  if (response_size > 0 && response.response_data[response_size - 1] != ']')
  {
    debug_log_warning_f(DEBUG_TAG_HA_API, "Response may be truncated - doesn't end with ']' (last char: '%c', 0x%02X)",
                        response.response_data[response_size - 1],
                        (unsigned char)response.response_data[response_size - 1]);
  }

  if (response_size > 32768) // 32KB limit for safety
  {
    debug_log_warning_f(DEBUG_TAG_HA_API, "Very large response (%zu bytes), this may cause memory issues", response_size);
  }

  // Start JSON parsing timing
  int64_t parse_start = esp_timer_get_time();

  // Reset watchdog before JSON parsing since it can take time
  if (task_watchdog_subscribed)
  {
    esp_task_wdt_reset();
  }

  // Parse JSON response using entity states parser
  const size_t ASYNC_THRESHOLD = 16384; // 16KB threshold
  bool use_async = (response_size > ASYNC_THRESHOLD) && entity_states_parser_is_ready();

  esp_err_t parse_err;
  int success_count = 0;

  if (use_async)
  {

    // Reset watchdog before starting async parsing
    if (task_watchdog_subscribed)
    {
      esp_task_wdt_reset();
    }

    // Submit for async parsing
    parse_err = entity_states_parser_submit_async(
        response.response_data,
        response_size,
        entity_ids,
        entity_count,
        states);

    if (parse_err == ESP_OK)
    {
      // Wait for completion with timeout, resetting watchdog periodically
      const int timeout_ms = 30000;       // 30 second total timeout
      const int check_interval_ms = 2000; // Check every 2 seconds
      int elapsed_ms = 0;

      while (elapsed_ms < timeout_ms)
      {
        parse_err = entity_states_parser_wait_completion(check_interval_ms);

        // Reset watchdog during wait
        if (task_watchdog_subscribed)
        {
          esp_task_wdt_reset();
        }

        if (parse_err != ESP_ERR_TIMEOUT)
        {
          break; // Completed or failed
        }

        elapsed_ms += check_interval_ms;
        // Async parsing still in progress
      }

      // Final watchdog reset after potentially long async operation
      if (task_watchdog_subscribed)
      {
        esp_task_wdt_reset();
      }

      if (parse_err == ESP_OK)
      {
        // Reset watchdog before counting entities
        if (task_watchdog_subscribed)
        {
          esp_task_wdt_reset();
        }

        // Count successful entities (states with non-empty entity_id)
        for (int i = 0; i < entity_count; i++)
        {
          if (strlen(states[i].entity_id) > 0)
          {
            success_count++;
          }

          // Reset watchdog periodically during counting for very large entity lists
          if (i > 0 && (i % 50) == 0 && task_watchdog_subscribed)
          {
            esp_task_wdt_reset();
          }
        }
      }
      else if (parse_err == ESP_ERR_TIMEOUT)
      {
        debug_log_error(DEBUG_TAG_HA_API, "Async parsing timed out");
      }
      else
      {
        debug_log_error_f(DEBUG_TAG_HA_API, "Async parsing failed: %s", esp_err_to_name(parse_err));
      }
    }
    else
    {
      debug_log_warning_f(DEBUG_TAG_HA_API, "Failed to submit async parsing: %s, falling back to sync", esp_err_to_name(parse_err));
      use_async = false; // Fall back to sync parsing
    }
  }

  if (!use_async)
  {

    // Reset watchdog before sync parsing
    if (task_watchdog_subscribed)
    {
      esp_task_wdt_reset();
    }

    parse_err = entity_states_parser_parse_sync(
        response.response_data,
        entity_ids,
        entity_count,
        states);

    // Reset watchdog after sync parsing
    if (task_watchdog_subscribed)
    {
      esp_task_wdt_reset();
    }

    if (parse_err == ESP_OK)
    {
      // Reset watchdog before counting entities
      if (task_watchdog_subscribed)
      {
        esp_task_wdt_reset();
      }

      // Count successful entities (states with non-empty entity_id)
      for (int i = 0; i < entity_count; i++)
      {
        if (strlen(states[i].entity_id) > 0)
        {
          success_count++;
        }

        // Reset watchdog periodically during counting for very large entity lists
        if (i > 0 && (i % 50) == 0 && task_watchdog_subscribed)
        {
          esp_task_wdt_reset();
        }
      }
    }
    else
    {
      debug_log_error_f(DEBUG_TAG_HA_API, "Sync parsing failed: %s", esp_err_to_name(parse_err));
    }
  }

  ha_api_free_response(&response);

  // Handle parsing failure
  if (parse_err != ESP_OK)
  {
    ha_status_change(HA_STATUS_SYNC_FAILED);
    return parse_err;
  }

  int64_t parse_time = esp_timer_get_time() - parse_start;
  int64_t total_time = esp_timer_get_time() - start_time;

  debug_log_debug_f(DEBUG_TAG_HA_API, "Performance: Request %lld ms, Parse %lld ms, Total %lld ms",
                    (request_time / 1000), (parse_time / 1000), (total_time / 1000));

  // Determine result based on success count
  if (success_count == entity_count)
  {

    ha_status_change(HA_STATUS_STATES_SYNCED);
    return ESP_OK;
  }
  else if (success_count > 0)
  {
    debug_log_warning_f(DEBUG_TAG_HA_API, "Fetched %d/%d entity states via bulk request", success_count, entity_count);
    ha_status_change(HA_STATUS_PARTIAL_SYNC);
    return ESP_ERR_NOT_FOUND;
  }
  else
  {
    debug_log_error(DEBUG_TAG_HA_API, "Failed to fetch any entity states via bulk request");
    ha_status_change(HA_STATUS_SYNC_FAILED);
    return ESP_ERR_NOT_FOUND;
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

  ha_api_response_t local_response;
  ha_api_response_t *resp = response ? response : &local_response;

  esp_err_t err = perform_http_request(url, "POST", json_string, resp);

  if (err == ESP_OK && resp->success)
  {
  }
  else
  {
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

  ha_service_call_t service_call = {
      .domain = "switch",
      .service = "turn_on",
      .service_data = NULL};
  strncpy(service_call.entity_id, entity_id, sizeof(service_call.entity_id) - 1);

  ha_api_response_t response;
  esp_err_t result = ha_api_call_service(&service_call, &response);

  if (result == ESP_OK)
  {
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

  ha_service_call_t service_call = {
      .domain = "switch",
      .service = "turn_off",
      .service_data = NULL};
  strncpy(service_call.entity_id, entity_id, sizeof(service_call.entity_id) - 1);

  ha_api_response_t response;
  esp_err_t result = ha_api_call_service(&service_call, &response);

  if (result == ESP_OK)
  {
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

bool ha_api_is_ready(void)
{
  return ha_api_initialized;
}
