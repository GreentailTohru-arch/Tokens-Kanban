/* Codex Token Display: BSP-owned hardware, USB statistics, independent UI. */
#include "bsp_display.h"
#include "bsp_button.h"
#include "bsp_battery.h"
#include "bsp_pins.h"
#include "key_guard.h"
#include "token_model.h"
#include "token_ui.h"
#include "token_ble.h"
#include "postcard.h"
#include "driver/usb_serial_jtag.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>
static QueueHandle_t snapshots, keys;
static key_guard_t s_key_guard;
static volatile bool s_input_ready;
static size_t accept_line(const char *line,char *reply,size_t capacity,unsigned transport) {
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
    if(event!=BSP_BTN_CLICK && event!=BSP_BTN_DOUBLE) return;
    key_event_t input={key,event}; xQueueSend(keys,&input,0);
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
            char reply[48]; size_t len=accept_line(stream.data,reply,sizeof(reply),TOKEN_USB);
            if(len) usb_serial_jtag_write_bytes(reply,len,pdMS_TO_TICKS(100));
        }
    }
}
void app_main(void) {
    snapshots=xQueueCreate(1,sizeof(token_snapshot_t)); keys=xQueueCreate(8,sizeof(key_event_t));
    if(!snapshots||!keys) { ESP_LOGE("tokens","Queue allocation failed"); return; }
    ESP_ERROR_CHECK(bsp_display_init());
    if(!bsp_lvgl_init()) { ESP_LOGE("tokens","LVGL unavailable"); return; }
    if(!bsp_lvgl_lock(1000)) return;
    token_ui_create(); bsp_lvgl_unlock(); bsp_display_backlight(75);
    if(!postcard_start()) ESP_LOGW("tokens","Postcard storage unavailable");
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
    bool received=false; token_nav_t nav={0};
    uint64_t last=0,next_battery=0; int battery=-1;
    for(;;) {
        uint64_t now=(uint64_t)esp_timer_get_time()/1000;
        if(xQueueReceive(snapshots,&current,0)==pdTRUE) { received=true; last=now; }
        key_event_t input;
        while(xQueueReceive(keys,&input,0)==pdTRUE) {
            unsigned key=input.key==BSP_BTN_UP?TOKEN_KEY_UP:input.key==BSP_BTN_DOWN?TOKEN_KEY_DOWN:TOKEN_KEY_OK;
            token_nav_key(&nav,key,input.event==BSP_BTN_DOUBLE);
        }
        if(now>=next_battery) { if(has_battery) battery=bsp_battery_soc(); next_battery=now+30000; }
        if(bsp_lvgl_lock(100)) {
            token_ui_update(&current,nav.period,nav.show_cost,received,now-last,battery,nav.page);
            postcard_update_ui(nav.contact);bsp_lvgl_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
