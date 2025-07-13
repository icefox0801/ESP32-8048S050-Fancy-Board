/**
 * @file ui_config.c
 * @brief UI Configuration Implementation
 *
 * Implements font configuration with proper fallbacks for the
 * ESP32-S3-8048S050 system monitor dashboard.
 */

#include "ui_config.h"

// ═══════════════════════════════════════════════════════════════════════════════
// FONT DEFINITIONS WITH FALLBACK
// ═══════════════════════════════════════════════════════════════════════════════

#ifdef CONFIG_LV_FONT_MONTSERRAT_28
const lv_font_t *font_title = &lv_font_montserrat_28; // Large title font (28px)
#else
const lv_font_t *font_title = &lv_font_montserrat_14; // Fallback to 14px
#endif

#ifdef CONFIG_LV_FONT_MONTSERRAT_16
const lv_font_t *font_normal = &lv_font_montserrat_16; // Normal text
#else
const lv_font_t *font_normal = &lv_font_montserrat_14; // Fallback to 14px
#endif

const lv_font_t *font_small = &lv_font_montserrat_14; // Small text

#ifdef CONFIG_LV_FONT_MONTSERRAT_32
const lv_font_t *font_big_numbers = &lv_font_montserrat_32; // Large numbers (32px)
#else
const lv_font_t *font_big_numbers = &lv_font_montserrat_14; // Fallback to 14px
#endif
