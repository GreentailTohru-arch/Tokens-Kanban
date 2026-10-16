#include "device_runtime.h"
#include "device_sounds.h"
#include "device_diagnostics.h"
#include "startup_assets.h"
#include "bsp_audio.h"
#include "bsp_display.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>
typedef struct { unsigned command,id,tone,volume,volume_revision; device_config_t config; page_config_t pages; monitor_config_t monitor; } request_t;
static QueueHandle_t queue;
static device_config_t config,requested;
static page_config_t pages,requested_pages;
static unsigned save_kind;
static monitor_config_t monitor,requested_monitor;
static unsigned volume_revision;
static bool volume_unsaved,volume_save_pending,preview_requested,preview_pending;
static uint64_t volume_changed_at,volume_save_after,preview_next;
static atomic_uint live_volume;
static unsigned brightness=75,requested_brightness;
static atomic_bool preview_playback;
static device_behavior_t behavior;
static bool nvs_ready,event_seen,event_pending,config_dirty;
static unsigned save_id,save_state,event_host,event_sequence;
static atomic_bool wake_requested,boot_finished;
static portMUX_TYPE mutex=portMUX_INITIALIZER_UNLOCKED;

static bool play_tone(unsigned tone,unsigned volume) {
    if(!tone || !volume) return true;
    const int16_t *source;size_t count;
    switch(tone) {
        case 1:source=codex_notification_pcm;count=codex_notification_samples;break;
        case 3:source=codex_error_pcm;count=codex_error_samples;break;
        case 4:source=opencode_complete_pcm;count=opencode_complete_samples;break;
        case 5:source=opencode_error_pcm;count=opencode_error_samples;break;
        case 6:source=antigravity_complete_pcm;count=antigravity_complete_samples;break;
        case 7:source=antigravity_error_pcm;count=antigravity_error_samples;break;
        case 8:source=startup_sound_pcm;count=startup_sound_samples;break;
        default:return false;
    }
    static bool initialized;
    if(!initialized) { if(bsp_audio_init()!=ESP_OK) return false;initialized=true; }
    if(bsp_audio_wake()!=ESP_OK || bsp_audio_set_format(CODEX_SOUND_HZ,16,1)!=ESP_OK) return false;
    bsp_audio_set_volume((uint8_t)volume);
    /* Only user-provided recordings; no synthesized tone fallback. */
    unsigned playing_volume=volume;
    int16_t pcm[320];
    for(size_t offset=0;offset<count;offset+=320) {
        if(atomic_load(&preview_playback)) {
            unsigned latest=atomic_load(&live_volume);
            if(latest!=playing_volume){bsp_audio_set_volume((uint8_t)latest);playing_volume=latest;}
            if(!latest)break;
        }
        size_t n=count-offset;if(n>320)n=320;
        memcpy(pcm,source+offset,n*sizeof(*pcm));
        if(bsp_audio_write(pcm,n*sizeof(*pcm))!=ESP_OK)return false;
    }
    /* Drain the last queued samples before suspending the codec. */
    memset(pcm,0,sizeof(pcm));
    for(unsigned i=0;i<6;++i) if(bsp_audio_write(pcm,sizeof(pcm))!=ESP_OK)return false;
    return bsp_audio_sleep()==ESP_OK;
}
static void worker(void *arg) {
    (void)arg;request_t request;
    for(;;) {
        if(xQueueReceive(queue,&request,portMAX_DELAY)!=pdTRUE) continue;
        if(request.command==11) {
            nvs_handle_t handle;esp_err_t error=nvs_open("tokens_ui",NVS_READWRITE,&handle);
            if(error==ESP_OK){error=nvs_set_blob(handle,"display1",&request.volume,sizeof(request.volume));if(error==ESP_OK)error=nvs_commit(handle);nvs_close(handle);}
            portENTER_CRITICAL(&mutex);if(error==ESP_OK){brightness=request.volume;save_state=2;atomic_store(&wake_requested,true);}else save_state=3;portEXIT_CRITICAL(&mutex);
        } else if(request.command==9 || request.command==10) {
            bool ok=false;int peak=-1,average=-1;unsigned samples=0;
            if(request.command==10)ok=request.volume && play_tone(request.tone,request.volume);
            else {
                ok=bsp_audio_init()==ESP_OK && bsp_audio_wake()==ESP_OK && bsp_audio_set_format(CODEX_SOUND_HZ,16,1)==ESP_OK;
                uint32_t sum=0;peak=0;
                int16_t pcm[320];
                for(unsigned block=0;ok&&block<20;++block) {
                    ok=bsp_audio_read(pcm,sizeof(pcm))==ESP_OK;
                    if(ok)for(unsigned j=0;j<320;++j){int n=pcm[j]<0?-(int)pcm[j]:(int)pcm[j];if(n>peak)peak=n;sum+=(unsigned)n;++samples;}
                }
                average=samples?(int)(sum/samples):-1;
                if(bsp_audio_sleep()!=ESP_OK)ok=false;
            }
            if(!ok)(void)bsp_audio_sleep();
            device_diagnostics_test_complete(request.id,ok,peak,average,samples);
        } else if(request.command==7) {
            nvs_handle_t handle;esp_err_t error=nvs_open("tokens_ui",NVS_READWRITE,&handle);
            if(error==ESP_OK){error=nvs_set_blob(handle,"mon1",&request.monitor,sizeof(request.monitor));if(error==ESP_OK)error=nvs_commit(handle);nvs_close(handle);}
            portENTER_CRITICAL(&mutex);if(error==ESP_OK){monitor=request.monitor;save_state=2;atomic_store(&wake_requested,true);}else save_state=3;portEXIT_CRITICAL(&mutex);
        } else if(request.command==4) {
            nvs_handle_t handle;esp_err_t error=nvs_open("tokens_ui",NVS_READWRITE,&handle);
            if(error==ESP_OK) {
                error=nvs_set_blob(handle,"pages1",&request.pages,sizeof(request.pages));
                if(error==ESP_OK)error=nvs_commit(handle);
                nvs_close(handle);
            }
            portENTER_CRITICAL(&mutex);
            if(error==ESP_OK) {pages=request.pages;save_state=2;atomic_store(&wake_requested,true);}
            else save_state=3;
            portEXIT_CRITICAL(&mutex);
        } else if(request.command==1 || request.command==5) {
            bool automatic=request.command==5;
            portENTER_CRITICAL(&mutex);
            if(automatic)request.config=config;
            else if(request.volume_revision!=volume_revision)request.config.volume=config.volume;
            unsigned written_revision=volume_revision;
            bool same=!config_dirty && !memcmp(&config,&request.config,sizeof(config));
            portEXIT_CRITICAL(&mutex);
            nvs_handle_t handle;esp_err_t error=same?ESP_OK:nvs_open("tokens_ui",NVS_READWRITE,&handle);
            if(error==ESP_OK && !same) {
                error=nvs_set_blob(handle,"cfg2",&request.config,sizeof(request.config));
                if(error==ESP_OK)error=nvs_commit(handle);
                nvs_close(handle);
            }
            portENTER_CRITICAL(&mutex);
            if(error==ESP_OK) {
                bool moved=written_revision!=volume_revision;
                if(!automatic) {
                    unsigned latest=config.volume;config=request.config;if(moved)config.volume=latest;
                    atomic_store(&live_volume,config.volume);save_state=2;atomic_store(&wake_requested,true);
                }
                config_dirty=moved;volume_unsaved=moved;
            } else {
                if(!automatic)save_state=3;
                config_dirty=true;
                volume_save_after=(uint64_t)esp_timer_get_time()/1000+2000;
            }
            if(automatic)volume_save_pending=false;
            portEXIT_CRITICAL(&mutex);
            if(error!=ESP_OK)ESP_LOGW("device","Settings save failed: %s",esp_err_to_name(error));
        } else if(request.command==6) {
            uint64_t now=(uint64_t)esp_timer_get_time()/1000;
            portENTER_CRITICAL(&mutex);device_config_t latest=config;
            bool fresh=now>=volume_changed_at && now-volume_changed_at<=800;
            preview_requested=false;portEXIT_CRITICAL(&mutex);
            unsigned tone=latest.done_tone?latest.done_tone:latest.quota_tone?latest.quota_tone:1;
            atomic_store(&preview_playback,true);
            if(fresh && latest.volume && !play_tone(tone,latest.volume)){ESP_LOGW("device","Volume preview failed");(void)bsp_audio_sleep();}
            atomic_store(&preview_playback,false);
            portENTER_CRITICAL(&mutex);preview_pending=false;portEXIT_CRITICAL(&mutex);
        } else if(!play_tone(request.tone,request.volume)) {
            ESP_LOGW("device","Tone playback failed");(void)bsp_audio_sleep();
        }
        if(request.command==3)atomic_store(&boot_finished,true);
    }
}
bool device_runtime_start(void) {
    atomic_store(&boot_finished,false);
    volume_revision=0;volume_unsaved=volume_save_pending=preview_requested=preview_pending=false;
    volume_changed_at=volume_save_after=preview_next=0;atomic_store(&preview_playback,false);
    config=device_config_default();pages=page_config_default();monitor=monitor_default();brightness=75;config_dirty=false;behavior.last_touch=(uint64_t)esp_timer_get_time()/1000;
    nvs_ready=nvs_flash_init()==ESP_OK;
    if(nvs_ready) {
        nvs_handle_t handle;
        if(nvs_open("tokens_ui",NVS_READONLY,&handle)==ESP_OK) {
            unsigned saved_brightness;size_t display_size=sizeof(saved_brightness);
            if(nvs_get_blob(handle,"display1",&saved_brightness,&display_size)==ESP_OK&&display_size==sizeof(saved_brightness)&&saved_brightness>=5&&saved_brightness<=100)brightness=saved_brightness;
            device_config_t stored;size_t size=sizeof(stored);
            bool found=nvs_get_blob(handle,"cfg2",&stored,&size)==ESP_OK && size==sizeof(stored) && device_config_valid(&stored);
            if(!found) {
                uint32_t legacy[4];size=sizeof(legacy);
                if(nvs_get_blob(handle,"cfg1",legacy,&size)==ESP_OK && size==sizeof(legacy)) {
                    stored=(device_config_t){legacy[0],legacy[1],legacy[2],legacy[3],1};
                    device_config_upgrade(&stored);found=device_config_valid(&stored);config_dirty=found;
                }
            }
            if(found) {
                device_config_t previous=stored;device_config_upgrade(&stored);
                if(device_config_valid(&stored)) {config=stored;config_dirty|=memcmp(&previous,&stored,sizeof(stored))!=0;}
            }
            page_config_t saved_pages;size=sizeof(saved_pages);
            if(nvs_get_blob(handle,"pages1",&saved_pages,&size)==ESP_OK && size==sizeof(saved_pages) && page_config_valid(&saved_pages))pages=saved_pages;
            monitor_config_t saved_monitor;size=sizeof(saved_monitor);
            if(nvs_get_blob(handle,"mon1",&saved_monitor,&size)==ESP_OK && size==sizeof(saved_monitor) && monitor_valid(&saved_monitor))monitor=saved_monitor;
            nvs_close(handle);
        }
    }
    atomic_store(&live_volume,config.volume);
    queue=xQueueCreate(4,sizeof(request_t));
    if(queue) {
        request_t startup={.command=3,.tone=config.startup_sound?8:0,.volume=config.volume};
        if(xQueueSend(queue,&startup,0)==pdTRUE && xTaskCreate(worker,"device_io",4096,NULL,4,NULL)==pdPASS)return true;
        vQueueDelete(queue);queue=NULL;
    }
    atomic_store(&boot_finished,true);return false;
}
unsigned device_runtime_volume(void) {
    portENTER_CRITICAL(&mutex);unsigned current=config.volume;portEXIT_CRITICAL(&mutex);return current;
}
void device_runtime_volume_set(unsigned value,uint64_t now) {
    if(value>100)value=100;
    portENTER_CRITICAL(&mutex);
    if(config.volume!=value) {
        config.volume=value;++volume_revision;config_dirty=true;volume_unsaved=true;
        volume_changed_at=now;volume_save_after=now+1000;preview_requested=value>0;
        atomic_store(&live_volume,value);
    }
    portEXIT_CRITICAL(&mutex);
}
monitor_config_t device_runtime_monitor(void){portENTER_CRITICAL(&mutex);monitor_config_t current=monitor;portEXIT_CRITICAL(&mutex);return current;}
page_config_t device_runtime_pages(void) {
    portENTER_CRITICAL(&mutex);page_config_t current=pages;portEXIT_CRITICAL(&mutex);return current;
}
bool device_runtime_booting(void) {return !atomic_load(&boot_finished);}
static bool notify(bool done) {
    portENTER_CRITICAL(&mutex);device_config_t current=config;portEXIT_CRITICAL(&mutex);
    request_t request={.command=2,.tone=done?current.done_tone:current.quota_tone,.volume=current.volume};
    return !request.tone || !request.volume || (queue && xQueueSend(queue,&request,0)==pdTRUE);
}
bool device_runtime_key(uint64_t now) {
    bool was_off=device_key(&behavior,now);if(was_off)bsp_display_backlight(device_runtime_brightness());return was_off;
}
unsigned device_runtime_brightness(void){portENTER_CRITICAL(&mutex);unsigned value=brightness;portEXIT_CRITICAL(&mutex);return value;}
void device_runtime_tick(uint64_t now,const token_snapshot_t *snapshot) {
    portENTER_CRITICAL(&mutex);device_config_t current=config;portEXIT_CRITICAL(&mutex);
    bool previous=behavior.off;
    if(atomic_exchange(&wake_requested,false)) (void)device_key(&behavior,now);
    bool off=device_screen_off(&behavior,&current,now);
    static unsigned applied=101;unsigned level=off?0:device_runtime_brightness();
    if(previous!=off||level!=applied){bsp_display_backlight(level);applied=level;}
    portENTER_CRITICAL(&mutex);
    bool save=queue && nvs_ready && volume_unsaved && !volume_save_pending && save_state!=1 && now>=volume_save_after;
    bool preview=queue && preview_requested && !preview_pending && current.volume && now>=preview_next && now>=volume_changed_at && now-volume_changed_at<=800;
    if(save)volume_save_pending=true;
    if(preview){preview_pending=true;preview_next=now+1200;}
    portEXIT_CRITICAL(&mutex);
    if(save){request_t request={.command=5};if(xQueueSend(queue,&request,0)!=pdTRUE){portENTER_CRITICAL(&mutex);volume_save_pending=false;portEXIT_CRITICAL(&mutex);}}
    if(preview){request_t request={.command=6};if(xQueueSend(queue,&request,0)!=pdTRUE){portENTER_CRITICAL(&mutex);preview_pending=false;portEXIT_CRITICAL(&mutex);}}
    unsigned exhausted=behavior.exhausted;
    if(snapshot && device_quota_alert(&behavior,snapshot) && !notify(false))behavior.exhausted=exhausted;
}
/* Reject signed numbers and overflow rather than allowing scanf to wrap. */
static bool numbers(const char *line,const char *prefix,unsigned *out,unsigned count) {
    size_t length=strlen(prefix);if(strncmp(line,prefix,length))return false;
    const char *p=line+length;
    for(unsigned i=0;i<count;++i) {
        if(*p++!=' ' || *p<'0' || *p>'9')return false;
        uint64_t value=0;
        do {value=value*10+(unsigned)(*p++-'0');if(value>UINT32_MAX)return false;}while(*p>='0' && *p<='9');
        out[i]=(unsigned)value;
    }
    return !*p;
}
size_t device_runtime_line(const char *line,char *reply,size_t capacity) {
    if(strncmp(line,"PC_DISPLAY",10) && strncmp(line,"PC_CFG",6) && strncmp(line,"PC_SOUND",8) && strncmp(line,"PC_EVT",6) && strncmp(line,"PC_PAGES",8) && strncmp(line,"PC_MON",6))return 0;
    if(!queue)return snprintf(reply,capacity,"PC_CFG_UNAVAILABLE\n");
    if(!strcmp(line,"PC_SOUND_BANKS"))return snprintf(reply,capacity,"PC_SOUND_BANKS 3\n");
    if(!strcmp(line,"PC_CFG_GET2")) {
        portENTER_CRITICAL(&mutex);device_config_t c=config;portEXIT_CRITICAL(&mutex);
        return snprintf(reply,capacity,"PC_CFG 2 %lu %lu %lu %lu %lu\n",(unsigned long)c.idle_seconds,
            (unsigned long)c.done_tone,(unsigned long)c.quota_tone,(unsigned long)c.volume,(unsigned long)c.startup_sound);
    }
    if(!strcmp(line,"PC_CFG_GET")) {
        portENTER_CRITICAL(&mutex);device_config_t c=config;portEXIT_CRITICAL(&mutex);
        return snprintf(reply,capacity,"PC_CFG 1 %lu %lu %lu %lu\n",(unsigned long)c.idle_seconds,
            (unsigned long)c.done_tone,(unsigned long)c.quota_tone,(unsigned long)c.volume);
    }
    if(!strcmp(line,"PC_CFG_STATUS")) {
        portENTER_CRITICAL(&mutex);unsigned id=save_id,state=save_state;portEXIT_CRITICAL(&mutex);
        return snprintf(reply,capacity,"PC_CFG_STATE %u %u\n",id,state);
    }
    if(!strcmp(line,"PC_PAGES_GET")) {
        page_config_t c=device_runtime_pages();
        return snprintf(reply,capacity,"PC_PAGES 1 %lu %lu %lu %lu %lu\n",(unsigned long)c.enabled,
            (unsigned long)c.order[0],(unsigned long)c.order[1],(unsigned long)c.order[2],(unsigned long)c.order[3]);
    }
    if(!strcmp(line,"PC_MON_GET")) {
        monitor_config_t c=device_runtime_monitor();return snprintf(reply,capacity,"PC_MON 1 %lu\n",(unsigned long)monitor_hash(&c));
    }
    unsigned read_offset[1];
    if(numbers(line,"PC_MON_READ",read_offset,1) && read_offset[0]<MONITOR_CONFIG_BYTES) {
        monitor_config_t c=device_runtime_monitor();char hex[MONITOR_CONFIG_BYTES*2+1];monitor_encode(&c,hex);
        unsigned start=read_offset[0],count=MONITOR_CONFIG_BYTES-start;if(count>16)count=16;
        return snprintf(reply,capacity,"PC_MON_DATA %lu %u %.*s\n",(unsigned long)monitor_hash(&c),start,(int)(count*2),hex+start*2);
    }
    if(!strncmp(line,"PC_MON_SET ",11)) {
        const char *p=line+11;uint64_t number=0;
        if(*p<'0'||*p>'9')return snprintf(reply,capacity,"PC_CFG_INVALID\n");
        do{number=number*10+(unsigned)(*p++-'0');if(number>UINT32_MAX)return snprintf(reply,capacity,"PC_CFG_INVALID\n");}while(*p>='0'&&*p<='9');
        request_t request={.command=7,.id=(unsigned)number};
        if(!number || *p++!=' ' || !monitor_decode(p,&request.monitor))return snprintf(reply,capacity,"PC_CFG_INVALID\n");
        if(!nvs_ready)return snprintf(reply,capacity,"PC_CFG_UNAVAILABLE\n");
        portENTER_CRITICAL(&mutex);unsigned state=save_state,old_id=save_id,old_kind=save_kind;
        monitor_config_t previous=requested_monitor;
        bool duplicate=save_kind==7 && request.id==save_id && !memcmp(&requested_monitor,&request.monitor,sizeof(request.monitor));
        bool accept=!duplicate && state!=1 && request.id!=save_id;
        if(accept){save_kind=7;save_id=request.id;save_state=1;requested_monitor=request.monitor;}
        portEXIT_CRITICAL(&mutex);
        if(!accept && !duplicate)return snprintf(reply,capacity,"PC_CFG_BUSY\n");
        if(accept && xQueueSend(queue,&request,0)!=pdTRUE){portENTER_CRITICAL(&mutex);save_id=old_id;save_kind=old_kind;save_state=state;requested_monitor=previous;portEXIT_CRITICAL(&mutex);return snprintf(reply,capacity,"PC_CFG_RETRY\n");}
        return snprintf(reply,capacity,"PC_CFG_STATE %u %u\n",request.id,duplicate?state:1);
    }
    unsigned values[6];
    if(!strcmp(line,"PC_DISPLAY_GET"))return snprintf(reply,capacity,"PC_DISPLAY 1 %u\n",device_runtime_brightness());
    if(numbers(line,"PC_DISPLAY_SET",values,2)) {
        unsigned id=values[0],value=values[1];if(!id||value<5||value>100)return snprintf(reply,capacity,"PC_CFG_INVALID\n");
        if(!nvs_ready)return snprintf(reply,capacity,"PC_CFG_UNAVAILABLE\n");
        portENTER_CRITICAL(&mutex);unsigned old_id=save_id,old_state=save_state,old_kind=save_kind,old_value=requested_brightness;
        bool duplicate=save_kind==11&&id==save_id&&value==requested_brightness;
        bool accept=!duplicate&&save_state!=1&&id!=save_id;
        if(accept){save_id=id;save_state=1;save_kind=11;requested_brightness=value;}portEXIT_CRITICAL(&mutex);
        if(!accept&&!duplicate)return snprintf(reply,capacity,"PC_CFG_BUSY\n");
        request_t request={.command=11,.id=id,.volume=value};
        if(accept&&xQueueSend(queue,&request,0)!=pdTRUE){portENTER_CRITICAL(&mutex);save_id=old_id;save_state=old_state;save_kind=old_kind;requested_brightness=old_value;portEXIT_CRITICAL(&mutex);return snprintf(reply,capacity,"PC_CFG_RETRY\n");}
        return snprintf(reply,capacity,"PC_CFG_STATE %u %u\n",id,duplicate?old_state:1);
    }
    if(numbers(line,"PC_PAGES_SET",values,6)) {
        request_t request={.command=4,.id=values[0],.pages={values[1],{values[2],values[3],values[4],values[5]}}};
        if(!request.id || !page_config_valid(&request.pages))return snprintf(reply,capacity,"PC_CFG_INVALID\n");
        if(!nvs_ready)return snprintf(reply,capacity,"PC_CFG_UNAVAILABLE\n");
        portENTER_CRITICAL(&mutex);
        unsigned state=save_state,old_id=save_id,old_kind=save_kind;
        page_config_t previous=requested_pages;
        bool duplicate=save_kind==4 && request.id==save_id && !memcmp(&requested_pages,&request.pages,sizeof(request.pages));
        bool accept=!duplicate && state!=1 && request.id!=save_id;
        if(accept) {save_kind=4;save_id=request.id;save_state=1;requested_pages=request.pages;}
        portEXIT_CRITICAL(&mutex);
        if(!accept && !duplicate)return snprintf(reply,capacity,"PC_CFG_BUSY\n");
        if(accept && xQueueSend(queue,&request,0)!=pdTRUE) {
            portENTER_CRITICAL(&mutex);save_id=old_id;save_kind=old_kind;save_state=state;requested_pages=previous;portEXIT_CRITICAL(&mutex);
            return snprintf(reply,capacity,"PC_CFG_RETRY\n");
        }
        return snprintf(reply,capacity,"PC_CFG_STATE %u %u\n",request.id,duplicate?state:1);
    }
    bool extended=numbers(line,"PC_CFG_SET2",values,6);
    if(extended || numbers(line,"PC_CFG_SET",values,5)) {
        unsigned id=values[0],timeout=values[1],done=values[2],quota=values[3],volume=values[4];
        portENTER_CRITICAL(&mutex);unsigned startup=config.startup_sound;portEXIT_CRITICAL(&mutex);
        request_t request={.command=1,.id=id,.config={timeout,done,quota,volume,extended?values[5]:startup}};
        if(!id || !device_config_valid(&request.config))return snprintf(reply,capacity,"PC_CFG_INVALID\n");
        if(!nvs_ready)return snprintf(reply,capacity,"PC_CFG_UNAVAILABLE\n");
        portENTER_CRITICAL(&mutex);
        request.volume_revision=volume_revision;
        unsigned state=save_state;
        bool duplicate=save_kind==1 && id==save_id && !memcmp(&requested,&request.config,sizeof(requested));
        bool accept=!duplicate && state!=1 && id!=save_id;
        unsigned old_id=save_id,old_kind=save_kind;device_config_t old_requested=requested;
        if(accept) {save_kind=1;save_id=id;save_state=1;requested=request.config;}
        portEXIT_CRITICAL(&mutex);
        if(!accept && !duplicate)return snprintf(reply,capacity,"PC_CFG_BUSY\n");
        if(accept && xQueueSend(queue,&request,0)!=pdTRUE) {
            portENTER_CRITICAL(&mutex);save_id=old_id;save_kind=old_kind;save_state=state;requested=old_requested;portEXIT_CRITICAL(&mutex);
            return snprintf(reply,capacity,"PC_CFG_RETRY\n");
        }
        return snprintf(reply,capacity,"PC_CFG_STATE %u %u\n",id,duplicate?state:1);
    }
    if(numbers(line,"PC_SOUND",values,2) && (values[0]==1 || (values[0]>=3 && values[0]<=7)) && values[1]<=100) {
        unsigned tone=values[0],volume=values[1];
        request_t request={.command=2,.tone=tone,.volume=volume};
        if(xQueueSend(queue,&request,0)!=pdTRUE)return snprintf(reply,capacity,"PC_CFG_RETRY\n");
        atomic_store(&wake_requested,true);return snprintf(reply,capacity,"PC_SOUND_QUEUED\n");
    }
    if(numbers(line,"PC_EVT",values,3) && values[0] && values[1] && values[2]==1) {
        unsigned host=values[0],sequence=values[1];
        portENTER_CRITICAL(&mutex);
        bool fresh=!event_seen || event_host!=host || sequence>event_sequence;
        bool busy=event_pending;
        if(fresh && !busy)event_pending=true;
        portEXIT_CRITICAL(&mutex);
        if(busy)return snprintf(reply,capacity,"PC_CFG_RETRY\n");
        if(fresh) {
            bool queued=notify(true);
            portENTER_CRITICAL(&mutex);
            if(queued){event_seen=true;event_host=host;event_sequence=sequence;}
            event_pending=false;portEXIT_CRITICAL(&mutex);
            if(!queued)return snprintf(reply,capacity,"PC_CFG_RETRY\n");
        }
        return snprintf(reply,capacity,"PC_EVT_ACK %u %u\n",host,sequence);
    }
    return snprintf(reply,capacity,"PC_CFG_INVALID\n");
}

bool device_runtime_test(unsigned id,unsigned kind) {
    if(!queue || kind<2 || kind>3)return false;
    portENTER_CRITICAL(&mutex);device_config_t current=config;portEXIT_CRITICAL(&mutex);
    request_t request={.id=id,.command=kind==2?9:10,.volume=current.volume,.tone=current.done_tone?current.done_tone:current.quota_tone?current.quota_tone:1};
    return xQueueSend(queue,&request,0)==pdTRUE;
}
