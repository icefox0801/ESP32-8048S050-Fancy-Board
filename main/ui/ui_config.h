
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
// FONT DEFINITIONS WITH FALLBACK
// ═══════════════════════════════════════════════════════════════════════════════

#ifdef CONFIG_LV_FONT_MONTSERRAT_28
static const lv_font_t *font_title = &lv_font_montserrat_28; ///< Large title font (28px)
#else
static const lv_font_t *font_title = &lv_font_montserrat_14; ///< Fallback to 14px
#endif

#ifdef CONFIG_LV_FONT_MONTSERRAT_16
static const lv_font_t *font_normal = &lv_font_montserrat_16; ///< Normal text font (16px)
#else
static const lv_font_t *font_normal = &lv_font_montserrat_14; ///< Fallback to 14px
#endif

static const lv_font_t *font_small = &lv_font_montserrat_14; ///< Small text font (14px)

#ifdef CONFIG_LV_FONT_MONTSERRAT_32
static const lv_font_t *font_big_numbers = &lv_font_montserrat_32; ///< Large numbers font (32px)
#else
static const lv_font_t *font_big_numbers = &lv_font_montserrat_14; ///< Fallback to 14px
#endif

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
