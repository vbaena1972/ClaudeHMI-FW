#ifndef UI_THEME_H
#define UI_THEME_H

/* ===================================================================
 *  Sistema de diseño de la HMI — alineado al design system "MedGuard"
 *  del proyecto hermano ClaudeHMI (estilo Nest híbrido: fondo carbón,
 *  alto contraste, minimalista). Mismos colores/radios/espaciados y la
 *  misma tipografía (Inter Medium). Los iconos usan la fuente Tabler
 *  propia de esta HMI (ver ui_icons.h).
 *  Pantalla del dispositivo: ST7796 480x320, LV_COLOR_DEPTH=16.
 *  Colores en 0xRRGGBB (usar con lv_color_hex / ui_col).
 * =================================================================== */

#include "lvgl.h"
#include "ui_icons.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Tipografía: Inter Medium (fonts/mg_font_*.c), igual que ClaudeHMI ----
 * Una sola familia + un peso; la jerarquía la dan tamaño y color.
 * El valor "héroe" numérico usa mg_num_48 (dígitos nítidos). */
LV_FONT_DECLARE(mg_font_12);
LV_FONT_DECLARE(mg_font_14);
LV_FONT_DECLARE(mg_font_18);
LV_FONT_DECLARE(mg_font_22);
LV_FONT_DECLARE(mg_font_36);
LV_FONT_DECLARE(mg_num_48);

#define UI_FONT_XS      &mg_font_12
#define UI_FONT_SM      &mg_font_12
#define UI_FONT_MD      &mg_font_14
#define UI_FONT_LG      &mg_font_18
#define UI_FONT_XL      &mg_font_22
#define UI_FONT_TITLE   &mg_font_18
#define UI_FONT_BIG     &mg_font_36
#define UI_FONT_HUGE    &mg_num_48    /* valor grande de presión/flujo (numérico) */

/* ---- Iconos (fuente Tabler propia) ---- */
#define UI_ICON_SM      &ui_font_tabler16
#define UI_ICON_MD      &ui_font_tabler20
#define UI_ICON_LG      &ui_font_tabler28

/* ---- Paleta (idéntica a PALETTE_DARK de ClaudeHMI/ui_theme.c) ---- */
#define UI_C_SCREEN_BG      0x15181e   /* bg */
#define UI_C_BEZEL          0x0f1115   /* bezel */
#define UI_C_CARD_BG        0x191c22   /* surface */
#define UI_C_CARD_BG2       0x12151a   /* surface2 (inset) */
#define UI_C_BORDER         0x2a2f37   /* hairline */
#define UI_C_BORDER_SOFT    0x2a2f37

#define UI_C_TEXT           0xf1f3f5   /* text */
#define UI_C_TEXT_STRONG    0xf1f3f5
#define UI_C_TEXT_2         0x9aa1ab   /* text_dim */
#define UI_C_TEXT_3         0x9aa1ab   /* text_dim */
#define UI_C_TEXT_MUTED     0x6f767f   /* text_mute */

#define UI_C_OK             0x34d399   /* ok */
#define UI_C_OK_SOFT        0x8ff0c4   /* decorativo */
#define UI_C_OK_DIM         0x5aa886   /* decorativo */
#define UI_C_OK_BG          0x123f2e   /* ok_bg */
#define UI_C_OK_BORDER      0x1d6e4f   /* ok_bd */

#define UI_C_WARN           0xfbbf24   /* warn */
#define UI_C_WARN_SOFT      0xf5d98a
#define UI_C_WARN_DIM       0xb98a3a
#define UI_C_WARN_BG        0x241a06   /* warn_bg */
#define UI_C_WARN_BORDER    0x7a5320   /* warn_bd */

#define UI_C_ALARM          0xf87171   /* alarm (rojo suave, como ClaudeHMI) */
#define UI_C_ALARM_SOFT     0xf87171
#define UI_C_ALARM_BG       0x2a1416   /* alarm_bg */
#define UI_C_ALARM_BORDER   0x5a2327   /* alarm_bd */

#define UI_C_INFO           0x38bdf8   /* info (cian) */
#define UI_C_BLUE           0x5aa0e0   /* bluetooth (decorativo) */
#define UI_C_TEAL           0x2dd4bf   /* accent (nube/navegación IoT) */

/* ---- Colores de gas (NFPA/CGA, idénticos a ClaudeHMI GAS_*) ---- */
#define UI_C_HEADER_ICONBG  0x0f6e56   /* GAS_GENERIC */
#define UI_C_GAS_O2         0x1d9e75   /* GAS_OXYGEN */
#define UI_C_GAS_AIR        0xe0a800   /* GAS_MED_AIR */
#define UI_C_GAS_N2O        0x2b6cb0   /* GAS_N2O */
#define UI_C_GAS_VAC        0xdfe3e8   /* GAS_VACUUM */

/* ---- Geometría (idéntica a ClaudeHMI) ---- */
#define UI_SCREEN_W     480
#define UI_SCREEN_H     320
#define UI_RADIUS_CARD  14
#define UI_RADIUS_TILE  11
#define UI_RADIUS_SM    8
#define UI_RADIUS_PILL  6
#define UI_PAD          12
#define UI_GAP          8

/* ---- Helpers de color ---- */
static inline lv_color_t ui_col(uint32_t hex) { return lv_color_hex(hex); }

#ifdef __cplusplus
}
#endif

#endif /* UI_THEME_H */
