/**
 * @file entity_states_parser.c
 * @brief Async Entity States JSON Parser Implementation
 *
 * This module provides asynchronous JSON parsing functionality for Home Assistant
 * entity states using SPIRAM for large response handling. The parser runs on
 * CPU idle time to avoid blocking the main application.
 *
 * @author System Monitor Dashboard
 * @date 2025-08-19
 */

#include "entity_states_parser.h"
#include "system_debug_utils.h"
#include <esp_timer.h>
#include <esp_task_wdt.h>
#include <esp_heap_caps.h>
#include <cJSON.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════════
// CONSTANTS AND MACROS
// ═══════════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════════════
// STATIC VARIABLES
// ═══════════════════════════════════════════════════════════════════════════════

static QueueHandle_t parse_queue = NULL;
static TaskHandle_t parse_task_handle = NULL;
static bool parser_initialized = false;
static entity_parser_stats_t parser_stats = {0};

// ═══════════════════════════════════════════════════════════════════════════════
// PRIVATE FUNCTION DECLARATIONS
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Main async JSON parsing task
 * @param pvParameters Task parameters (unused)
 */
static void entity_parse_task(void *pvParameters);

/**
 * @brief Parse entity states from JSON data
 * @param json_data Raw JSON string
 * @param entity_ids Array of entity IDs to find
 * @param entity_count Number of entities to find
 * @param states Output array for entity states
 * @return Number of entities successfully parsed
 */
static int parse_entity_states_from_json(
    const char *json_data,
    const char **entity_ids,
    int entity_count,
    ha_entity_state_t *states);

// ═══════════════════════════════════════════════════════════════════════════════
// PUBLIC FUNCTION IMPLEMENTATIONS
// ═══════════════════════════════════════════════════════════════════════════════

esp_err_t entity_states_parser_init(void)
{
  if (parser_initialized)
  {
    debug_log_info(DEBUG_TAG_PARSER, "Entity states parser already initialized");
    return ESP_OK;
  }

  // Create queue for parsing jobs
  parse_queue = xQueueCreate(ENTITY_PARSER_MAX_JOBS, sizeof(entity_parse_job_t));
  if (parse_queue == NULL)
  {
    debug_log_error(DEBUG_TAG_PARSER, "Failed to create parse queue");
    return ESP_ERR_NO_MEM;
  }

  // Create async parsing task with low priority (runs on CPU idle)
  BaseType_t task_created = xTaskCreatePinnedToCore(
      entity_parse_task,             // Task function
      "entity_parser",               // Task name
      ENTITY_PARSER_TASK_STACK_SIZE, // Stack size
      NULL,                          // Parameters
      ENTITY_PARSER_TASK_PRIORITY,   // Priority (low for idle processing)
      &parse_task_handle,            // Task handle
      ENTITY_PARSER_TASK_CORE        // Core affinity
  );

  if (task_created != pdPASS)
  {
    debug_log_error(DEBUG_TAG_PARSER, "Failed to create parse task");
    vQueueDelete(parse_queue);
    parse_queue = NULL;
    return ESP_ERR_NO_MEM;
  }

  // Reset statistics
  memset(&parser_stats, 0, sizeof(parser_stats));

  parser_initialized = true;
  debug_log_info(DEBUG_TAG_PARSER, "🔄 Entity states parser initialized");
  debug_log_info_f(DEBUG_TAG_PARSER, "📋 Queue size: %d jobs, Task core: %d, Priority: %d",
                   ENTITY_PARSER_MAX_JOBS, ENTITY_PARSER_TASK_CORE, ENTITY_PARSER_TASK_PRIORITY);

  return ESP_OK;
}

void entity_states_parser_deinit(void)
{
  if (!parser_initialized)
  {
    return;
  }

  // Delete task
  if (parse_task_handle)
  {
    vTaskDelete(parse_task_handle);
    parse_task_handle = NULL;
  }

  // Delete queue
  if (parse_queue)
  {
    vQueueDelete(parse_queue);
    parse_queue = NULL;
  }

  parser_initialized = false;
  debug_log_info(DEBUG_TAG_PARSER, "🔌 Entity states parser deinitialized");
}

esp_err_t entity_states_parser_submit_async(
    const char *json_data,
    size_t json_size,
    const char **entity_ids,
    int entity_count,
    ha_entity_state_t *states)
{
  if (!parser_initialized)
  {
    debug_log_error(DEBUG_TAG_PARSER, "Parser not initialized");
    return ESP_ERR_INVALID_STATE;
  }

  if (!json_data || !entity_ids || !states || entity_count <= 0)
  {
    debug_log_error(DEBUG_TAG_PARSER, "Invalid parameters");
    return ESP_ERR_INVALID_ARG;
  }

  // Allocate SPIRAM for JSON data copy
  char *json_copy = heap_caps_malloc(json_size + 1, MALLOC_CAP_SPIRAM);
  if (!json_copy)
  {
    debug_log_error_f(DEBUG_TAG_PARSER, "Failed to allocate %zu bytes in SPIRAM for JSON", json_size);
    return ESP_ERR_NO_MEM;
  }

  // Copy JSON data to SPIRAM
  memcpy(json_copy, json_data, json_size);
  json_copy[json_size] = '\0';

  // Create parse job
  entity_parse_job_t job = {
      .json_data = json_copy,
      .json_size = json_size,
      .entity_ids = entity_ids,
      .entity_count = entity_count,
      .states = states,
      .caller_task = xTaskGetCurrentTaskHandle()};

  // Submit job to queue
  if (xQueueSend(parse_queue, &job, 0) != pdTRUE)
  {
    debug_log_error(DEBUG_TAG_PARSER, "Parse queue is full, cannot submit job");
    heap_caps_free(json_copy);
    return ESP_ERR_NO_MEM;
  }

  debug_log_info_f(DEBUG_TAG_PARSER, "📤 Submitted async parse job (%zu bytes, %d entities)",
                   json_size, entity_count);

  return ESP_OK;
}

esp_err_t entity_states_parser_wait_completion(uint32_t timeout_ms)
{
  if (!parser_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  // Convert timeout to ticks
  TickType_t timeout_ticks = (timeout_ms == portMAX_DELAY) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);

  // Wait for notification from parse task
  uint32_t notification_value = ulTaskNotifyTake(pdTRUE, timeout_ticks);

  if (notification_value == 0)
  {
    debug_log_warning_f(DEBUG_TAG_PARSER, "⏰ Parse operation timed out after %lu ms", timeout_ms);
    return ESP_ERR_TIMEOUT;
  }

  debug_log_info(DEBUG_TAG_PARSER, "✅ Async parse completed");
  return ESP_OK;
}

esp_err_t entity_states_parser_parse_sync(
    const char *json_data,
    const char **entity_ids,
    int entity_count,
    ha_entity_state_t *states)
{
  if (!json_data || !entity_ids || !states || entity_count <= 0)
  {
    return ESP_ERR_INVALID_ARG;
  }

  debug_log_info_f(DEBUG_TAG_PARSER, "🔄 Starting synchronous parse (%d entities)", entity_count);

  int64_t start_time = esp_timer_get_time();

  // Parse entities synchronously
  int found_count = parse_entity_states_from_json(json_data, entity_ids, entity_count, states);

  int64_t parse_time = esp_timer_get_time() - start_time;

  debug_log_info_f(DEBUG_TAG_PARSER, "⏱️ Sync parse completed in %lld ms (%d/%d entities found)",
                   parse_time / 1000, found_count, entity_count);

  // Update statistics
  parser_stats.jobs_processed++;
  parser_stats.entities_found += found_count;
  parser_stats.entities_missing += (entity_count - found_count);
  parser_stats.total_parse_time_ms += parse_time / 1000;
  parser_stats.average_parse_time_ms = parser_stats.total_parse_time_ms / parser_stats.jobs_processed;

  return (found_count > 0) ? ESP_OK : ESP_ERR_NOT_FOUND;
}

esp_err_t entity_states_parser_get_stats(entity_parser_stats_t *stats)
{
  if (!stats)
  {
    return ESP_ERR_INVALID_ARG;
  }

  memcpy(stats, &parser_stats, sizeof(entity_parser_stats_t));
  return ESP_OK;
}

void entity_states_parser_reset_stats(void)
{
  memset(&parser_stats, 0, sizeof(parser_stats));
  debug_log_info(DEBUG_TAG_PARSER, "📊 Parser statistics reset");
}

bool entity_states_parser_is_ready(void)
{
  return parser_initialized;
}

int entity_states_parser_get_queue_size(void)
{
  if (!parse_queue)
  {
    return -1;
  }

  return uxQueueMessagesWaiting(parse_queue);
}

// ═══════════════════════════════════════════════════════════════════════════════
// PRIVATE FUNCTION IMPLEMENTATIONS
// ═══════════════════════════════════════════════════════════════════════════════

static void entity_parse_task(void *pvParameters)
{
  entity_parse_job_t job;

  debug_log_info(DEBUG_TAG_PARSER, "🔄 Entity parse task started");

  while (1)
  {
    // Wait for parse jobs
    if (xQueueReceive(parse_queue, &job, portMAX_DELAY) == pdTRUE)
    {
      debug_log_info_f(DEBUG_TAG_PARSER, "🔄 Processing parse job (%zu bytes, %d entities)",
                       job.json_size, job.entity_count);

      int64_t start_time = esp_timer_get_time();

      // Parse entities from JSON
      int found_count = parse_entity_states_from_json(
          job.json_data,
          job.entity_ids,
          job.entity_count,
          job.states);

      int64_t parse_time = esp_timer_get_time() - start_time;

      debug_log_info_f(DEBUG_TAG_PARSER, "⏱️ Async parse completed in %lld ms (%d/%d entities found)",
                       parse_time / 1000, found_count, job.entity_count);

      // Update statistics
      parser_stats.jobs_processed++;
      parser_stats.entities_found += found_count;
      parser_stats.entities_missing += (job.entity_count - found_count);
      parser_stats.total_parse_time_ms += parse_time / 1000;
      parser_stats.average_parse_time_ms = parser_stats.total_parse_time_ms / parser_stats.jobs_processed;

      // Update largest response size
      if (job.json_size > parser_stats.largest_response_size)
      {
        parser_stats.largest_response_size = job.json_size;
      }

      // Free SPIRAM JSON data
      if (job.json_data)
      {
        heap_caps_free(job.json_data);
      }

      // Notify caller that processing is complete
      xTaskNotifyGive(job.caller_task);
    }
  }
}

static int parse_entity_states_from_json(
    const char *json_data,
    const char **entity_ids,
    int entity_count,
    ha_entity_state_t *states)
{
  if (!json_data || !entity_ids || !states)
  {
    return 0;
  }

  // Parse JSON
  cJSON *json = cJSON_Parse(json_data);
  if (!json)
  {
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr != NULL)
    {
      long error_pos = error_ptr - json_data;
      debug_log_error_f(DEBUG_TAG_PARSER, "JSON parse failed: %s at position %ld", error_ptr, error_pos);
    }
    else
    {
      debug_log_error(DEBUG_TAG_PARSER, "JSON parse failed: Unknown error");
    }
    return 0;
  }

  if (!cJSON_IsArray(json))
  {
    debug_log_error(DEBUG_TAG_PARSER, "Expected JSON array for entity states");
    cJSON_Delete(json);
    return 0;
  }

  int total_entities = cJSON_GetArraySize(json);
  int success_count = 0;

  debug_log_info_f(DEBUG_TAG_PARSER, "📦 Processing %d entities from JSON response", total_entities);

  // Clear all states first
  memset(states, 0, sizeof(ha_entity_state_t) * entity_count);

  // Look for our requested entities in the JSON response
  for (int i = 0; i < entity_count; i++)
  {
    bool found = false;

    // Reset watchdog periodically during the search
    if (i % 10 == 0)
    {
      esp_task_wdt_reset();
    }

    // Search through all entities in JSON response
    cJSON *entity = NULL;
    cJSON_ArrayForEach(entity, json)
    {
      if (!cJSON_IsObject(entity))
        continue;

      cJSON *entity_id_json = cJSON_GetObjectItem(entity, "entity_id");
      if (!entity_id_json || !cJSON_IsString(entity_id_json))
        continue;

      // Check if this is one of our requested entities
      if (strcmp(cJSON_GetStringValue(entity_id_json), entity_ids[i]) == 0)
      {
        // Found matching entity, extract its state
        cJSON *state_json = cJSON_GetObjectItem(entity, "state");
        if (!state_json || !cJSON_IsString(state_json))
        {
          debug_log_warning_f(DEBUG_TAG_PARSER, "Entity %s has no valid state", entity_ids[i]);
          break;
        }

        // Fill in the state data
        ha_entity_state_t *state = &states[i];

        // Copy entity ID
        strncpy(state->entity_id, entity_ids[i], sizeof(state->entity_id) - 1);
        state->entity_id[sizeof(state->entity_id) - 1] = '\0';

        // Copy state value
        strncpy(state->state, cJSON_GetStringValue(state_json), sizeof(state->state) - 1);
        state->state[sizeof(state->state) - 1] = '\0';

        // Extract friendly name from attributes if available
        cJSON *attributes = cJSON_GetObjectItem(entity, "attributes");
        if (attributes && cJSON_IsObject(attributes))
        {
          cJSON *friendly_name = cJSON_GetObjectItem(attributes, "friendly_name");
          if (friendly_name && cJSON_IsString(friendly_name))
          {
            strncpy(state->friendly_name, cJSON_GetStringValue(friendly_name), sizeof(state->friendly_name) - 1);
            state->friendly_name[sizeof(state->friendly_name) - 1] = '\0';
          }
        }

        // Set timestamp
        state->last_updated = time(NULL);

        success_count++;
        found = true;
        debug_log_info_f(DEBUG_TAG_PARSER, "✅ Found entity %s: %s (%s)",
                         entity_ids[i], state->state, state->friendly_name);
        break;
      }
    }

    if (!found)
    {
      debug_log_warning_f(DEBUG_TAG_PARSER, "❌ Entity %s not found in JSON response", entity_ids[i]);
    }
  }

  cJSON_Delete(json);

  return success_count;
}
