#include "ui_connectivityScreen.h"
#include "ui_widgets.h"
#include "ui_theme.h"
#include "ui.h"

/* Conectividad (mockup 4g): grid 2x2 de tarjetas de conexión. */

lv_obj_t *ui_connectivityScreen = NULL;

static int32_t s_col[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
static int32_t s_row[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

static lv_obj_t *conn_card(lv_obj_t *grid, const char *sym, uint32_t icon_col, uint32_t border_col,
                           const char *title, const char *status, uint32_t status_col,
                           int col, int row)
{
    lv_obj_t *c = ui_card(grid);
    lv_obj_set_grid_cell(c, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
    lv_obj_set_style_radius(c, UI_RADIUS_TILE, 0);
    lv_obj_set_style_border_color(c, ui_col(border_col), 0);
    lv_obj_set_style_border_opa(c, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(c, 10, 0);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(c, 4, 0);

    lv_obj_t *top = ui_box(c);
    lv_obj_set_size(top, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(top, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *tl = ui_box(top);
    lv_obj_set_size(tl, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(tl, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tl, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(tl, 8, 0);
    ui_icon(tl, sym, UI_ICON_MD, icon_col);
    ui_label(tl, title, UI_FONT_MD, UI_C_TEXT);
    ui_label(top, status, UI_FONT_XS, status_col);
    return c;
}

void ui_connectivityScreen_screen_init(void)
{
    ui_connectivityScreen = ui_screen_base();
    lv_obj_set_flex_flow(ui_connectivityScreen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(ui_connectivityScreen, 8, 0);
    lv_obj_set_style_pad_row(ui_connectivityScreen, 8, 0);

    lv_obj_t *hdr = ui_nav_header(ui_connectivityScreen, "Conectividad");
    ui_pill(hdr, "telemetría activa", UI_FONT_XS, UI_C_OK_DIM, UI_C_OK_BG, UI_C_OK_BORDER);

    lv_obj_t *grid = ui_box(ui_connectivityScreen);
    lv_obj_set_width(grid, LV_PCT(100));
    lv_obj_set_flex_grow(grid, 1);
    lv_obj_set_style_pad_column(grid, 8, 0);
    lv_obj_set_style_pad_row(grid, 8, 0);
    lv_obj_set_grid_dsc_array(grid, s_col, s_row);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);

    /* Wi-Fi */
    lv_obj_t *wifi = conn_card(grid, UI_SYM_WIFI, UI_C_OK, UI_C_OK_BORDER, "Wi-Fi", "Conectado", UI_C_OK, 0, 0);
    ui_label(wifi, "Hospital-BIOMED-5G", UI_FONT_SM, 0xcfd3d9);
    lv_obj_t *sig = ui_box(wifi);
    lv_obj_set_size(sig, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(sig, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(sig, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(sig, 6, 0);
    lv_obj_t *bar = lv_bar_create(sig);
    lv_obj_set_flex_grow(bar, 1);
    lv_obj_set_height(bar, 5);
    lv_obj_set_style_bg_color(bar, ui_col(0x22262d), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, ui_col(UI_C_OK), LV_PART_INDICATOR);
    lv_bar_set_value(bar, 82, LV_ANIM_OFF);
    ui_label(sig, "-52 dBm", UI_FONT_XS, UI_C_TEXT_MUTED);
    ui_label(wifi, "IP 10.4.12.87 · DHCP", UI_FONT_XS, UI_C_TEXT_MUTED);

    /* Nube MQTT */
    lv_obj_t *cloud = conn_card(grid, UI_SYM_CLOUD_CHECK, UI_C_TEAL, UI_C_TEAL, "Nube · MQTT", "Sincronizado", UI_C_TEAL, 1, 0);
    ui_label(cloud, "broker.axira.io:8883 · TLS", UI_FONT_SM, 0xcfd3d9);
    lv_obj_t *sp1 = ui_box(cloud); lv_obj_set_flex_grow(sp1, 1); lv_obj_set_width(sp1, LV_PCT(100));
    ui_label(cloud, "Última sync hace 8 s · QoS 1", UI_FONT_XS, UI_C_TEXT_MUTED);
    ui_label(cloud, "Buffer local 0 registros", UI_FONT_XS, UI_C_TEXT_MUTED);

    /* Bluetooth (tap -> config por app) */
    lv_obj_t *ble = conn_card(grid, UI_SYM_BLUETOOTH, UI_C_BLUE, UI_C_BLUE, "Bluetooth LE", "Anunciando", UI_C_BLUE, 0, 1);
    lv_obj_add_flag(ble, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ble, ui_open_bleapp_cb, LV_EVENT_CLICKED, NULL);
    ui_label(ble, "Visible para app de servicio", UI_FONT_SM, 0xcfd3d9);
    lv_obj_t *sp2 = ui_box(ble); lv_obj_set_flex_grow(sp2, 1); lv_obj_set_width(sp2, LV_PCT(100));
    ui_label(ble, "Sin dispositivo emparejado", UI_FONT_XS, UI_C_TEXT_MUTED);
    ui_label(ble, "MAC C8:2B:…:9F", UI_FONT_XS, UI_C_TEXT_MUTED);

    /* Ethernet */
    lv_obj_t *eth = conn_card(grid, UI_SYM_NETWORK, UI_C_TEXT_MUTED, UI_C_BORDER, "Ethernet", "Sin cable", UI_C_TEXT_MUTED, 1, 1);
    ui_label(eth, "Respaldo cableado (failover)", UI_FONT_SM, UI_C_TEXT_3);
    lv_obj_t *sp3 = ui_box(eth); lv_obj_set_flex_grow(sp3, 1); lv_obj_set_width(sp3, LV_PCT(100));
    ui_label(eth, "RJ45 · 100 Mbps", UI_FONT_XS, UI_C_TEXT_MUTED);
    ui_label(eth, "Inactivo", UI_FONT_XS, UI_C_TEXT_MUTED);
}

void ui_connectivityScreen_screen_destroy(void)
{
    if (ui_connectivityScreen) { lv_obj_del(ui_connectivityScreen); ui_connectivityScreen = NULL; }
}
