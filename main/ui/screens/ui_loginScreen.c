#include "ui_loginScreen.h"
#include "ui_widgets.h"
#include "ui_theme.h"
#include "ui_cfg.h"
#include "ui.h"
#include <stdint.h>
#include <string.h>

#define MAX_FAILS 5
#define LOCK_MS 30000U
static const char s_tecnico[] = {'T',(char)0xC3,(char)0xA9,'c','n','i','c','o',0};

lv_obj_t *ui_loginScreen = NULL;
static const app_user_t *s_target;
static char s_pin[9];
static int s_len, s_fails;
static uint32_t s_lock_until;
static lv_obj_t *s_who, *s_dots, *s_error;

static const char *role_name(app_user_role_t r){return r==APP_ROLE_FACTORY?"Fabricante":r==APP_ROLE_ADMIN?"Administrador":s_tecnico;}
static uint32_t role_color(app_user_role_t r){return r==APP_ROLE_FACTORY?0x8b5cf6:r==APP_ROLE_ADMIN?UI_C_BLUE:UI_C_OK;}
static void refresh(void){char b[24]="";for(int i=0;i<s_len&&i<8;i++){b[i*2]='*';if(i<7)b[i*2+1]=' ';}lv_label_set_text(s_dots,b);}
static void select_user(const app_user_t *u){s_target=u;s_len=0;s_pin[0]=0;refresh();lv_label_set_text_fmt(s_who,"%s - %s",u?u->name:"Seleccione usuario",u?role_name(u->role):"");lv_obj_add_flag(s_error,LV_OBJ_FLAG_HIDDEN);}
static void user_cb(lv_event_t *e){select_user(ui_auth_user_at((int)(intptr_t)lv_event_get_user_data(e)));}
static void key_cb(lv_event_t *e){
    char k=(char)(intptr_t)lv_event_get_user_data(e);uint32_t now=lv_tick_get();
    if(s_lock_until&&(int32_t)(now-s_lock_until)<0){lv_label_set_text(s_error,"Acceso bloqueado temporalmente");lv_obj_clear_flag(s_error,LV_OBJ_FLAG_HIDDEN);return;}s_lock_until=0;
    if(k=='<'){if(s_len>0)s_pin[--s_len]=0;refresh();return;}
    if(k=='='){
        if(s_target&&s_len>=4&&ui_auth_login(s_target,s_pin)){s_fails=0;ui_open_general_authenticated_cb(NULL);return;}
        s_len=0;s_pin[0]=0;refresh();if(++s_fails>=MAX_FAILS){s_fails=0;s_lock_until=now+LOCK_MS;lv_label_set_text(s_error,"5 intentos - bloqueado 30 segundos");}else lv_label_set_text_fmt(s_error,"PIN incorrecto - intento %d/%d",s_fails,MAX_FAILS);lv_obj_clear_flag(s_error,LV_OBJ_FLAG_HIDDEN);return;
    }
    if(k>='0'&&k<='9'&&s_len<8){s_pin[s_len++]=k;s_pin[s_len]=0;refresh();}
}
static lv_obj_t *key(lv_obj_t *g,const char *txt,char code,int col,int row){lv_obj_t *o=ui_card(g);lv_obj_set_grid_cell(o,LV_GRID_ALIGN_STRETCH,col,1,LV_GRID_ALIGN_STRETCH,row,1);lv_obj_set_style_pad_all(o,0,0);lv_obj_add_flag(o,LV_OBJ_FLAG_CLICKABLE);lv_obj_add_event_cb(o,key_cb,LV_EVENT_CLICKED,(void*)(intptr_t)code);lv_obj_t *kl = (code=='<' || code=='=') ? ui_icon(o,code=='<'?UI_SYM_BACKSPACE:UI_SYM_CHECK,UI_ICON_MD,code=='='?UI_C_OK:UI_C_TEXT) : ui_label(o,txt,UI_FONT_LG,UI_C_TEXT);lv_obj_center(kl);return o;}
static void add_user(lv_obj_t *p,int i){const app_user_t *u=ui_auth_user_at(i);if(!u)return;lv_obj_t *r=ui_card(p);lv_obj_set_size(r,LV_PCT(100),44);lv_obj_set_flex_flow(r,LV_FLEX_FLOW_ROW);lv_obj_set_flex_align(r,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);lv_obj_set_style_pad_all(r,7,0);lv_obj_set_style_pad_column(r,8,0);lv_obj_add_flag(r,LV_OBJ_FLAG_CLICKABLE);lv_obj_add_event_cb(r,user_cb,LV_EVENT_CLICKED,(void*)(intptr_t)i);ui_icon_badge(r,u->role==APP_ROLE_ADMIN?UI_SYM_SHIELD_CHECK:UI_SYM_SETTINGS,UI_ICON_SM,role_color(u->role),UI_C_CARD_BG,28);lv_obj_t *b=ui_box(r);lv_obj_set_flex_flow(b,LV_FLEX_FLOW_COLUMN);lv_obj_set_height(b,32);ui_label(b,u->name,UI_FONT_SM,UI_C_TEXT);ui_label(b,role_name(u->role),UI_FONT_XS,UI_C_TEXT_MUTED);}
void ui_loginScreen_screen_init(void){
    s_target=NULL;s_len=0;s_pin[0]=0;
    ui_loginScreen=ui_screen_base();lv_obj_set_flex_flow(ui_loginScreen,LV_FLEX_FLOW_COLUMN);lv_obj_set_style_pad_all(ui_loginScreen,8,0);lv_obj_set_style_pad_row(ui_loginScreen,6,0);
    ui_nav_header(ui_loginScreen,"Acceso a configuracion");
    lv_obj_t *body=ui_box(ui_loginScreen);lv_obj_set_width(body,LV_PCT(100));lv_obj_set_flex_grow(body,1);lv_obj_set_flex_flow(body,LV_FLEX_FLOW_ROW);lv_obj_set_style_pad_column(body,10,0);
    lv_obj_t *left=ui_box(body);lv_obj_set_size(left,220,LV_PCT(100));lv_obj_set_flex_flow(left,LV_FLEX_FLOW_COLUMN);lv_obj_set_style_pad_row(left,5,0);ui_label(left,"Seleccione su cuenta",UI_FONT_MD,UI_C_TEXT);for(int i=0;i<ui_auth_user_count();i++)add_user(left,i);
    lv_obj_t *right=ui_box(body);lv_obj_set_flex_grow(right,1);lv_obj_set_height(right,LV_PCT(100));lv_obj_set_flex_flow(right,LV_FLEX_FLOW_COLUMN);lv_obj_set_style_pad_row(right,5,0);s_who=ui_label(right,"Seleccione usuario",UI_FONT_SM,UI_C_TEXT_2);s_dots=ui_label(right,"",UI_FONT_MD,UI_C_OK);s_error=ui_label(right,"",UI_FONT_XS,UI_C_ALARM_SOFT);lv_obj_add_flag(s_error,LV_OBJ_FLAG_HIDDEN);
    static int32_t cols[]={LV_GRID_FR(1),LV_GRID_FR(1),LV_GRID_FR(1),LV_GRID_TEMPLATE_LAST};static int32_t rows[]={LV_GRID_FR(1),LV_GRID_FR(1),LV_GRID_FR(1),LV_GRID_FR(1),LV_GRID_TEMPLATE_LAST};lv_obj_t *g=ui_box(right);lv_obj_set_width(g,LV_PCT(100));lv_obj_set_flex_grow(g,1);lv_obj_set_grid_dsc_array(g,cols,rows);lv_obj_set_layout(g,LV_LAYOUT_GRID);lv_obj_set_style_pad_all(g,0,0);lv_obj_set_style_pad_row(g,4,0);lv_obj_set_style_pad_column(g,4,0);const char *d[] = {"1","2","3","4","5","6","7","8","9"};for(int i=0;i<9;i++)key(g,d[i],(char)('1'+i),i%3,i/3);key(g,"",'<',0,3);key(g,"0",'0',1,3);key(g,"",'=',2,3);
}
void ui_loginScreen_screen_destroy(void){if(ui_loginScreen){lv_obj_del(ui_loginScreen);ui_loginScreen=NULL;}}