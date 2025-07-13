
/**
 * @file ui_config.h
 * @brief UI Configuration and Font Definitions
 *
 * Provides centralized font configuration and UI constants for the
 * ESP32-S3-8048S050 system monitor dashboard.
 */

#pragma once

#include "lvgl.h"

// ═══════════════════════════════════════════════════════════════════════════════
// FONT DECLARATIONS - DEFINED IN ui_config.c
// ═══════════════════════════════════════════════════════════════════════════════

extern const lv_font_t *font_title;       ///< Large title font (28px or fallback to 14px)
extern const lv_font_t *font_normal;      ///< Normal text font (16px or fallback to 14px)
extern const lv_font_t *font_small;       ///< Small text font (14px)
extern const lv_font_t *font_big_numbers; ///< Large numbers font (32px or fallback to 14px)

// ═══════════════════════════════════════════════════════════════════════════════
// UI CONSTANTS
// ═══════════════════════════════════════════════════════════════════════════════

// UI Control Labels
#define UI_CONTROLS_LABEL_A "Water Pump"
#define UI_CONTROLS_LABEL_B "Wave Maker"
#define UI_CONTROLS_LABEL_C "Light Switch"
#define UI_CONTROLS_LABEL_D "FEED"

// Backwards compatibility macros (deprecated)
#define UI_LABEL_A UI_CONTROLS_LABEL_A
#define UI_LABEL_B UI_CONTROLS_LABEL_B
#define UI_LABEL_C UI_CONTROLS_LABEL_C
#define UI_LABEL_D UI_CONTROLS_LABEL_D
