/* Exercise the production worker, storage errors and commands without hardware. */
#include <assert.h>
#include <setjmp.h>
#include "../main/device_runtime.c"
static request_t pending[4];
static unsigned count,writes,commits,audio_samples,backlight=75;
static bool fail_commit,fail_task,have_stored;
static device_config_t stored,staged;
static const int16_t *expected_pcm;
static size_t expected_count,captured_count;
static jmp_buf yielded;
int64_t esp_timer_get_time(void){return 0;}
QueueHandle_t xQueueCreate(unsigned n,size_t size){assert(n==4&&size==sizeof(request_t));return pending;}
void vQueueDelete(QueueHandle_t q){assert(q==pending);}
int xQueueSend(QueueHandle_t q,const void *data,unsigned wait){(void)wait;assert(q==pending);if(count==4)return 0;pending[count++]=*(const request_t *)data;return 1;}
int xQueueReceive(QueueHandle_t q,void *out,unsigned wait){(void)wait;assert(q==pending);if(!count)longjmp(yielded,1);*(request_t *)out=pending[0];memmove(pending,pending+1,--count*sizeof(request_t));return 1;}
int xTaskCreate(void (*f)(void *),const char *name,unsigned stack,void *arg,unsigned priority,void *out){(void)f;(void)name;(void)stack;(void)arg;(void)priority;(void)out;return fail_task?0:1;}
esp_err_t nvs_flash_init(void){return ESP_OK;}
esp_err_t nvs_open(const char *name,int mode,nvs_handle_t *handle){(void)mode;assert(!strcmp(name,"tokens_ui"));*handle=1;return ESP_OK;}
esp_err_t nvs_get_blob(nvs_handle_t handle,const char *key,void *data,size_t *size){(void)handle;assert(!strcmp(key,"cfg1")&&*size==sizeof(stored));if(!have_stored)return ESP_FAIL;memcpy(data,&stored,*size);return ESP_OK;}
esp_err_t nvs_set_blob(nvs_handle_t handle,const char *key,const void *data,size_t size){(void)handle;assert(!strcmp(key,"cfg1")&&size==sizeof(stored));memcpy(&staged,data,size);writes++;return ESP_OK;}
esp_err_t nvs_commit(nvs_handle_t handle){(void)handle;commits++;if(fail_commit)return ESP_FAIL;stored=staged;have_stored=true;return ESP_OK;}
void nvs_close(nvs_handle_t handle){(void)handle;}
esp_err_t bsp_audio_init(void){return ESP_OK;}
esp_err_t bsp_audio_wake(void){return ESP_OK;}
esp_err_t bsp_audio_sleep(void){return ESP_OK;}
esp_err_t bsp_audio_set_format(uint32_t hz,uint8_t bits,uint8_t channels){assert(hz==16000&&bits==16&&channels==1);return ESP_OK;}
void bsp_audio_set_volume(uint8_t volume){assert(volume<=100);}
esp_err_t bsp_audio_write(const void *data,size_t n){
    assert(n<=640&&n%2==0);const int16_t *pcm=data;
    if(expected_pcm)for(size_t i=0;i<n/2;++i,++captured_count)
        assert(pcm[i]==(captured_count<expected_count?expected_pcm[captured_count]:0));
    audio_samples+=n/2;return ESP_OK;
}
void bsp_display_backlight(uint8_t percent){backlight=percent;}
static void run(void){if(!setjmp(yielded))worker(NULL);}
static char reply[100];
static void line(const char *s,const char *expected){assert(device_runtime_line(s,reply,sizeof(reply))>0);assert(!strcmp(reply,expected));}
static void reboot(void){count=save_id=save_state=event_host=event_sequence=0;event_seen=event_pending=false;behavior=(device_behavior_t){0};atomic_store(&wake_requested,false);assert(device_runtime_start());}
int main(void){
    /* Capture the actual BSP output, including the partial final block and drain. */
    assert(codex_notification_samples==9134 && codex_error_samples==9387);
    const int16_t *clips[]={codex_notification_pcm,codex_error_pcm,opencode_complete_pcm,opencode_error_pcm,antigravity_complete_pcm,antigravity_error_pcm};
    size_t counts[]={9134,9387,5944,7059,41472,17664};unsigned ids[]={1,3,4,5,6,7};
    for(unsigned i=0;i<6;++i){expected_pcm=clips[i];expected_count=counts[i];captured_count=0;assert(play_tone(ids[i],35));assert(captured_count==expected_count+1920);}
    expected_pcm=NULL;assert(!play_tone(2,35));
    reboot();line("PC_CFG_GET","PC_CFG 1 0 1 3 35\n");
    line("PC_SOUND_BANKS","PC_SOUND_BANKS 3\n");
    const char *invalid[]={"PC_CFG_SET 1 1 1 3 35","PC_CFG_SET 1 60 3 1 35","PC_CFG_SET 1 60 1 3 101","PC_CFG_SET 1 4294967296 1 3 35","PC_SOUND 2 30","PC_SOUND -1 30","PC_EVT 1 1 4294967297","PC_EVT 1 +1 1"};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i)line(invalid[i],"PC_CFG_INVALID\n");
    line("PC_CFG_SET 10 60 4 5 40","PC_CFG_STATE 10 1\n");assert(config.idle_seconds==0&&count==1);
    line("PC_CFG_SET 10 60 4 5 40","PC_CFG_STATE 10 1\n");assert(count==1);
    line("PC_CFG_SET 11 60 4 5 40","PC_CFG_BUSY\n");run();assert(writes==1&&commits==1);
    line("PC_CFG_STATUS","PC_CFG_STATE 10 2\n");reboot();line("PC_CFG_GET","PC_CFG 1 60 4 5 40\n");
    fail_commit=true;line("PC_CFG_SET 12 120 1 3 20","PC_CFG_STATE 12 1\n");run();line("PC_CFG_STATUS","PC_CFG_STATE 12 3\n");line("PC_CFG_GET","PC_CFG 1 60 4 5 40\n");fail_commit=false;
    reboot();assert(config.idle_seconds==60);
    for(unsigned i=0;i<4;++i)line("PC_SOUND 1 35","PC_SOUND_QUEUED\n");
    line("PC_CFG_SET 20 120 1 3 20","PC_CFG_RETRY\n");assert(save_id==0);
    line("PC_EVT 7 1 1","PC_CFG_RETRY\n");assert(!event_seen);run();assert(audio_samples>0);
    line("PC_EVT 7 1 1","PC_EVT_ACK 7 1\n");assert(count==1);line("PC_EVT 7 1 1","PC_EVT_ACK 7 1\n");assert(count==1);run();
    line("PC_EVT 7 3 1","PC_EVT_ACK 7 3\n");line("PC_EVT 7 2 1","PC_EVT_ACK 7 2\n");assert(count==1);run();
    line("PC_EVT 8 1 1","PC_EVT_ACK 8 1\n");run();
    config.done_tone=0;line("PC_EVT 8 2 1","PC_EVT_ACK 8 2\n");assert(count==0);
    config.volume=0;line("PC_EVT 8 3 1","PC_EVT_ACK 8 3\n");assert(count==0);
    /* Saving wakes, stats do not. First key is consumed rather than navigating. */
    atomic_store(&wake_requested,false);device_runtime_tick(60000,NULL);assert(behavior.off&&backlight==0);
    token_snapshot_t snapshot={0};snapshot.v[T_QUOTA_AGE]=-1;device_runtime_tick(65000,&snapshot);assert(backlight==0);
    assert(device_runtime_key(70000)&&backlight==75);assert(!device_runtime_key(70001));
    device_runtime_tick(130001,NULL);assert(backlight==0);
    line("PC_SOUND 1 10","PC_SOUND_QUEUED\n");device_runtime_tick(130002,NULL);assert(backlight==75);run();
    stored=(device_config_t){600,2,2,55};have_stored=true;reboot();
    line("PC_CFG_GET","PC_CFG 1 600 1 3 55\n");assert(config_dirty);
    line("PC_CFG_SET 40 600 1 3 55","PC_CFG_STATE 40 1\n");run();assert(!config_dirty&&stored.done_tone==1&&stored.quota_tone==3);
    nvs_ready=false;line("PC_CFG_SET 30 60 1 3 10","PC_CFG_UNAVAILABLE\n");
    fail_task=true;assert(!device_runtime_start());line("PC_CFG_GET","PC_CFG_UNAVAILABLE\n");
    puts("Settings worker, persistence failures, audio bounds, wake and event deduplication: PASS");return 0;
}
