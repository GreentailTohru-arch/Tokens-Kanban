#include "token_ui.h"
#include "postcard_codec.h"
#include "postcard_defaults.h"
#include "lvgl.h"
#include "src/misc/cache/instance/lv_image_cache.h"
#include <stdio.h>
#include <string.h>
LV_FONT_DECLARE(token_font_14);
#define BG 0x2F3A49
#define PANEL 0x374453
#define INK 0xF1F7F4
#define MUTED 0xADB8C3
#define TEAL 0x68C4CB
static lv_obj_t *tabs[3], *tab_labels[3], *value, *exact, *battery_label;
static lv_obj_t *quota_name[2], *quota_pct[2], *quota_time[2], *bars[2];
static lv_obj_t *credits, *plan, *quota_panel, *detail_panel, *details[3];
static lv_obj_t *home_group, *stats_group, *page_marks[3], *card_group, *card_image, *clock_label, *date_label;
static lv_image_dsc_t card_faces[2];
static const uint8_t *card_data;
static bool card_contact;
static lv_obj_t *model_names[2], *model_totals[2], *model_shares[2], *stats_period;
static lv_obj_t *active_days, *heat_months[3], *trend_dates[3], *peak_label, *heat_obj, *trend_obj;
static uint8_t heat_data[TOKEN_HEAT_DAYS];
static uint16_t trend_data[TOKEN_TREND_DAYS];
static bool charts_available;
static lv_obj_t *signal_light, *signal_label;
static const char *periods[] = {"今日", "本周", "累计"};
static lv_obj_t *label(lv_obj_t *parent, int x, int y, int w, const char *text, uint32_t color) {
    lv_obj_t *o = lv_label_create(parent);
    lv_obj_set_pos(o, x, y); lv_obj_set_width(o, w);
    lv_obj_set_style_text_font(o, &token_font_14, 0);
    lv_obj_set_style_text_color(o, lv_color_hex(color), 0);
    lv_label_set_long_mode(o, LV_LABEL_LONG_CLIP); lv_label_set_text(o, text); return o;
}
static lv_obj_t *box(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color, int radius) {
    lv_obj_t *o = lv_obj_create(parent); lv_obj_remove_style_all(o);
    lv_obj_set_pos(o,x,y); lv_obj_set_size(o,w,h);
    lv_obj_set_style_bg_color(o,lv_color_hex(color),0);
    lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0); lv_obj_set_style_radius(o,radius,0);
    lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE); return o;
}
static lv_obj_t *group(lv_obj_t *parent) {
    lv_obj_t *o=lv_obj_create(parent);lv_obj_remove_style_all(o);
    lv_obj_set_size(o,240,320);lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE);return o;
}
static void visible(lv_obj_t *o,bool show) {
    if(show) lv_obj_remove_flag(o,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(o,LV_OBJ_FLAG_HIDDEN);
}
static void draw_heat(lv_event_t *e) {
    lv_obj_t *o=lv_event_get_target_obj(e);lv_area_t area;lv_obj_get_coords(o,&area);
    lv_layer_t *layer=lv_event_get_layer(e);
    const uint32_t colors[]={0x344352,0x3B5267,0x456985,0x528DB5,0x9EDBFA};
    lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.radius=1;d.bg_opa=LV_OPA_COVER;
    for(int i=0;i<TOKEN_HEAT_DAYS;++i) {
        unsigned level=charts_available?heat_data[i]:0;if(level==5) continue;
        if(level>4) level=0;
        d.bg_color=lv_color_hex(colors[level]);
        int x=area.x1+(i/7)*8,y=area.y1+(i%7)*8;
        lv_area_t cell={x,y,x+5,y+5};lv_draw_rect(layer,&d,&cell);
    }
}
static void draw_trend(lv_event_t *e) {
    if(!charts_available) return;
    lv_obj_t *o=lv_event_get_target_obj(e);lv_area_t a;lv_obj_get_coords(o,&a);
    lv_layer_t *layer=lv_event_get_layer(e);
    lv_draw_line_dsc_t d;lv_draw_line_dsc_init(&d);d.color=lv_color_hex(0x70BFFF);d.width=2;
    for(int i=1;i<TOKEN_TREND_DAYS;++i) {
        d.p1.x=a.x1+(i-1)*(a.x2-a.x1)/59;d.p2.x=a.x1+i*(a.x2-a.x1)/59;
        d.p1.y=a.y2-trend_data[i-1]*(a.y2-a.y1)/1000;
        d.p2.y=a.y2-trend_data[i]*(a.y2-a.y1)/1000;
        lv_draw_line(layer,&d);
    }
}
void token_ui_create(void) {
    lv_obj_t *screen = lv_obj_create(NULL); lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen,lv_color_hex(BG),0);
    lv_obj_set_style_bg_opa(screen,LV_OPA_COVER,0); lv_obj_remove_flag(screen,LV_OBJ_FLAG_SCROLLABLE);
    signal_light=box(screen,20,17,12,12,0x788391,LV_RADIUS_CIRCLE);
    signal_label=label(screen,39,14,125,"Codex / 未连接",INK);
    battery_label=label(screen,169,14,54,"--%",MUTED);
    lv_obj_set_style_text_align(battery_label,LV_TEXT_ALIGN_RIGHT,0);
    home_group=group(screen);
    for (int i=0;i<3;++i) {
        tabs[i]=box(home_group,16+i*70,43,66,27,PANEL,7);
        tab_labels[i]=label(tabs[i],0,4,66,periods[i],MUTED);
        lv_obj_set_style_text_align(tab_labels[i],LV_TEXT_ALIGN_CENTER,0);
    }
    label(home_group,18,82,205,"TOTAL TOKENS",MUTED);
    value=label(home_group,16,104,210,"--",INK); lv_obj_set_style_text_font(value,&lv_font_montserrat_32,0);
    exact=label(home_group,18,143,208,"连接电脑后同步",MUTED);
    quota_panel=box(home_group,14,172,212,104,PANEL,10);
    for(int i=0;i<2;++i) {
        int y=7+i*48;
        quota_name[i]=label(quota_panel,10,y,110,"账户额度",MUTED);
        quota_pct[i]=label(quota_panel,119,y,83,"--",INK);
        lv_obj_set_style_text_align(quota_pct[i],LV_TEXT_ALIGN_RIGHT,0);
        bars[i]=lv_bar_create(quota_panel); lv_obj_remove_style_all(bars[i]);
        lv_obj_set_pos(bars[i],10,y+21); lv_obj_set_size(bars[i],192,5);
        lv_obj_set_style_bg_color(bars[i],lv_color_hex(0x475B69),0);
        lv_obj_set_style_bg_opa(bars[i],LV_OPA_COVER,0); lv_obj_set_style_radius(bars[i],3,0);
        lv_obj_set_style_bg_color(bars[i],lv_color_hex(TEAL),LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(bars[i],LV_OPA_COVER,LV_PART_INDICATOR);
        lv_obj_set_style_radius(bars[i],3,LV_PART_INDICATOR);
        quota_time[i]=label(quota_panel,10,y+28,192,"暂无数据",MUTED);
    }
    detail_panel=box(home_group,14,172,212,104,PANEL,10);
    for(int i=0;i<3;++i) details[i]=label(detail_panel,10,10+i*28,192,"--",INK);
    plan=label(home_group,18,280,82,"Codex",MUTED);
    credits=label(home_group,101,280,122,"",MUTED); lv_obj_set_style_text_align(credits,LV_TEXT_ALIGN_RIGHT,0);
    stats_group=group(screen);
    label(stats_group,18,42,82,"模型",INK);
    stats_period=label(stats_group,150,42,72,"今日",MUTED);
    lv_obj_set_style_text_align(stats_period,LV_TEXT_ALIGN_RIGHT,0);
    for(int i=0;i<2;++i) {
        model_names[i]=label(stats_group,18,65+i*22,104,"--",INK);
        model_totals[i]=label(stats_group,122,65+i*22,61,"--",INK);
        model_shares[i]=label(stats_group,185,65+i*22,37,"--",MUTED);
        lv_obj_set_style_text_align(model_totals[i],LV_TEXT_ALIGN_RIGHT,0);
        lv_obj_set_style_text_align(model_shares[i],LV_TEXT_ALIGN_RIGHT,0);
    }
    box(stats_group,18,113,204,1,0x455361,0);
    label(stats_group,18,121,70,"活动",INK);
    active_days=label(stats_group,100,121,122,"等待统计",MUTED);
    lv_obj_set_style_text_align(active_days,LV_TEXT_ALIGN_RIGHT,0);
    heat_obj=group(stats_group);lv_obj_set_pos(heat_obj,17,145);lv_obj_set_size(heat_obj,206,54);
    lv_obj_add_event_cb(heat_obj,draw_heat,LV_EVENT_DRAW_MAIN,NULL);
    for(int i=0;i<3;++i) {
        heat_months[i]=label(stats_group,18+i*86,202,32,"--",MUTED);
        trend_dates[i]=label(stats_group,18+i*81,286,42,"--",MUTED);
    }
    label(stats_group,18,225,60,"趋势",INK);
    peak_label=label(stats_group,85,225,137,"--",MUTED);
    lv_obj_set_style_text_align(peak_label,LV_TEXT_ALIGN_RIGHT,0);
    trend_obj=group(stats_group);lv_obj_set_pos(trend_obj,19,250);lv_obj_set_size(trend_obj,202,33);
    lv_obj_add_event_cb(trend_obj,draw_trend,LV_EVENT_DRAW_MAIN,NULL);
    visible(stats_group,false);visible(detail_panel,false);
    card_group=group(screen);
    clock_label=label(card_group,18,14,46,"00:00",INK);
    date_label=label(card_group,70,14,152,"",INK);
    lv_obj_set_style_text_align(date_label,LV_TEXT_ALIGN_RIGHT,0);
    card_image=lv_image_create(card_group);lv_obj_set_pos(card_image,0,38);
    visible(card_group,false);visible(card_image,false);
    for(int i=0;i<3;++i) page_marks[i]=box(screen,81+i*28,309,22,3,i==0?TEAL:0x52616E,2);
    token_ui_postcard_image(postcard_default_pixels);
    lv_screen_load(screen);
}
void token_ui_postcard_image(const uint8_t *data) {
    for(unsigned i=0;i<2;++i) {
        lv_image_cache_drop(&card_faces[i]);
        card_faces[i]=(lv_image_dsc_t){.header={.magic=LV_IMAGE_HEADER_MAGIC,
            .cf=LV_COLOR_FORMAT_RGB565,.w=POSTCARD_WIDTH,.h=POSTCARD_HEIGHT,.stride=POSTCARD_WIDTH*2},
            .data_size=POSTCARD_FACE_BYTES,.data=data+i*POSTCARD_FACE_BYTES};
    }
    card_data=data;lv_image_set_src(card_image,&card_faces[card_contact?1:0]);visible(card_image,true);
}
void token_ui_postcard_clock(uint64_t seconds,bool dated,bool contact) {
    char text[48];snprintf(text,sizeof(text),"%02u:%02u",(unsigned)(seconds/3600%24),(unsigned)(seconds/60%60));
    lv_label_set_text(clock_label,text);
    unsigned month,day,weekday;
    static const char *weekdays[]={"日","一","二","三","四","五","六"};
    if(dated && postcard_calendar(seconds,&month,&day,&weekday))
        snprintf(text,sizeof(text),"%u月%u日, 星期%s",month,day,weekdays[weekday]);
    else text[0]=0;
    lv_label_set_text(date_label,text);
    if(card_contact!=contact && card_data) lv_image_set_src(card_image,&card_faces[contact?1:0]);
    card_contact=contact;
}
static void duration(char *out,size_t size,int64_t sec) {
    if(sec<0) snprintf(out,size,"暂无重置时间");
    else if(sec==0) snprintf(out,size,"等待额度更新");
    else if(sec>=86400) snprintf(out,size,"%lld天 %lld小时后重置",(long long)(sec/86400),(long long)(sec/3600%24));
    else snprintf(out,size,"%lld小时 %lld分后重置",(long long)(sec/3600),(long long)(sec/60%60));
}
void token_ui_update(const token_snapshot_t *s,unsigned period,bool show_cost,bool received,uint64_t elapsed_ms,int battery,unsigned page) {
    char text[80],num[32];
    if(period>2) period=0;
    if(page>2) page=0;
    visible(home_group,page==0);visible(stats_group,page==1);visible(card_group,page==2);
    visible(signal_light,page!=2);visible(signal_label,page!=2);visible(battery_label,page!=2);
    visible(quota_panel,!show_cost);visible(detail_panel,show_cost);
    for(unsigned i=0;i<3;++i) lv_obj_set_style_bg_color(page_marks[i],lv_color_hex(i==page?TEAL:0x52616E),0);
    for(unsigned i=0;i<3;++i) {
        lv_obj_set_style_bg_color(tabs[i],lv_color_hex(i==period?0x516471:PANEL),0);
        lv_obj_set_style_text_color(tab_labels[i],lv_color_hex(i==period?0xBDECD8:MUTED),0);
    }
    if(battery<0) snprintf(text,sizeof(text),"--%%"); else snprintf(text,sizeof(text),"%d%%",battery);
    lv_label_set_text(battery_label,text);
    int64_t total=received?token_total(s,period):-1;
    token_number(num,sizeof(num),total,total>=100000000); lv_label_set_text(value,num);
    if(total<0) snprintf(text,sizeof(text),"连接电脑后同步");
    else if(show_cost) {
        int idx=T_DAY_CENTS+(int)period*2;
        int64_t cents=s->v[idx];
        const char *prefix=s->v[idx+1]==2?"部分估算":"估算";
        if(s->v[idx+1]<=0 || cents<0) snprintf(text,sizeof(text),"估算 -- / 数据不足");
        else if(cents>=100000000) snprintf(text,sizeof(text),"%s $%.2fM",prefix,(double)cents/100000000.0);
        else snprintf(text,sizeof(text),"%s $%lld.%02lld",prefix,(long long)(cents/100),(long long)(cents%100));
    }
    else { token_number(num,sizeof(num),total,false); snprintf(text,sizeof(text),"%s tokens",num); }
    lv_label_set_text(exact,text);
    const char *detail_names[]={"输入","缓存","输出"};
    for(int i=0;i<3;++i) {
        token_number(num,sizeof(num),received?s->v[T_DAY_IN+period*3+i]:-1,true);
        snprintf(text,sizeof(text),"%s  %s",detail_names[i],num);lv_label_set_text(details[i],text);
    }
    charts_available=received && s->v[T_ANALYTICS]==1;
    if(charts_available) { memcpy(heat_data,s->heat,sizeof(heat_data));memcpy(trend_data,s->trend,sizeof(trend_data)); }
    lv_obj_invalidate(heat_obj);lv_obj_invalidate(trend_obj);
    const char *models[]={"其他模型","gpt-6-astra","gpt-6.1-sol","gpt-6-sol","gpt-5.6-sol","gpt-5.6-luna","gpt-6-luna","gpt-5.6-terra"};
    lv_label_set_text(stats_period,periods[period]);
    for(int i=0;i<2;++i) {
        int idx=T_MODELS+(int)period*4+i*2;int64_t id=s->v[idx],count=s->v[idx+1];
        bool valid=charts_available && count>=0 && id>=0 && id<=7;
        lv_label_set_text(model_names[i],valid?models[id]:"--");
        token_number(num,sizeof(num),valid?count:-1,count>=10000);lv_label_set_text(model_totals[i],num);
        if(valid && total>0) snprintf(text,sizeof(text),"%lld%%",(long long)((count*100+total/2)/total));else snprintf(text,sizeof(text),"--");
        lv_label_set_text(model_shares[i],text);
    }
    if(charts_available) snprintf(text,sizeof(text),"活跃 %lld 天",(long long)s->v[T_ACTIVE_DAYS]);else snprintf(text,sizeof(text),"等待统计");
    lv_label_set_text(active_days,text);
    for(int i=0;i<3;++i) {
        if(charts_available) snprintf(text,sizeof(text),"%lld月",(long long)s->v[T_HEAT_MONTH0+i]);else snprintf(text,sizeof(text),"--");
        lv_label_set_text(heat_months[i],text);
        int64_t date=s->v[T_TREND_DATE0+i];
        if(charts_available) snprintf(text,sizeof(text),"%lld/%lld",(long long)(date/100),(long long)(date%100));else snprintf(text,sizeof(text),"--");
        lv_label_set_text(trend_dates[i],text);
    }
    token_number(num,sizeof(num),charts_available?s->v[T_PEAK]:-1,true);
    snprintf(text,sizeof(text),"峰值 %s",num);lv_label_set_text(peak_label,text);
    bool live=token_online(received,elapsed_ms,0);
    int signal=token_signal(s,received,elapsed_ms);
    const uint32_t colors[]={0x788391,0x5BA7FF,0x4CDD91,0xFF626C};
    const char *states[]={"状态未知","待命","正在运行","额度不足"};
    lv_obj_set_style_bg_color(signal_light,lv_color_hex(colors[signal]),0);
    snprintf(text,sizeof(text),"Codex / %s",!live?(received?"数据过期":"未连接"):states[signal]);
    lv_label_set_text(signal_label,text);
    for(int i=0;i<2;++i) {
        int idx=T_USED1+i*3; int64_t window=s->v[idx+1];
        if(window==10080) snprintf(text,sizeof(text),"每周额度");
        else if(window==300) snprintf(text,sizeof(text),"5小时额度");
        else if(window>0) snprintf(text,sizeof(text),"%lld分额度",(long long)window);
        else snprintf(text,sizeof(text),"账户额度");
        lv_label_set_text(quota_name[i],text);
        bool valid=received && s->v[idx]>=0 && s->v[T_QUOTA_AGE]>=0 && s->v[T_QUOTA_AGE]+(int64_t)(elapsed_ms/1000)<180;
        if(valid) snprintf(text,sizeof(text),"剩余 %lld%%",(long long)(100-s->v[idx])); else snprintf(text,sizeof(text),"--");
        lv_label_set_text(quota_pct[i],text); lv_bar_set_value(bars[i],valid?(int32_t)(100-s->v[idx]):0,LV_ANIM_OFF);
        if(valid) duration(text,sizeof(text),token_remaining(s->v[idx+2],elapsed_ms));
        else snprintf(text,sizeof(text),"%s",s->v[idx]<0?"暂无数据":"额度待更新");
        lv_label_set_text(quota_time[i],text);
    }
    const char *plans[]={"Codex","Free","Plus","Pro","Team","Enterprise"};
    lv_label_set_text(plan,plans[s->v[T_PLAN]>=0&&s->v[T_PLAN]<=5?s->v[T_PLAN]:0]);
    if(received && s->v[T_CREDITS]>=0) snprintf(text,sizeof(text),"重置券 %lld",(long long)s->v[T_CREDITS]); else text[0]=0;
    lv_label_set_text(credits,text);
}
