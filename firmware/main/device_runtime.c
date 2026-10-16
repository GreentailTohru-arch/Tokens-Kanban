#include "device_runtime.h"
#include "bsp_audio.h"
#include "bsp_display.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>
typedef struct { unsigned command,id,tone,volume; device_config_t config; } request_t;
static QueueHandle_t queue;
static device_config_t config,requested;
static device_behavior_t behavior;
static bool nvs_ready,event_seen,event_pending;
static unsigned save_id,save_state,event_host,event_sequence;
static atomic_bool wake_requested;
static portMUX_TYPE mutex=portMUX_INITIALIZER_UNLOCKED;

static bool play_tone(unsigned tone,unsigned volume) {
    if(!tone || !volume) return true;
    static bool initialized;
    if(!initialized) { if(bsp_audio_init()!=ESP_OK) return false;initialized=true; }
    if(bsp_audio_wake()!=ESP_OK || bsp_audio_set_format(16000,16,1)!=ESP_OK) return false;
    bsp_audio_set_volume((uint8_t)volume);
    /* Short generated PCM in 20ms chunks, with attack/release to avoid clicks. */
    const unsigned frequency[3][2]={{880,0},{660,880},{392,294}};
    unsigned notes=tone==1?1:2;int16_t pcm[320];
    for(unsigned note=0;note<notes;++note) {
        unsigned count=tone==3?2560:2080;
        for(unsigned offset=0;offset<count;offset+=320) {
            unsigned n=count-offset;if(n>320)n=320;
            for(unsigned i=0;i<n;++i) {
                unsigned pos=offset+i,fade=pos<count-pos?pos:count-pos;
                float envelope=fade<192?fade/192.0f:1.0f;
                pcm[i]=(int16_t)(6000.0f*envelope*sinf(6.28318530718f*frequency[tone-1][note]*pos/16000.0f));
            }
            if(bsp_audio_write(pcm,n*sizeof(*pcm))!=ESP_OK) return false;
        }
        memset(pcm,0,sizeof(pcm));if(bsp_audio_write(pcm,sizeof(pcm))!=ESP_OK)return false;
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
        if(request.command==1) {
            portENTER_CRITICAL(&mutex);bool same=!memcmp(&config,&request.config,sizeof(config));portEXIT_CRITICAL(&mutex);
            nvs_handle_t handle;esp_err_t error=same?ESP_OK:nvs_open("tokens_ui",NVS_READWRITE,&handle);
            if(error==ESP_OK && !same) {
                error=nvs_set_blob(handle,"cfg1",&request.config,sizeof(request.config));
                if(error==ESP_OK) error=nvs_commit(handle);
                nvs_close(handle);
            }
            portENTER_CRITICAL(&mutex);
            if(error==ESP_OK) {config=request.config;save_state=2;atomic_store(&wake_requested,true);}
            else save_state=3;
            portEXIT_CRITICAL(&mutex);
            if(error!=ESP_OK) ESP_LOGW("device","Settings save failed: %s",esp_err_to_name(error));
        } else if(!play_tone(request.tone,request.volume)) {
            ESP_LOGW("device","Tone playback failed");
            (void)bsp_audio_sleep();
        }
    }
}
bool device_runtime_start(void) {
    config=device_config_default();behavior.last_touch=(uint64_t)esp_timer_get_time()/1000;
    nvs_ready=nvs_flash_init()==ESP_OK;
    if(nvs_ready) {
        nvs_handle_t handle;
        if(nvs_open("tokens_ui",NVS_READONLY,&handle)==ESP_OK) {
            device_config_t stored;size_t size=sizeof(stored);
            if(nvs_get_blob(handle,"cfg1",&stored,&size)==ESP_OK && size==sizeof(stored) && device_config_valid(&stored)) config=stored;
            nvs_close(handle);
        }
    }
    queue=xQueueCreate(4,sizeof(request_t));if(!queue)return false;
    if(xTaskCreate(worker,"device_io",4096,NULL,4,NULL)==pdPASS)return true;
    vQueueDelete(queue);queue=NULL;return false;
}
static bool notify(bool done) {
    portENTER_CRITICAL(&mutex);device_config_t current=config;portEXIT_CRITICAL(&mutex);
    request_t request={.command=2,.tone=done?current.done_tone:current.quota_tone,.volume=current.volume};
    return !request.tone || !request.volume || (queue && xQueueSend(queue,&request,0)==pdTRUE);
}
bool device_runtime_key(uint64_t now) {
    bool was_off=device_key(&behavior,now);if(was_off)bsp_display_backlight(75);return was_off;
}
void device_runtime_tick(uint64_t now,const token_snapshot_t *snapshot) {
    portENTER_CRITICAL(&mutex);device_config_t current=config;portEXIT_CRITICAL(&mutex);
    bool previous=behavior.off;
    if(atomic_exchange(&wake_requested,false)) (void)device_key(&behavior,now);
    bool off=device_screen_off(&behavior,&current,now);
    if(previous!=off)bsp_display_backlight(off?0:75);
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
    if(strncmp(line,"PC_CFG",6) && strncmp(line,"PC_SOUND",8) && strncmp(line,"PC_EVT",6))return 0;
    if(!queue)return snprintf(reply,capacity,"PC_CFG_UNAVAILABLE\n");
    if(!strcmp(line,"PC_CFG_GET")) {
        portENTER_CRITICAL(&mutex);device_config_t c=config;portEXIT_CRITICAL(&mutex);
        return snprintf(reply,capacity,"PC_CFG 1 %lu %lu %lu %lu\n",(unsigned long)c.idle_seconds,
            (unsigned long)c.done_tone,(unsigned long)c.quota_tone,(unsigned long)c.volume);
    }
    if(!strcmp(line,"PC_CFG_STATUS")) {
        portENTER_CRITICAL(&mutex);unsigned id=save_id,state=save_state;portEXIT_CRITICAL(&mutex);
        return snprintf(reply,capacity,"PC_CFG_STATE %u %u\n",id,state);
    }
    unsigned values[5];
    if(numbers(line,"PC_CFG_SET",values,5)) {
        unsigned id=values[0],timeout=values[1],done=values[2],quota=values[3],volume=values[4];
        request_t request={.command=1,.id=id,.config={timeout,done,quota,volume}};
        if(!id || !device_config_valid(&request.config))return snprintf(reply,capacity,"PC_CFG_INVALID\n");
        if(!nvs_ready)return snprintf(reply,capacity,"PC_CFG_UNAVAILABLE\n");
        portENTER_CRITICAL(&mutex);
        unsigned state=save_state;
        bool duplicate=id==save_id && !memcmp(&requested,&request.config,sizeof(requested));
        bool accept=!duplicate && state!=1 && id!=save_id;
        unsigned old_id=save_id;device_config_t old_requested=requested;
        if(accept) {save_id=id;save_state=1;requested=request.config;}
        portEXIT_CRITICAL(&mutex);
        if(!accept && !duplicate)return snprintf(reply,capacity,"PC_CFG_BUSY\n");
        if(accept && xQueueSend(queue,&request,0)!=pdTRUE) {
            portENTER_CRITICAL(&mutex);save_id=old_id;save_state=state;requested=old_requested;portEXIT_CRITICAL(&mutex);
            return snprintf(reply,capacity,"PC_CFG_RETRY\n");
        }
        return snprintf(reply,capacity,"PC_CFG_STATE %u %u\n",id,duplicate?state:1);
    }
    if(numbers(line,"PC_SOUND",values,2) && values[0]>=1 && values[0]<=3 && values[1]<=100) {
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
