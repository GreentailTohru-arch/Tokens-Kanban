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
#include "token_wifi.h"
#include "freertos/semphr.h"
#include "postcard.h"
#include "device_runtime.h"
#include "device_diagnostics.h"
#include "boot_logo.h"
#include "driver/usb_serial_jtag.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>
static SemaphoreHandle_t protocol_guard;
static QueueHandle_t snapshots, system_snapshots, keys;
static key_guard_t s_key_guard;
static volatile bool s_input_ready;
static atomic_bool input_overflow;
static size_t accept_unlocked(const char *line,char *reply,size_t capacity,unsigned transport) {
    if(!strcmp(line,"PC_VERSION"))return snprintf(reply,capacity,"PC_VERSION v19\n");
    size_t wifi=token_wifi_line(line,reply,capacity,transport);if(wifi)return wifi;
    size_t logo=boot_logo_line(line,reply,capacity);if(logo)return logo;
    size_t diagnostic=device_diagnostics_line(line,reply,capacity);if(diagnostic)return diagnostic;
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
static size_t accept_line(const char *line,char *reply,size_t capacity,unsigned transport){
    if(xSemaphoreTake(protocol_guard,pdMS_TO_TICKS(1500))!=pdTRUE)return 0;
    size_t n=accept_unlocked(line,reply,capacity,transport);xSemaphoreGive(protocol_guard);return n;
}
static size_t accept_wifi(const char *line,char *reply,size_t capacity){return accept_line(line,reply,capacity,TOKEN_WIFI);}
static size_t accept_ble(const char *line,char *reply,size_t capacity) {
    return accept_line(line,reply,capacity,TOKEN_BLE);
}
typedef struct { bsp_btn_t key; bsp_btn_ev_t event; } key_event_t;
static void on_key(bsp_btn_t key,bsp_btn_ev_t event,void *user) {
    (void)user;device_diagnostics_key((unsigned)key,(unsigned)event);
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
    protocol_guard=xSemaphoreCreateMutex();if(!protocol_guard)return;
    snapshots=xQueueCreate(1,sizeof(token_snapshot_t)); system_snapshots=xQueueCreate(1,sizeof(system_snapshot_t)); keys=xQueueCreate(16,sizeof(key_event_t));
    if(!snapshots||!system_snapshots||!keys) { ESP_LOGE("tokens","Queue allocation failed"); return; }
    ESP_ERROR_CHECK(bsp_display_init());
    if(!bsp_lvgl_init()) { ESP_LOGE("tokens","LVGL unavailable"); return; }
    if(!bsp_lvgl_lock(1000)) return;
    token_ui_create();token_ui_startup(true);bsp_lvgl_unlock();bsp_display_backlight(75);
    uint64_t boot_started=(uint64_t)esp_timer_get_time()/1000;bool boot_visible=true;
    if(!postcard_start()) ESP_LOGW("tokens","Postcard storage unavailable");
    if(!device_runtime_start()) ESP_LOGW("tokens","Settings/audio worker unavailable");
    bsp_display_backlight(device_runtime_brightness());
    if(!boot_logo_start())ESP_LOGW("tokens","Custom boot logo unavailable; using built-in logo");
    key_guard_arm(&s_key_guard,esp_timer_get_time(),10000000);
    esp_err_t buttons=bsp_button_init(on_key,NULL);
    if(buttons==ESP_OK) key_guard_sync_released(&s_key_guard,bsp_button_read_mv()>=BSP_BTN_MV_RELEASED_MIN);
    s_input_ready=buttons==ESP_OK;
    if(buttons!=ESP_OK) ESP_LOGW("tokens","Buttons unavailable: %s",esp_err_to_name(buttons));
    bool has_battery=bsp_battery_init()==ESP_OK;
    if(!device_diagnostics_start(has_battery,buttons==ESP_OK))ESP_LOGW("tokens","Diagnostics unavailable");
    if(xTaskCreate(usb_worker,"token_usb",4096,NULL,4,NULL)!=pdPASS) { ESP_LOGE("tokens","USB task unavailable"); return; }
    esp_err_t ble=token_ble_start(accept_ble);
    if(ble!=ESP_OK) ESP_LOGW("tokens","Bluetooth unavailable: %s",esp_err_to_name(ble));
    if(!token_wifi_start(accept_wifi))ESP_LOGW("tokens","Wi-Fi service unavailable");
    bool network_visible=false,network_setup=false;uint64_t network_until=0;
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
        bool boot_ended=boot_visible && now-boot_started>=boot_logo_duration() && (!device_runtime_booting() || now-boot_started>=6000);
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
            if(input.event==BSP_BTN_PRESS && device_diagnostics_screen_stop())volume_controls_wake(&volume,key);
            if(input.event==BSP_BTN_PRESS && device_runtime_key(now))volume_controls_wake(&volume,key);
            unsigned event=input.event==BSP_BTN_PRESS?VOLUME_PRESS:input.event==BSP_BTN_CLICK?VOLUME_CLICK:
                input.event==BSP_BTN_DOUBLE?VOLUME_DOUBLE:input.event==BSP_BTN_LONG?VOLUME_LONG:VOLUME_RELEASE;
            bool consumed=volume_controls_key(&volume,key,event,now);
            if(!consumed && key==TOKEN_KEY_OK && input.event==BSP_BTN_LONG){
                network_visible=true;network_setup=false;network_until=now+120000;volume_controls_cancel(&volume,now);consumed=true;
            }
            if(network_visible&&!consumed){
                consumed=true;
                if(input.event==BSP_BTN_CLICK||input.event==BSP_BTN_DOUBLE){
                    if(key==TOKEN_KEY_OK)network_visible=false;
                    else{network_setup=!network_setup;if(network_setup)token_wifi_setup_request();network_until=now+120000;}
                }
            }
            if(!consumed && (input.event==BSP_BTN_CLICK || input.event==BSP_BTN_DOUBLE)) {
                (void)device_runtime_key(now);page_nav_key(&page_config,&nav,key,input.event==BSP_BTN_DOUBLE);
            }
        }
        if(network_visible){if(now>=network_until)network_visible=false;else (void)device_runtime_key(now);}
        volume_controls_tick(&volume,now);
        if(volume.down)(void)device_runtime_key(now);
        if(volume.value!=before_volume)device_runtime_volume_set(volume.value,now);
        if(!boot_visible)device_runtime_tick(now,(changed || (boot_ended && received))?&current:NULL);
        if(now>=next_battery) { if(has_battery) battery=bsp_battery_soc(); next_battery=now+30000; }
        if(bsp_lvgl_lock(100)) {
            if(!boot_visible){token_ui_startup(false);boot_logo_finish();}
            else{unsigned side;const uint8_t *pixels=boot_logo_frame(now-boot_started,&side);if(pixels)token_ui_startup_frame(pixels,side);}
            token_ui_pages(&page_config,nav.page);
            monitor_config_t monitor=device_runtime_monitor();
            token_ui_system(&metrics,metrics_received && now-metrics_last<15000,&monitor);
            token_ui_update(&current,nav.period,nav.show_cost,received,now-last,battery,nav.page);
            token_ui_volume(volume.visible,volume.value);
            token_wifi_status_t network;token_wifi_status(&network);char qr[160],hint[96];
            if(network_setup&&network.ap)snprintf(qr,sizeof(qr),"WIFI:T:WPA;S:%s;P:%s;;",network.ap_ssid,network.ap_password);
            else token_wifi_management_url(qr,sizeof(qr),false);
            if(network_setup)snprintf(hint,sizeof(hint),network.ap?"%s":"热点启动中",network.ap_ssid);
            else snprintf(hint,sizeof(hint),network.state==NET_CONNECTED?"手机扫码管理设备":"未联网，扫码打开网站");
            token_ui_network(network_visible,network_setup,qr,hint);
            postcard_update_ui(nav.contact);int test_screen=device_diagnostics_screen(now);if(test_screen>=0)(void)device_runtime_key(now);token_ui_diagnostic(test_screen);bsp_lvgl_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
