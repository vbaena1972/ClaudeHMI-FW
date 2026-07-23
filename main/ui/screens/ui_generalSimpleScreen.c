#include "ui_generalSimpleScreen.h"
#include "ui_i18n.h"
#include "ui_widgets.h"
#include "ui_theme.h"
#include "ui_cfg.h"
#include "ui_nav.h"
#include <string.h>
lv_obj_t *ui_generalSimpleScreen=NULL;static lv_obj_t*s_pct;
static const char s_espanol[] = {'E','s','p','a',(char)0xC3,(char)0xB1,'o','l',0};
static void rebuild(void){lv_obj_t*o=ui_generalSimpleScreen;ui_generalSimpleScreen=NULL;s_pct=NULL;ui_generalSimpleScreen_screen_init();ui_nav_replace(ui_generalSimpleScreen);lv_obj_delete_delayed(o,250);}
static void lang_cb(lv_event_t*e){const char *lang=(const char*)lv_event_get_user_data(e);ui_cfg_set_lang(lang);rebuild();}
static void theme_cb(lv_event_t*e){ui_cfg_set_theme((const char*)lv_event_get_user_data(e));rebuild();}
static void dim_cb(lv_event_t*e){ui_cfg_set_dim_minutes((int)(intptr_t)lv_event_get_user_data(e));rebuild();}
static void bright_cb(lv_event_t*e){lv_obj_t*s=lv_event_get_target(e);int v=lv_slider_get_value(s);if(s_pct)lv_label_set_text_fmt(s_pct,"%d%%",v);if(lv_event_get_code(e)==LV_EVENT_RELEASED)ui_cfg_set_brightness(v);else ui_cfg_preview_brightness(v);}
static lv_obj_t*base_card(lv_obj_t*p){lv_obj_t*c=ui_card(p);lv_obj_set_style_pad_all(c,10,0);lv_obj_set_style_pad_row(c,5,0);lv_obj_set_flex_flow(c,LV_FLEX_FLOW_COLUMN);return c;}
static lv_obj_t*seg(lv_obj_t*p,const char*t,bool on,lv_event_cb_t cb,void*ud){lv_obj_t*b=lv_button_create(p);lv_obj_set_height(b,34);lv_obj_set_flex_grow(b,1);ui_style_button(b,on?UI_C_OK_BG:UI_C_CARD_BG2);lv_obj_set_style_border_width(b,1,0);lv_obj_set_style_border_color(b,ui_col(on?UI_C_OK_BORDER:UI_C_BORDER),0);lv_obj_t*l=ui_label(b,t,UI_FONT_XS,on?UI_C_OK:UI_C_TEXT_2);lv_obj_center(l);lv_obj_add_event_cb(b,cb,LV_EVENT_CLICKED,ud);return b;}
void ui_generalSimpleScreen_screen_init(void)
{
 ui_generalSimpleScreen=ui_screen_base();lv_obj_set_flex_flow(ui_generalSimpleScreen,LV_FLEX_FLOW_COLUMN);lv_obj_set_style_pad_all(ui_generalSimpleScreen,8,0);lv_obj_set_style_pad_row(ui_generalSimpleScreen,7,0);ui_nav_header(ui_generalSimpleScreen,_t("Pantalla"));
 lv_obj_t*b=base_card(ui_generalSimpleScreen);lv_obj_set_size(b,LV_PCT(100),72);lv_obj_t*bh=ui_box(b);lv_obj_set_size(bh,LV_PCT(100),20);lv_obj_set_flex_flow(bh,LV_FLEX_FLOW_ROW);lv_obj_set_flex_align(bh,LV_FLEX_ALIGN_SPACE_BETWEEN,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);ui_label(bh,_t("Brillo"),UI_FONT_MD,UI_C_TEXT);s_pct=ui_label(bh,"",UI_FONT_MD,UI_C_OK);lv_label_set_text_fmt(s_pct,"%d%%",ui_cfg_brightness());lv_obj_t*s=lv_slider_create(b);lv_obj_set_size(s,LV_PCT(100),7);lv_slider_set_range(s,30,100);lv_slider_set_value(s,ui_cfg_brightness()<30?30:ui_cfg_brightness(),LV_ANIM_OFF);lv_obj_set_ext_click_area(s,10);lv_obj_add_event_cb(s,bright_cb,LV_EVENT_VALUE_CHANGED,NULL);lv_obj_add_event_cb(s,bright_cb,LV_EVENT_RELEASED,NULL);
 lv_obj_t*mid=ui_box(ui_generalSimpleScreen);lv_obj_set_size(mid,LV_PCT(100),72);lv_obj_set_flex_flow(mid,LV_FLEX_FLOW_ROW);lv_obj_set_style_pad_column(mid,7,0);
 lv_obj_t*t=base_card(mid);lv_obj_set_flex_grow(t,1);lv_obj_set_height(t,72);ui_label(t,_t("Tema"),UI_FONT_MD,UI_C_TEXT);lv_obj_t*tr=ui_box(t);lv_obj_set_size(tr,LV_PCT(100),36);lv_obj_set_flex_flow(tr,LV_FLEX_FLOW_ROW);lv_obj_set_style_pad_column(tr,6,0);bool light=!strcmp(ui_cfg_theme(),"light");seg(tr,_t("Oscuro"),!light,theme_cb,"dark");seg(tr,_t("Claro"),light,theme_cb,"light");
 lv_obj_t*l=base_card(mid);lv_obj_set_flex_grow(l,1);lv_obj_set_height(l,72);ui_label(l,_t("Idioma"),UI_FONT_MD,UI_C_TEXT);lv_obj_t*lr=ui_box(l);lv_obj_set_size(lr,LV_PCT(100),36);lv_obj_set_flex_flow(lr,LV_FLEX_FLOW_ROW);lv_obj_set_style_pad_column(lr,6,0);bool en=!strcmp(ui_cfg_lang(),"en");seg(lr,s_espanol,!en,lang_cb,"es");seg(lr,"English",en,lang_cb,"en");
 lv_obj_t*d=base_card(ui_generalSimpleScreen);lv_obj_set_size(d,LV_PCT(100),76);ui_label(d,_t("Atenuar tras inactividad"),UI_FONT_MD,UI_C_TEXT);lv_obj_t*dr=ui_box(d);lv_obj_set_size(dr,LV_PCT(100),38);lv_obj_set_flex_flow(dr,LV_FLEX_FLOW_ROW);lv_obj_set_style_pad_column(dr,6,0);int dm=ui_cfg_dim_minutes();seg(dr,_t("Nunca"),dm==0,dim_cb,(void*)(intptr_t)0);seg(dr,"1 min",dm==1,dim_cb,(void*)(intptr_t)1);seg(dr,"5 min",dm==5,dim_cb,(void*)(intptr_t)5);seg(dr,"10 min",dm==10,dim_cb,(void*)(intptr_t)10);
 lv_obj_t*f=ui_box(ui_generalSimpleScreen);lv_obj_set_size(f,LV_PCT(100),18);lv_obj_set_flex_align(f,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);ui_label(f,_t("Una alarma restaura inmediatamente el brillo"),UI_FONT_XS,UI_C_TEXT_MUTED);ui_cfg_apply_visual_mode(ui_generalSimpleScreen);
}
void ui_generalSimpleScreen_screen_destroy(void){if(ui_generalSimpleScreen){lv_obj_del(ui_generalSimpleScreen);ui_generalSimpleScreen=NULL;s_pct=NULL;}}