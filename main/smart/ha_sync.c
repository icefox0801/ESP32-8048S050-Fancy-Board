/**
 * @file ha_sync.c
 * @brief Home Assistant Device State Synchronization Implementation
 *
 * This module provides the implementation for synchronizing device states
 * with Home Assistant
 */

#include "ha_sync.h"
#include "ha_api.h"
#include "ui_dashboard.h"
#include "ui_controls_panel.h"
#include "../utils/system_debug_utils.h"
#include <esp_timer.h>
#include <string.h>

esp_err_t ha_sync_immediate_switches(void)
{
  debug_log_info(DEBUG_TAG_HA_SYNC, "Performing immediate switch sync using bulk API");

  // Entity IDs from smart config
  const char *switch_entity_ids[] = {
      HA_ENTITY_A_ID, // Switch A
      HA_ENTITY_B_ID, // Switch B
      HA_ENTITY_C_ID  // Switch C
  };
  const int switch_count = sizeof(switch_entity_ids) / sizeof(switch_entity_ids[0]);

  // Fetch all switch states in one bulk request
  ha_entity_state_t switch_states[switch_count];
  esp_err_t ret = ha_api_get_multiple_entity_states(switch_entity_ids, switch_count, switch_states);

  if (ret == ESP_OK)
  {
    // Update UI with switch states
    bool switch_a_on = (strcmp(switch_states[0].state, "on") == 0);
    bool switch_b_on = (strcmp(switch_states[1].state, "on") == 0);
    bool switch_c_on = (strcmp(switch_states[2].state, "on") == 0);

    // Update the UI with switch states
    controls_panel_set_switch(SWITCH_A, switch_a_on);
    controls_panel_set_switch(SWITCH_B, switch_b_on);
    controls_panel_set_switch(SWITCH_C, switch_c_on);

    debug_log_info_f(DEBUG_TAG_HA_SYNC, "Immediate sync completed: %s=%s, %s=%s, %s=%s",
                     switch_entity_ids[0], switch_states[0].state,
                     switch_entity_ids[1], switch_states[1].state,
                     switch_entity_ids[2], switch_states[2].state);

    return ESP_OK;
  }
  else
  {
    debug_log_warning_f(DEBUG_TAG_HA_SYNC, "Immediate sync failed: %s", esp_err_to_name(ret));
    return ret;
  }
}
