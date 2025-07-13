
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
// FONT DECLARATIONS - DEFINED IN ui_helpers.c
// ═══════════════════════════════════════════════════════════════════════════════

extern const lv_font_t *font_title;       ///< Large title font (28px or fallback to 14px)
extern const lv_font_t *font_normal;      ///< Normal text font (16px or fallback to 14px)
extern const lv_font_t *font_small;       ///< Small text font (14px)
extern const lv_font_t *font_big_numbers; ///< Large numbers font (32px or fallback to 14px)

// ═══════════════════════════════════════════════════════════════════════════════
// UI CONSTANTS
// ═══════════════════════════════════════════════════════════════════════════════

// UI Control Labels
#define UI_CONTROLS_LABEL_A HA_ENTITY_A_LABEL
#define UI_CONTROLS_LABEL_B HA_ENTITY_B_LABEL
#define UI_CONTROLS_LABEL_C HA_ENTITY_C_LABEL
#define UI_CONTROLS_LABEL_D HA_ENTITY_D_LABEL
