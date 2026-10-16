/* Codex Token Display: BSP-owned hardware, USB statistics, independent UI. */
#include "bsp_display.h"
#include "bsp_button.h"
#include "bsp_battery.h"
#include "bsp_pins.h"
#include "key_guard.h"
#include "token_model.h"
#include "token_ui.h"
#include "system_model.h"
#include "page_model.h"
#include "volume_model.h"
#include <stdatomic.h>
#include "token_ble.h"
#include "postcard.h"
#include "device_runtime.h"
#include "driver/usb_serial_jtag.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>
static QueueHandle_t snapshots, system_snapshots, keys;
static key_guard_t s_key_guard;
static volatile bool s_input_ready;
static atomic_bool input_overflow;
static size_t accept_line(const char *line,char *reply,size_t capacity,unsigned transport) {
    if(!strcmp(line,"PC_SYS_HELLO2"))return snprintf(reply,capacity,"PC_SYS_READY 2\n");
    if(!strcmp(line,"PC_SYS_HELLO"))return snprintf(reply,capacity,"PC_SYS_READY 1\n");
    system_snapshot_t metrics;
    if(system_parse(line,&metrics)) {
        xQueueOverwrite(system_snapshots,&metrics);
        return snprintf(reply,capacity,"PC_SYS_ACK %lld\n",(long long)metrics.v[SYS_SEQ]);
    }
    size_t managed=device_runtime_line(line,reply,capacity);if(managed)return managed;
    if(!strncmp(line,"PC_",3)) return postcard_line(line,reply,capacity);
    if(!strcmp(line,"CT_HELLO 4")) return snprintf(reply,capacity,"CT_READY 4\n");
    if(!strcmp(line,"CT_HELLO 3")) return snprintf(reply,capacity,"CT_READY 3\n");
    if(!strcmp(line,"CT_HELLO 2")) return snprintf(reply,capacity,"CT_READY 2\n");
    if(!strcmp(line,"CT_HELLO")) return snprintf(reply,capacity,"CT_READY 1\n");
    token_snapshot_t next;
    if(!token_parse(line,&next)) return 0;
    next.transport=transport;
    xQueueOverwrite(snapshots,&next);
    return snprintf(reply,capacity,"CT_ACK %lld\n",(long long)next.v[T_SEQ]);
}
static size_t accept_ble(const char *line,char *reply,size_t capacity) {
    return accept_line(line,reply,capacity,TOKEN_BLE);
}
typedef struct { bsp_btn_t key; bsp_btn_ev_t event; } key_event_t;
static void on_key(bsp_btn_t key,bsp_btn_ev_t event,void *user) {
    (void)user;
    if(key_guard_consume(&s_key_guard,event==BSP_BTN_RELEASE,esp_timer_get_time())) return;
    if(!s_input_ready) return;
    key_event_t input={key,event};if(xQueueSend(keys,&input,0)!=pdTRUE)atomic_store(&input_overflow,true);
}
static void usb_worker(void *arg) {
    (void)arg;
    usb_serial_jtag_driver_config_t config=USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    config.rx_buffer_size=2048; config.tx_buffer_size=256;
    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&config));
    static token_stream_t stream; char bytes[64];
    for(;;) {
        int n=usb_serial_jtag_read_bytes(bytes,sizeof(bytes),pdMS_TO_TICKS(100));
        for(int i=0;i<n;++i) {
            if(!token_stream_push(&stream,bytes[i])) continue;
            char reply[96]; size_t len=accept_line(stream.data,reply,sizeof(reply),TOKEN_USB);
            if(len) usb_serial_jtag_write_bytes(reply,len,pdMS_TO_TICKS(100));
        }
    }
}
void app_main(void) {
    snapshots=xQueueCreate(1,sizeof(token_snapshot_t)); system_snapshots=xQueueCreate(1,sizeof(system_snapshot_t)); keys=xQueueCreate(16,sizeof(key_event_t));
    if(!snapshots||!system_snapshots||!keys) { ESP_LOGE("tokens","Queue allocation failed"); return; }
    ESP_ERROR_CHECK(bsp_display_init());
    if(!bsp_lvgl_init()) { ESP_LOGE("tokens","LVGL unavailable"); return; }
    if(!bsp_lvgl_lock(1000)) return;
    token_ui_create();token_ui_startup(true);bsp_lvgl_unlock();bsp_display_backlight(75);
    uint64_t boot_started=(uint64_t)esp_timer_get_time()/1000;bool boot_visible=true;
    if(!postcard_start()) ESP_LOGW("tokens","Postcard storage unavailable");
    if(!device_runtime_start()) ESP_LOGW("tokens","Settings/audio worker unavailable");
    key_guard_arm(&s_key_guard,esp_timer_get_time(),10000000);
    esp_err_t buttons=bsp_button_init(on_key,NULL);
    if(buttons==ESP_OK) key_guard_sync_released(&s_key_guard,bsp_button_read_mv()>=BSP_BTN_MV_RELEASED_MIN);
    s_input_ready=buttons==ESP_OK;
    if(buttons!=ESP_OK) ESP_LOGW("tokens","Buttons unavailable: %s",esp_err_to_name(buttons));
    bool has_battery=bsp_battery_init()==ESP_OK;
    if(xTaskCreate(usb_worker,"token_usb",4096,NULL,4,NULL)!=pdPASS) { ESP_LOGE("tokens","USB task unavailable"); return; }
    esp_err_t ble=token_ble_start(accept_ble);
    if(ble!=ESP_OK) ESP_LOGW("tokens","Bluetooth unavailable: %s",esp_err_to_name(ble));
    token_snapshot_t current={0};
    for(int i=0;i<TOKEN_FIELDS;++i) current.v[i]=-1;
    current.v[T_PLAN]=0;
    bool received=false; token_nav_t nav={.page=PAGE_COUNT};
    volume_controls_t volume={0};
    system_snapshot_t metrics;for(unsigned i=0;i<SYS_FIELDS;++i)metrics.v[i]=-1;
    bool metrics_received=false;uint64_t metrics_last=0;
    uint64_t last=0,next_battery=0; int battery=-1;
    for(;;) {
        uint64_t now=(uint64_t)esp_timer_get_time()/1000;
        bool boot_ended=boot_visible && now-boot_started>=1200 && (!device_runtime_booting() || now-boot_started>=6000);
        if(boot_ended) {boot_visible=false;(void)device_runtime_key(now);}
        bool changed=xQueueReceive(snapshots,&current,0)==pdTRUE;
        if(changed) { received=true; last=now; }
        if(xQueueReceive(system_snapshots,&metrics,0)==pdTRUE) {metrics_received=true;metrics_last=now;}
        page_config_t page_config=device_runtime_pages();page_nav_normalize(&page_config,&nav);
        volume.value=device_runtime_volume();unsigned before_volume=volume.value;
        if(atomic_exchange(&input_overflow,false))volume_controls_cancel(&volume,now);
        key_event_t input;
        while(xQueueReceive(keys,&input,0)==pdTRUE) {
            unsigned key=input.key==BSP_BTN_UP?TOKEN_KEY_UP:input.key==BSP_BTN_DOWN?TOKEN_KEY_DOWN:TOKEN_KEY_OK;
            if(boot_visible || boot_ended)continue;
            if(input.event==BSP_BTN_PRESS && device_runtime_key(now))volume_controls_wake(&volume,key);
            unsigned event=input.event==BSP_BTN_PRESS?VOLUME_PRESS:input.event==BSP_BTN_CLICK?VOLUME_CLICK:
                input.event==BSP_BTN_DOUBLE?VOLUME_DOUBLE:input.event==BSP_BTN_LONG?VOLUME_LONG:VOLUME_RELEASE;
            bool consumed=volume_controls_key(&volume,key,event,now);
            if(!consumed && (input.event==BSP_BTN_CLICK || input.event==BSP_BTN_DOUBLE)) {
                (void)device_runtime_key(now);page_nav_key(&page_config,&nav,key,input.event==BSP_BTN_DOUBLE);
            }
        }
        volume_controls_tick(&volume,now);
        if(volume.down)(void)device_runtime_key(now);
        if(volume.value!=before_volume)device_runtime_volume_set(volume.value,now);
        if(!boot_visible)device_runtime_tick(now,(changed || (boot_ended && received))?&current:NULL);
        if(now>=next_battery) { if(has_battery) battery=bsp_battery_soc(); next_battery=now+30000; }
        if(bsp_lvgl_lock(100)) {
            if(!boot_visible)token_ui_startup(false);
            token_ui_pages(&page_config,nav.page);
            monitor_config_t monitor=device_runtime_monitor();
            token_ui_system(&metrics,metrics_received && now-metrics_last<15000,&monitor);
            token_ui_update(&current,nav.period,nav.show_cost,received,now-last,battery,nav.page);
            token_ui_volume(volume.visible,volume.value);
            postcard_update_ui(nav.contact);bsp_lvgl_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
