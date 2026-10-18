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
#include "pet_model.h"
#include "pet_controls.h"
#include "pet_remote.h"
#include "volume_model.h"
#include <stdatomic.h>
#include "token_ble.h"
#include "freertos/semphr.h"
#include "postcard.h"
#include "wallpaper.h"
#include "device_runtime.h"
#include "device_diagnostics.h"
#include "boot_logo.h"
#include "lvgl.h"
#include "driver/usb_serial_jtag.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>
static SemaphoreHandle_t protocol_guard;
static pet_remote_t remote;
static portMUX_TYPE remote_mutex=portMUX_INITIALIZER_UNLOCKED;
static TaskHandle_t main_task_handle;
static QueueHandle_t snapshots, system_snapshots, keys, pet_snapshots;
typedef struct {unsigned state;uint64_t at;} pet_message_t;
static key_guard_t s_key_guard;
static volatile bool s_input_ready;
static atomic_bool input_overflow;
static size_t accept_unlocked(const char *line,char *reply,size_t capacity,unsigned transport) {
    if(!strcmp(line,"PC_INFO_STACK"))return snprintf(reply,capacity,"PC_STACK %u %u\n",(unsigned)CONFIG_ESP_MAIN_TASK_STACK_SIZE,(unsigned)uxTaskGetStackHighWaterMark(main_task_handle));
    if(!strcmp(line,"PC_VERSION"))return snprintf(reply,capacity,"PC_VERSION v28\n");
    if(!strcmp(line,"PC_PET_HELLO"))return snprintf(reply,capacity,"PC_PET_READY 1\n");
    if(!strncmp(line,"PC_PET ",7)){
        unsigned seq,state;
        if(!pet_parse(line+7,&seq,&state))return snprintf(reply,capacity,"PC_PET_INVALID\n");
        pet_message_t msg={state,(uint64_t)esp_timer_get_time()/1000};xQueueOverwrite(pet_snapshots,&msg);
        return snprintf(reply,capacity,"PC_PET_ACK %u\n",seq);
    }
    portENTER_CRITICAL(&remote_mutex);
    size_t control=pet_remote_line(&remote,line,reply,capacity,transport,(uint64_t)esp_timer_get_time()/1000);
    portEXIT_CRITICAL(&remote_mutex);if(control)return control;
    size_t wall=wallpaper_line(line,reply,capacity);if(wall)return wall;
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
static size_t accept_ble(const char *line,char *reply,size_t capacity) {
    return accept_line(line,reply,capacity,TOKEN_BLE);
}
typedef struct { bsp_btn_t key; bsp_btn_ev_t event; uint64_t at; } key_event_t;
static void on_key(bsp_btn_t key,bsp_btn_ev_t event,void *user) {
    (void)user;device_diagnostics_key((unsigned)key,(unsigned)event);
    if(key_guard_consume(&s_key_guard,event==BSP_BTN_RELEASE,esp_timer_get_time())) return;
    if(!s_input_ready) return;
    key_event_t input={key,event,(uint64_t)esp_timer_get_time()/1000};if(xQueueSend(keys,&input,0)!=pdTRUE)atomic_store(&input_overflow,true);
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
            char reply[256]; size_t len=accept_line(stream.data,reply,sizeof(reply),TOKEN_USB);
            if(len) usb_serial_jtag_write_bytes(reply,len,pdMS_TO_TICKS(100));
        }
    }
}
void app_main(void) {
    main_task_handle=xTaskGetCurrentTaskHandle();
    protocol_guard=xSemaphoreCreateMutex();if(!protocol_guard)return;
    pet_snapshots=xQueueCreate(1,sizeof(pet_message_t));
    snapshots=xQueueCreate(1,sizeof(token_snapshot_t));
    system_snapshots=xQueueCreate(1,sizeof(system_snapshot_t)); keys=xQueueCreate(16,sizeof(key_event_t));
    if(!snapshots||!system_snapshots||!keys||!pet_snapshots) { ESP_LOGE("tokens","Queue allocation failed"); return; }
    ESP_ERROR_CHECK(bsp_display_init());
    if(!bsp_lvgl_init()) { ESP_LOGE("tokens","LVGL unavailable"); return; }
    if(!bsp_lvgl_lock(1000)) return;
    token_ui_create();token_ui_startup(true);bsp_lvgl_unlock();
    if(!wallpaper_start()) ESP_LOGW("tokens","Wallpaper storage requires the new full firmware layout");
    if(!postcard_start()) ESP_LOGW("tokens","Postcard storage unavailable");
    if(!device_runtime_start()) ESP_LOGW("tokens","Settings/audio worker unavailable");
    if(!boot_logo_start())ESP_LOGW("tokens","Custom boot logo unavailable; using built-in logo");
    /* Select the persisted image while dark: never expose the default first. */
    if(!bsp_lvgl_lock(1000))return;
    unsigned boot_side=0;const uint8_t *boot_pixels=boot_logo_frame(0,&boot_side);
    if(boot_pixels)token_ui_startup_frame(boot_pixels,boot_side,boot_logo_height());
    lv_refr_now(NULL);
    ESP_ERROR_CHECK(bsp_display_wait_frame());
    bsp_display_backlight(device_runtime_brightness());
    bsp_lvgl_unlock();
    uint64_t boot_started=(uint64_t)esp_timer_get_time()/1000;bool boot_visible=true;
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
    token_snapshot_t current={0};
    for(int i=0;i<TOKEN_FIELDS;++i) current.v[i]=-1;
    current.v[T_PLAN]=0;
    pet_message_t pet={PET_COUNT,0};
    bool received=false; unsigned rendered_page=PAGE_COUNT; token_nav_t nav={.page=PAGE_COUNT};
    volume_controls_t volume={0};pet_controls_t pet_keys={0};
    system_snapshot_t metrics;for(unsigned i=0;i<SYS_FIELDS;++i)metrics.v[i]=-1;
    bool metrics_received=false;uint64_t metrics_last=0;
    uint64_t last=0,next_battery=0; int battery=-1;
    for(;;) {
        uint64_t now=(uint64_t)esp_timer_get_time()/1000;
        bool boot_ended=boot_visible && now-boot_started>=boot_logo_duration() && (!device_runtime_booting() || now-boot_started>=6000);
        if(boot_ended) {boot_visible=false;(void)device_runtime_key(now);}
        (void)xQueueReceive(pet_snapshots,&pet,0);
        bool changed=xQueueReceive(snapshots,&current,0)==pdTRUE;
        if(changed) { received=true; last=now; }
        if(xQueueReceive(system_snapshots,&metrics,0)==pdTRUE) {metrics_received=true;metrics_last=now;}
        page_config_t page_config=device_runtime_pages();page_nav_normalize(&page_config,&nav);
        volume.value=device_runtime_volume();unsigned before_volume=volume.value;
        pet_bindings_t bindings=device_runtime_pet_bindings();unsigned remote_action=0;
        bool pet_active=nav.page==PAGE_PET && !boot_visible && !device_runtime_screen_off();
        portENTER_CRITICAL(&remote_mutex);bool remote_live=remote.session && now-remote.polled<=1500;portEXIT_CRITICAL(&remote_mutex);
        if(!remote_live && pet_keys.voice)pet_controls_cancel(&pet_keys);
        if(!pet_active)pet_controls_cancel(&pet_keys);
        if(atomic_exchange(&input_overflow,false)){volume_controls_cancel(&volume,now);pet_controls_cancel(&pet_keys);}
        key_event_t input;
        while(xQueueReceive(keys,&input,0)==pdTRUE) {
            unsigned key=input.key==BSP_BTN_UP?TOKEN_KEY_UP:input.key==BSP_BTN_DOWN?TOKEN_KEY_DOWN:TOKEN_KEY_OK;
            if(boot_visible || boot_ended)continue;
            if(input.event==BSP_BTN_PRESS && device_diagnostics_screen_stop())volume_controls_wake(&volume,key);
            if(input.event==BSP_BTN_PRESS && device_runtime_key(now))volume_controls_wake(&volume,key);
            unsigned event=input.event==BSP_BTN_PRESS?VOLUME_PRESS:input.event==BSP_BTN_CLICK?VOLUME_CLICK:
                input.event==BSP_BTN_DOUBLE?VOLUME_DOUBLE:input.event==BSP_BTN_LONG?VOLUME_LONG:VOLUME_RELEASE;
            bool blocked=volume.visible || volume.waking || now<volume.quiet_until || (key==TOKEN_KEY_OK && now<volume.ok_until);
            bool consumed=volume_controls_key(&volume,key,event,now);
            if(nav.page==PAGE_PET){
                if(blocked){pet_controls_cancel(&pet_keys);continue;}
                if(input.event==BSP_BTN_PRESS || input.event==BSP_BTN_RELEASE || input.event==BSP_BTN_LONG){
                    unsigned ev=input.event==BSP_BTN_PRESS?PET_CONTROL_PRESS:input.event==BSP_BTN_RELEASE?PET_CONTROL_RELEASE:PET_CONTROL_LONG;
                    unsigned action=pet_controls_event(&pet_keys,&bindings,key,ev,input.at);
                    if(action)remote_action=action;
                }
                continue;
            }
            if(!consumed && (input.event==BSP_BTN_CLICK || input.event==BSP_BTN_DOUBLE)) {
                (void)device_runtime_key(now);page_nav_key(&page_config,&nav,key,input.event==BSP_BTN_DOUBLE);
            }
        }
        if(nav.page==PAGE_PET){unsigned action=pet_controls_tick(&pet_keys,&bindings,now);if(action)remote_action=action;}
        if(remote_action==PET_ACTION_NEXT){page_nav_key(&page_config,&nav,TOKEN_KEY_OK,true);remote_action=0;pet_controls_cancel(&pet_keys);}
        portENTER_CRITICAL(&remote_mutex);
        if(nav.page!=PAGE_PET || !pet_active){remote.action=0;remote.voice=false;}
        else pet_remote_update(&remote,remote_action,pet_keys.voice,now);
        portEXIT_CRITICAL(&remote_mutex);
        volume_controls_tick(&volume,now);
        if(volume.down)(void)device_runtime_key(now);
        if(volume.value!=before_volume)device_runtime_volume_set(volume.value,now);
        if(!boot_visible)device_runtime_tick(now,(changed || (boot_ended && received))?&current:NULL);
        if(now>=next_battery) { if(has_battery) battery=bsp_battery_soc(); next_battery=now+30000; }
        if(bsp_lvgl_lock(100)) {
            if(!boot_visible){token_ui_startup(false);boot_logo_finish();}
            else{unsigned side;const uint8_t *pixels=boot_logo_frame(now-boot_started,&side);if(pixels)token_ui_startup_frame(pixels,side,boot_logo_height());}
            theme_config_t theme=device_runtime_theme();token_ui_theme(&theme);
            token_ui_pages(&page_config,nav.page);
            monitor_config_t monitor=device_runtime_monitor();
            token_ui_system(&metrics,metrics_received && now-metrics_last<15000,&monitor);
            token_ui_update(&current,nav.period,nav.show_cost,received,now-last,battery,nav.page);
            token_ui_pet(pet_resolve(token_online(received,now-last,0),token_signal(&current,received,now-last),pet.state,now-pet.at),now,nav.page==PAGE_PET && !boot_visible && !device_runtime_screen_off());
            token_ui_volume(volume.visible,volume.value);
            postcard_update_ui(nav.contact);int test_screen=device_diagnostics_screen(now);if(test_screen>=0)(void)device_runtime_key(now);token_ui_diagnostic(test_screen);
            if(rendered_page!=nav.page && !boot_visible && !device_runtime_screen_off()){
                /* Present the complete new page in this update, without waiting
                 * for the periodic LVGL refresh. No fade, slide or blank frame. */
                lv_obj_invalidate(lv_screen_active());
                lv_refr_now(NULL);
                (void)bsp_display_wait_frame();
                rendered_page=nav.page;
            }
            bsp_lvgl_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
