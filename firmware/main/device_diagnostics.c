#include "device_diagnostics.h"
#include "diagnostic_model.h"
#include "device_runtime.h"
#include "token_ble.h"
#include "bsp_battery.h"
#include "bsp_button.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "esp_chip_info.h"
#include "esp_mac.h"
#include "esp_flash.h"
#include "esp_partition.h"
#include "esp_app_desc.h"
#include "esp_heap_caps.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>
typedef struct {unsigned id,kind;char name[DEVICE_NAME_BYTES+1];} diagnostic_request_t;
typedef struct {
    int cpu,mhz,soc,mv,adc;uint64_t uptime;
    unsigned heap_total,heap_free,heap_min,heap_largest,flash,app,postcard;int nvs_used,nvs_free;
} diagnostic_snapshot_t;
static diagnostic_snapshot_t snapshot={.cpu=-1,.soc=-1,.mv=-1,.adc=-1,.nvs_used=-1,.nvs_free=-1};
static char device_name[DEVICE_NAME_BYTES+1]=DEVICE_DEFAULT_NAME,requested_name[DEVICE_NAME_BYTES+1];
static QueueHandle_t diagnostic_queue;
static bool has_battery,has_buttons;
static unsigned name_id,name_state,test_id,test_state,test_kind,test_samples;
static int test_peak=-1,test_average=-1,screen_index=-1;
static uint64_t screen_until;
static atomic_uint key_count[3],last_key,last_event;
static portMUX_TYPE diagnostic_mutex=portMUX_INITIALIZER_UNLOCKED;
static void sample(void) {
    static uint64_t previous_time,previous_idle;
    uint64_t idle=ulTaskGetIdleRunTimeCounter(),now=(uint64_t)esp_timer_get_time();
    diagnostic_snapshot_t s={.cpu=diagnostic_cpu_load(previous_time,previous_idle,now,idle),.mhz=(int)esp_rom_get_cpu_ticks_per_us(),.uptime=now/1000,.soc=-1,.mv=-1,.adc=-1,.nvs_used=-1,.nvs_free=-1};previous_time=now;previous_idle=idle;
    s.heap_total=heap_caps_get_total_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);s.heap_free=heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);s.heap_min=heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);s.heap_largest=heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
    uint32_t bytes=0;if(esp_flash_get_size(NULL,&bytes)==ESP_OK)s.flash=bytes;
    const esp_partition_t *part=esp_partition_find_first(ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_ANY,NULL);if(part)s.app=part->size;
    part=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,0x40,"postcard");if(part)s.postcard=part->size;
    nvs_stats_t stats;if(nvs_get_stats(NULL,&stats)==ESP_OK){s.nvs_used=stats.used_entries;s.nvs_free=stats.free_entries;}
    if(has_battery){s.soc=bsp_battery_soc();s.mv=bsp_battery_mv();}if(has_buttons)s.adc=bsp_button_read_mv();
    portENTER_CRITICAL(&diagnostic_mutex);snapshot=s;portEXIT_CRITICAL(&diagnostic_mutex);
}
static void diagnostic_worker(void *arg) {
    (void)arg;diagnostic_request_t request;
    for(;;){
        bool received=xQueueReceive(diagnostic_queue,&request,pdMS_TO_TICKS(1000))==pdTRUE;
        if(received&&request.kind==1){
            nvs_handle_t handle;esp_err_t error=nvs_open("tokens_ui",NVS_READWRITE,&handle);
            if(error==ESP_OK){error=nvs_set_str(handle,"name1",request.name);if(error==ESP_OK)error=nvs_commit(handle);nvs_close(handle);}
            portENTER_CRITICAL(&diagnostic_mutex);if(error==ESP_OK)memcpy(device_name,request.name,sizeof(device_name));name_state=error==ESP_OK?2:3;portEXIT_CRITICAL(&diagnostic_mutex);
            if(error==ESP_OK)token_ble_name_changed();
        }
        sample();
        if(received&&request.kind==2){portENTER_CRITICAL(&diagnostic_mutex);bool ok=snapshot.soc>=0&&snapshot.mv>=0;portEXIT_CRITICAL(&diagnostic_mutex);device_diagnostics_test_complete(request.id,ok,-1,-1,0);}
    }
}
bool device_diagnostics_start(bool battery_ready,bool buttons_ready) {
    has_battery=battery_ready;has_buttons=buttons_ready;
    nvs_handle_t handle;if(nvs_open("tokens_ui",NVS_READONLY,&handle)==ESP_OK){char text[DEVICE_NAME_BYTES+1];size_t n=sizeof(text);if(nvs_get_str(handle,"name1",text,&n)==ESP_OK&&diagnostic_name_valid(text))memcpy(device_name,text,n);nvs_close(handle);}
    diagnostic_queue=xQueueCreate(2,sizeof(diagnostic_request_t));if(!diagnostic_queue)return false;
    if(xTaskCreate(diagnostic_worker,"device_info",4096,NULL,2,NULL)!=pdPASS){vQueueDelete(diagnostic_queue);diagnostic_queue=NULL;return false;}return true;
}
void device_diagnostics_name(char *out,size_t capacity) {
    portENTER_CRITICAL(&diagnostic_mutex);if(capacity)snprintf(out,capacity,"%s",device_name);portEXIT_CRITICAL(&diagnostic_mutex);
}
void device_diagnostics_key(unsigned key,unsigned event){if(key>=3)return;if(event==BSP_BTN_PRESS)atomic_fetch_add(&key_count[key],1);atomic_store(&last_key,key);atomic_store(&last_event,event);}
void device_diagnostics_test_complete(unsigned id,bool ok,int peak,int average,unsigned samples){portENTER_CRITICAL(&diagnostic_mutex);if(test_id==id&&test_state==1){test_state=ok?2:3;test_peak=peak;test_average=average;test_samples=samples;}portEXIT_CRITICAL(&diagnostic_mutex);}
int device_diagnostics_screen(uint64_t now){portENTER_CRITICAL(&diagnostic_mutex);if(screen_index>=0&&now>=screen_until)screen_index=-1;int value=screen_index;portEXIT_CRITICAL(&diagnostic_mutex);return value;}
bool device_diagnostics_screen_stop(void){portENTER_CRITICAL(&diagnostic_mutex);bool active=screen_index>=0;screen_index=-1;portEXIT_CRITICAL(&diagnostic_mutex);return active;}
static bool number(const char **text,unsigned *value){const char *p=*text;if(*p<'0'||*p>'9')return false;uint64_t n=0;do{n=n*10+(unsigned)(*p++-'0');if(n>2147483647)return false;}while(*p>='0'&&*p<='9');*value=(unsigned)n;*text=p;return true;}
size_t device_diagnostics_line(const char *line,char *reply,size_t cap) {
    if(strncmp(line,"PC_INFO",7)&&strncmp(line,"PC_NAME",7)&&strncmp(line,"PC_TEST",7)&&strncmp(line,"PC_SCREEN",9))return 0;
    if(!diagnostic_queue)return snprintf(reply,cap,"PC_INFO_UNAVAILABLE\n");
    portENTER_CRITICAL(&diagnostic_mutex);diagnostic_snapshot_t s=snapshot;char name[DEVICE_NAME_BYTES+1];memcpy(name,device_name,sizeof(name));unsigned ni=name_id,ns=name_state,ti=test_id,ts=test_state,tk=test_kind,tc=test_samples;int tp=test_peak,ta=test_average;portEXIT_CRITICAL(&diagnostic_mutex);
    if(!strcmp(line,"PC_INFO"))return snprintf(reply,cap,"PC_INFO 1\n");
    if(!strcmp(line,"PC_NAME_BLE"))return snprintf(reply,cap,"PC_NAME_BLE 1 %u\n",DEVICE_BLE_NAME_BYTES);
    if(!strcmp(line,"PC_INFO_SYSTEM"))return snprintf(reply,cap,"PC_SYSTEM %llu %d %d %u %u %u %u\n",(unsigned long long)s.uptime,s.cpu,s.mhz,s.heap_total,s.heap_free,s.heap_min,s.heap_largest);
    if(!strcmp(line,"PC_INFO_STORAGE"))return snprintf(reply,cap,"PC_STORAGE %u %u %u %d %d\n",s.flash,s.app,s.postcard,s.nvs_used,s.nvs_free);
    if(!strcmp(line,"PC_INFO_BATTERY"))return snprintf(reply,cap,"PC_BATTERY %d %d 500\n",s.soc,s.mv);
    if(!strcmp(line,"PC_INFO_NAME")){char hex[DEVICE_NAME_BYTES*2+1];diagnostic_name_encode(name,hex);return snprintf(reply,cap,"PC_NAME %s\n",hex);}
    if(!strcmp(line,"PC_INFO_KEYS"))return snprintf(reply,cap,"PC_KEYS %d %u %u %u %u %u\n",s.adc,atomic_load(&key_count[0]),atomic_load(&key_count[1]),atomic_load(&key_count[2]),atomic_load(&last_key),atomic_load(&last_event));
    if(!strcmp(line,"PC_INFO_CHIP")){uint8_t mac[6];esp_chip_info_t chip;esp_chip_info(&chip);if(esp_read_mac(mac,ESP_MAC_BASE)!=ESP_OK)memset(mac,0,sizeof(mac));char elf[65];esp_app_get_elf_sha256(elf,sizeof(elf));return snprintf(reply,cap,"PC_CHIP ESP32-C3 %u %u %02x%02x%02x%02x%02x%02x %.12s v32\n",chip.cores,chip.revision,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],elf);}
    if(!strcmp(line,"PC_NAME_STATE"))return snprintf(reply,cap,"PC_NAME_STATE %u %u\n",ni,ns);
    if(!strcmp(line,"PC_TEST_STATE"))return snprintf(reply,cap,"PC_TEST_STATE %u %u %u %d %d %u\n",ti,ts,tk,tp,ta,tc);
    if(!strncmp(line,"PC_NAME_SET ",12)){
        const char *p=line+12;diagnostic_request_t r={.kind=1};if(!number(&p,&r.id)||!r.id||*p++!=' '||!diagnostic_name_decode(p,r.name))return snprintf(reply,cap,"PC_NAME_INVALID\n");
        portENTER_CRITICAL(&diagnostic_mutex);bool duplicate=r.id==name_id&&!strcmp(r.name,requested_name);bool accept=!duplicate&&name_state!=1&&r.id!=name_id;
        if(accept){name_id=r.id;name_state=1;memcpy(requested_name,r.name,sizeof(requested_name));}portEXIT_CRITICAL(&diagnostic_mutex);
        if(duplicate)return snprintf(reply,cap,"PC_NAME_STATE %u %u\n",ni,ns);
        if(!accept)return snprintf(reply,cap,"PC_NAME_BUSY\n");
        if(xQueueSend(diagnostic_queue,&r,0)!=pdTRUE){portENTER_CRITICAL(&diagnostic_mutex);name_state=3;portEXIT_CRITICAL(&diagnostic_mutex);return snprintf(reply,cap,"PC_NAME_STATE %u 3\n",r.id);}
        return snprintf(reply,cap,"PC_NAME_STATE %u 1\n",r.id);
    }
    if(!strncmp(line,"PC_TEST ",8)){
        const char *p=line+8;unsigned id,kind;if(!number(&p,&id)||!id||*p++!=' '||!number(&p,&kind)||*p||kind<1||kind>3)return snprintf(reply,cap,"PC_TEST_INVALID\n");
        portENTER_CRITICAL(&diagnostic_mutex);bool duplicate=test_id==id&&test_kind==kind;bool accept=!duplicate&&test_state!=1&&test_id!=id;
        if(!accept){portEXIT_CRITICAL(&diagnostic_mutex);if(duplicate)return snprintf(reply,cap,"PC_TEST_STATE %u %u %u %d %d %u\n",ti,ts,tk,tp,ta,tc);return snprintf(reply,cap,"PC_TEST_BUSY\n");}
        test_id=id;test_kind=kind;test_state=1;test_peak=test_average=-1;test_samples=0;portEXIT_CRITICAL(&diagnostic_mutex);
        bool accepted;if(kind==1){diagnostic_request_t r={.id=id,.kind=2};accepted=xQueueSend(diagnostic_queue,&r,0)==pdTRUE;}else accepted=device_runtime_test(id,kind);
        if(!accepted)device_diagnostics_test_complete(id,false,-1,-1,0);
        return snprintf(reply,cap,"PC_TEST_STATE %u %u %u -1 -1 0\n",id,accepted?1:3,kind);
    }
    if(!strncmp(line,"PC_SCREEN ",10)){const char *p=line+10;int value=-1;if(strcmp(p,"-1")){unsigned n;if(!number(&p,&n)||*p||n>6)return snprintf(reply,cap,"PC_SCREEN_INVALID\n");value=(int)n;}
        portENTER_CRITICAL(&diagnostic_mutex);screen_index=value;screen_until=(uint64_t)esp_timer_get_time()/1000+15000;portEXIT_CRITICAL(&diagnostic_mutex);return snprintf(reply,cap,"PC_SCREEN_OK\n");}
    return snprintf(reply,cap,"PC_INFO_INVALID\n");
}
