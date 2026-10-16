/* Real application UI rendered by LVGL into PPM, without a physical display. */
#include "lvgl.h"
#include "token_ui.h"
#include "postcard_codec.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
LV_FONT_DECLARE(token_font_14);
static unsigned char framebuffer[240*320*3];
static uint16_t buffer[240*32];
static void flush(lv_display_t *display,const lv_area_t *area,uint8_t *pixels) {
    uint16_t *p=(uint16_t *)pixels;
    for(int y=area->y1;y<=area->y2;++y) for(int x=area->x1;x<=area->x2;++x) {
        uint16_t c=*p++;size_t i=(y*240+x)*3;
        framebuffer[i]=((c>>11)&31)*255/31;framebuffer[i+1]=((c>>5)&63)*255/63;framebuffer[i+2]=(c&31)*255/31;
    }
    lv_display_flush_ready(display);
}
static void save(const char *name) {
    lv_refr_now(NULL);FILE *file=fopen(name,"wb");assert(file);
    fprintf(file,"P6\n240 320\n255\n");fwrite(framebuffer,1,sizeof(framebuffer),file);fclose(file);
}
static void check_labels(lv_obj_t *obj) {
    if(lv_obj_has_flag(obj,LV_OBJ_FLAG_HIDDEN)) return;
    if(lv_obj_check_type(obj,&lv_label_class)) {
        const unsigned char *p=(const unsigned char *)lv_label_get_text(obj);
        const lv_font_t *font=lv_obj_get_style_text_font(obj,LV_PART_MAIN);
        while(*p) {
            uint32_t cp=*p++;
            if(cp>=0xE0) { cp=(cp&15)<<12;cp|=(*p++&63)<<6;cp|=*p++&63; }
            else if(cp>=0xC0) { cp=(cp&31)<<6;cp|=*p++&63; }
            lv_font_glyph_dsc_t d={0};
            assert(lv_font_get_glyph_dsc(font,&d,cp,0)&&!d.is_placeholder);
        }
        lv_point_t size;
        lv_text_get_size(&size,lv_label_get_text(obj),font,lv_obj_get_style_text_letter_space(obj,0),0,10000,LV_TEXT_FLAG_NONE);
        if(size.x>lv_obj_get_width(obj)) { fprintf(stderr,"Clipped label: %s (%d > %d)\n",lv_label_get_text(obj),(int)size.x,(int)lv_obj_get_width(obj));abort(); }
    }
    for(uint32_t i=0;i<lv_obj_get_child_count(obj);++i) check_labels(lv_obj_get_child(obj,i));
}
int main(void) {
    lv_init();lv_display_t *display=lv_display_create(240,320);
    lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display,buffer,NULL,sizeof(buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display,flush);
    lv_font_glyph_dsc_t d={0};assert(lv_font_get_glyph_dsc(&token_font_14,&d,0x4ECA,0)&&!d.is_placeholder);
    assert(!lv_font_get_glyph_dsc(&token_font_14,&d,0x9F98,0)||d.is_placeholder);
    token_ui_create();page_config_t pages=page_config_default();token_ui_pages(&pages,0);
    token_ui_startup(true);save("startup.ppm");check_labels(lv_layer_top());
    token_ui_startup(false);token_ui_startup(false);
    token_snapshot_t s={.v={1,18800000,15000000,15200,215584794,209747072,506820,640773677,623715584,1746255,13,10080,530000,-1,-1,-1,1,3,0}};
    s.v[T_ACTIVITY]=TOKEN_BUSY;s.transport=TOKEN_BLE;
    s.v[T_DAY_CENTS]=2736;s.v[T_DAY_COST_STATUS]=1;
    s.v[T_WEEK_CENTS]=15022;s.v[T_WEEK_COST_STATUS]=1;
    s.v[T_ALL_CENTS]=35187;s.v[T_ALL_COST_STATUS]=1;
    s.v[T_ANALYTICS]=1;s.v[T_ACTIVE_DAYS]=55;s.v[T_PEAK]=537300000;
    for(int i=0;i<3;++i) {
        s.v[T_MODELS+i*4]=1;s.v[T_MODELS+i*4+1]=token_total(&s,i)*99/100;
        s.v[T_MODELS+i*4+2]=2;s.v[T_MODELS+i*4+3]=token_total(&s,i)/100;
        s.v[T_HEAT_MONTH0+i]=4+i*3;s.v[T_TREND_DATE0+i]=818+i*99;
    }
    s.v[T_TREND_DATE2]=1016;
    for(int i=0;i<TOKEN_HEAT_DAYS;++i) s.heat[i]=i<110?0:(uint8_t)(1+(i*7)%4);
    s.heat[180]=s.heat[181]=5;
    for(int i=0;i<TOKEN_TREND_DAYS;++i) s.trend[i]=(uint16_t)(40+(i*37)%200+(i%17==0?650:0));
    token_ui_update(&s,0,false,true,0,86,1);save("analytics.ppm");check_labels(lv_screen_active());
    token_ui_update(&s,2,true,true,0,86,0);save("details.ppm");check_labels(lv_screen_active());
    token_snapshot_t fixture=s;
    token_ui_update(&s,0,false,true,0,86,0);save("overview.ppm");check_labels(lv_screen_active());
    unsigned char before[sizeof(framebuffer)];memcpy(before,framebuffer,sizeof(framebuffer));
    token_ui_update(&s,0,true,true,0,86,0);save("cost.ppm");check_labels(lv_screen_active());
    /* OK changes the subline and swaps quota/details; header/total/footer stay. */
    for(int y=0;y<320;++y) if((y<143 || y>=161) && (y<172 || y>=276))
        assert(!memcmp(before+y*240*3,framebuffer+y*240*3,240*3));
    s.v[T_DAY_COST_STATUS]=2;
    token_ui_update(&s,0,true,true,0,86,0);save("partial-cost.ppm");check_labels(lv_screen_active());
    s.v[T_DAY_CENTS]=-1;s.v[T_DAY_COST_STATUS]=0;
    token_ui_update(&s,0,true,true,0,86,0);save("unknown-cost.ppm");check_labels(lv_screen_active());
    s.v[T_DAY_CENTS]=1000000000000000LL;s.v[T_DAY_COST_STATUS]=2;
    token_ui_update(&s,0,true,true,0,86,0);check_labels(lv_screen_active());
    s.v[T_DAY_CENTS]=2736;s.v[T_DAY_COST_STATUS]=1;
    s.v[T_ACTIVITY]=TOKEN_IDLE;
    token_ui_update(&s,0,false,true,0,86,0);save("idle.ppm");check_labels(lv_screen_active());
    s.v[T_USED1]=100;
    token_ui_update(&s,0,false,true,0,86,0);save("limited.ppm");check_labels(lv_screen_active());
    s.v[T_USED1]=13;
    token_ui_update(&s,1,false,true,0,86,0);save("week.ppm");check_labels(lv_screen_active());
    token_ui_update(&s,2,true,true,0,-1,0);save("all-cost.ppm");check_labels(lv_screen_active());
    token_ui_update(&s,0,false,true,200000,86,0);save("disconnected.ppm");check_labels(lv_screen_active());
    for(int i=0;i<TOKEN_FIELDS;++i)s.v[i]=-1;s.v[T_PLAN]=0;
    token_ui_update(&s,0,false,false,0,-1,0);save("waiting.ppm");check_labels(lv_screen_active());
    token_ui_update(&s,0,false,false,0,-1,1);save("analytics-waiting.ppm");check_labels(lv_screen_active());
    s=fixture;
    token_ui_update(&s,0,false,true,0,86,PAGE_POSTCARD);token_ui_pages(&pages,PAGE_POSTCARD);token_ui_postcard_clock(0,false,false);save("postcard-empty.ppm");
    token_ui_postcard_clock(1792143660ULL,true,false);save("postcard-default-front.ppm");check_labels(lv_screen_active());
    token_ui_postcard_clock(1792143660ULL,true,true);save("postcard-default-contact.ppm");check_labels(lv_screen_active());
    static uint8_t card[POSTCARD_BYTES];
    FILE *asset=fopen("postcard-fixture.rgb565","rb");
    if(asset) {assert(fread(card,1,sizeof(card),asset)==sizeof(card));fclose(asset);}
    else for(unsigned i=0;i<sizeof(card);i+=2) {card[i]=0xa9;card[i+1]=0x31;}
    token_ui_postcard_image(card);token_ui_postcard_clock(1792143660ULL,true,false);save("postcard-front.ppm");check_labels(lv_screen_active());
    token_ui_postcard_clock(1792143660ULL,true,true);save("postcard-contact.ppm");check_labels(lv_screen_active());
    for(unsigned day=0;day<7;++day){token_ui_postcard_clock(1798416000ULL+day*86400,true,false);lv_refr_now(NULL);check_labels(lv_screen_active());}
    monitor_config_t monitor=monitor_default();
    system_snapshot_t metrics={.v={1,8,8,64,570,510,9910,1000}};
    token_ui_pages(&pages,PAGE_SYSTEM);token_ui_system(&metrics,true,&monitor);token_ui_update(&s,0,false,true,0,86,PAGE_SYSTEM);save("system.ppm");check_labels(lv_screen_active());
    for(unsigned i=SYS_CPU;i<SYS_FIELDS;++i)metrics.v[i]=-1;
    token_ui_system(&metrics,true,&monitor);save("system-missing.ppm");check_labels(lv_screen_active());
    metrics=(system_snapshot_t){.v={1,100,100,100,1500,1500,1000000000000LL,1000000000000LL}};
    token_ui_system(&metrics,true,&monitor);save("system-max.ppm");check_labels(lv_screen_active());
    token_ui_system(&metrics,false,&monitor);save("system-stale.ppm");check_labels(lv_screen_active());
    for(unsigned mask=1;mask<16;++mask){pages.enabled=mask;token_nav_t nav={.page=PAGE_COUNT};page_nav_normalize(&pages,&nav);for(unsigned n=0;n<page_count(&pages);++n){token_ui_pages(&pages,nav.page);token_ui_update(&s,0,false,true,0,86,nav.page);lv_refr_now(NULL);check_labels(lv_screen_active());page_nav_key(&pages,&nav,TOKEN_KEY_OK,true);}}
    pages=page_config_default();
    for(unsigned i=0;i<1000;++i) {
        s.v[T_DAY_IN]=(int64_t)i*90000;s.v[T_DAY_CACHE]=i;s.v[T_DAY_OUT]=i;
        token_ui_update(&s,i%3,(i%3)==0,true,i*1000,86,i%4);token_ui_pages(&pages,i%4);token_ui_system(&metrics,i%2==0,&monitor);token_ui_postcard_clock(1792143660ULL+(i%7)*86400,true,i%2==0);lv_refr_now(NULL);
    }
    metrics=(system_snapshot_t){.v={1,100,100,100,1500,1500,1000000000000LL,1000000000000LL,100000,100,9007199254740991LL,9007199254740991LL}};
    token_ui_update(&s,0,false,true,0,86,PAGE_SYSTEM);
    for(unsigned n=0;n<=6;++n)for(unsigned metric=0;metric<M_METRICS;++metric)for(unsigned mode=0;mode<5;++mode) {
        monitor=monitor_default();monitor.bits=1;monitor.unit=mode;monitor.decimals=2;monitor.space=1;strcpy(monitor.up_label,"上传");strcpy(monitor.down_label,"下载");
        for(unsigned j=0;j<6;++j){monitor.cards[j].enabled=j<n;monitor.cards[j].metric=metric;strcpy(monitor.cards[j].title,"春夏秋冬天地");}
        token_ui_system(&metrics,true,&monitor);lv_refr_now(NULL);check_labels(lv_screen_active());
    }
    monitor=monitor_default();metrics=(system_snapshot_t){.v={1,8,8,64,570,510,9910,1000,3380,12,1073741824,2147483648}};
    token_ui_system(&metrics,true,&monitor);save("system-configurable.ppm");
    for(unsigned page=0;page<PAGE_COUNT;++page) {
        token_ui_pages(&pages,page);token_ui_update(&s,0,false,true,0,86,page);
        lv_refr_now(NULL);unsigned char underlay[sizeof(framebuffer)];memcpy(underlay,framebuffer,sizeof(underlay));
        token_ui_volume(true,page==0?0:page==1?100:35);lv_tick_inc(200);lv_timer_handler();lv_refr_now(NULL);
        check_labels(lv_layer_top());char name[40];snprintf(name,sizeof(name),"volume-page-%u.ppm",page);save(name);
        token_ui_volume(false,35);lv_tick_inc(200);lv_timer_handler();lv_refr_now(NULL);
        assert(!memcmp(underlay,framebuffer,sizeof(underlay)));
    }
    for(unsigned i=0;i<200;++i){token_ui_volume(true,i%101);lv_tick_inc(200);lv_timer_handler();token_ui_volume(false,i%101);lv_tick_inc(200);lv_timer_handler();}
    assert(lv_mem_test()==LV_RESULT_OK);
    lv_mem_monitor_t memory;lv_mem_monitor(&memory);
    printf("LVGL 48KB pool: free=%zu largest=%zu max_used=%zu after 1000 updates\n",memory.free_size,memory.free_biggest_size,memory.max_used);
    puts("Actual LVGL UI and runtime font checks: PASS");return 0;
}
