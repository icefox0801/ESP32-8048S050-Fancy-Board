
/**
 * @file ui_config.h
 * @brief UI Configuration and Font Definitions
 *
 * Provides centralized font configuration and UI constants for the
 * ESP32-S3-8048S050 system monitor dashboard.
 */

#pragma once

#include "lvgl.h"
#include "smart_config.h"

// ═══════════════════════════════════════════════════════════════════════════════
// FONT DECLARATIONS
// ═══════════════════════════════════════════════════════════════════════════════
extern const lv_font_t *font_title;       // Large title font
extern const lv_font_t *font_normal;      // Normal text
extern const lv_font_t *font_small;       // Small text
extern const lv_font_t *font_big_numbers; // Large numbers

// ═══════════════════════════════════════════════════════════════════════════════
// UI CONSTANTS
// ═══════════════════════════════════════════════════════════════════════════════

// UI Control Labels
#define UI_CONTROLS_LABEL_A HA_ENTITY_A_LABEL
#define UI_CONTROLS_LABEL_B HA_ENTITY_B_LABEL
#define UI_CONTROLS_LABEL_C HA_ENTITY_C_LABEL
#define UI_CONTROLS_LABEL_D HA_ENTITY_D_LABEL
