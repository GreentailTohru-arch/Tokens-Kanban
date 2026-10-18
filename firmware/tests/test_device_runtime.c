/* Exercise the production worker, storage errors and commands without hardware. */
#include <assert.h>
#include <setjmp.h>
#include "../main/device_runtime.c"
static pet_bindings_t stored_keys,staged_keys;static bool have_keys,keys_write;
static request_t pending[4];
static unsigned count,writes,commits,audio_samples,backlight=75,format_calls,last_audio_volume;
static unsigned mute_at,inject_volume=UINT32_MAX;
static int64_t fake_time_us;
static bool fail_commit,fail_task,have_stored;
static device_config_t stored,staged;
static theme_config_t stored_theme,staged_theme;static bool have_theme,theme_write;
static size_t pages_size=sizeof(page_config_t);
static page_config_t stored_pages,staged_pages;static bool have_pages,pages_write;
static monitor_config_t stored_monitor,staged_monitor;static bool have_monitor,monitor_write;
static unsigned stored_brightness=75,staged_brightness;static bool display_write,have_brightness;
static uint32_t legacy[4];static bool have_legacy;
static const int16_t *expected_pcm;
static size_t expected_count,captured_count;
static jmp_buf yielded;
int64_t esp_timer_get_time(void){return fake_time_us;}
QueueHandle_t xQueueCreate(unsigned n,size_t size){assert(n==4&&size==sizeof(request_t));return pending;}
void vQueueDelete(QueueHandle_t q){assert(q==pending);}
int xQueueSend(QueueHandle_t q,const void *data,unsigned wait){(void)wait;assert(q==pending);if(count==4)return 0;pending[count++]=*(const request_t *)data;return 1;}
int xQueueReceive(QueueHandle_t q,void *out,unsigned wait){(void)wait;assert(q==pending);if(!count)longjmp(yielded,1);*(request_t *)out=pending[0];memmove(pending,pending+1,--count*sizeof(request_t));return 1;}
int xTaskCreate(void (*f)(void *),const char *name,unsigned stack,void *arg,unsigned priority,void *out){(void)f;(void)name;(void)stack;(void)arg;(void)priority;(void)out;return fail_task?0:1;}
esp_err_t nvs_flash_init(void){return ESP_OK;}
esp_err_t nvs_open(const char *name,int mode,nvs_handle_t *handle){(void)mode;assert(!strcmp(name,"tokens_ui"));*handle=1;return ESP_OK;}
esp_err_t nvs_get_blob(nvs_handle_t handle,const char *key,void *data,size_t *size){
    (void)handle;
    if(!strcmp(key,"petkeys1")){assert(*size==sizeof(stored_keys));if(!have_keys)return ESP_FAIL;memcpy(data,&stored_keys,*size);return ESP_OK;}
    if(!strcmp(key,"theme1")){assert(*size==sizeof(stored_theme));if(!have_theme)return ESP_FAIL;memcpy(data,&stored_theme,*size);return ESP_OK;}
    if(!strcmp(key,"display1")){assert(*size==4);if(!have_brightness)return ESP_FAIL;memcpy(data,&stored_brightness,4);return ESP_OK;}
    if(!strcmp(key,"mon1")){assert(*size==sizeof(stored_monitor));if(!have_monitor)return ESP_FAIL;memcpy(data,&stored_monitor,*size);return ESP_OK;}
    if(!strcmp(key,"pages1")){assert(*size==sizeof(stored_pages));if(!have_pages)return ESP_FAIL;memcpy(data,&stored_pages,pages_size);*size=pages_size;return ESP_OK;}
    if(!strcmp(key,"cfg2")){assert(*size==sizeof(stored));if(!have_stored)return ESP_FAIL;memcpy(data,&stored,*size);return ESP_OK;}
    assert(!strcmp(key,"cfg1")&&*size==sizeof(legacy));if(!have_legacy)return ESP_FAIL;memcpy(data,legacy,*size);return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t handle,const char *key,const void *data,size_t size){(void)handle;keys_write=!strcmp(key,"petkeys1");if(keys_write){assert(size==sizeof(staged_keys));memcpy(&staged_keys,data,size);writes++;return ESP_OK;}theme_write=!strcmp(key,"theme1");if(theme_write){assert(size==sizeof(staged_theme));memcpy(&staged_theme,data,size);writes++;return ESP_OK;}display_write=!strcmp(key,"display1");if(display_write){assert(size==4);memcpy(&staged_brightness,data,4);writes++;return ESP_OK;}monitor_write=!strcmp(key,"mon1");pages_write=!strcmp(key,"pages1");if(monitor_write){assert(size==sizeof(staged_monitor));memcpy(&staged_monitor,data,size);}else if(pages_write){assert(size==sizeof(staged_pages));memcpy(&staged_pages,data,size);}else{assert(!strcmp(key,"cfg2")&&size==sizeof(stored));memcpy(&staged,data,size);}writes++;return ESP_OK;}
esp_err_t nvs_commit(nvs_handle_t handle){(void)handle;commits++;if(fail_commit)return ESP_FAIL;if(keys_write){stored_keys=staged_keys;have_keys=true;}else if(theme_write){stored_theme=staged_theme;have_theme=true;}else if(display_write){stored_brightness=staged_brightness;have_brightness=true;}else if(monitor_write){stored_monitor=staged_monitor;have_monitor=true;}else if(pages_write){stored_pages=staged_pages;have_pages=true;}else{stored=staged;have_stored=true;}if(inject_volume!=UINT32_MAX){unsigned next=inject_volume;inject_volume=UINT32_MAX;device_runtime_volume_set(next,(uint64_t)fake_time_us/1000);}return ESP_OK;}
void nvs_close(nvs_handle_t handle){(void)handle;}
esp_err_t bsp_audio_init(void){return ESP_OK;}
esp_err_t bsp_audio_wake(void){return ESP_OK;}
esp_err_t bsp_audio_sleep(void){return ESP_OK;}
esp_err_t bsp_audio_set_format(uint32_t hz,uint8_t bits,uint8_t channels){assert(hz==16000&&bits==16&&channels==1);format_calls++;return ESP_OK;}
void bsp_audio_set_volume(uint8_t volume){assert(volume<=100);last_audio_volume=volume;}
esp_err_t bsp_audio_write(const void *data,size_t n){
    assert(n<=640&&n%2==0);const int16_t *pcm=data;
    if(expected_pcm)for(size_t i=0;i<n/2;++i,++captured_count)
        assert(pcm[i]==(captured_count<expected_count?expected_pcm[captured_count]:0));
    audio_samples+=n/2;if(mute_at && audio_samples>=mute_at){mute_at=0;device_runtime_volume_set(0,(uint64_t)fake_time_us/1000);}return ESP_OK;
}
void bsp_display_backlight(uint8_t percent){backlight=percent;}
static unsigned diagnostic_result_id,diagnostic_result_samples;static bool diagnostic_result_ok;
void device_diagnostics_test_complete(unsigned id,bool ok,int peak,int average,unsigned samples){(void)peak;(void)average;diagnostic_result_id=id;diagnostic_result_samples=samples;diagnostic_result_ok=ok;}
esp_err_t bsp_audio_read(void *buffer,size_t n){assert(n==640);int16_t *pcm=buffer;for(unsigned i=0;i<320;++i)pcm[i]=i%2?-100:100;return ESP_OK;}
static void run(void){if(!setjmp(yielded))worker(NULL);}
static char reply[384];
static void line(const char *s,const char *expected){assert(device_runtime_line(s,reply,sizeof(reply))>0);assert(!strcmp(reply,expected));}
static void reboot(void){count=save_id=save_state=event_host=event_sequence=0;event_seen=event_pending=false;behavior=(device_behavior_t){0};atomic_store(&wake_requested,false);assert(device_runtime_start());assert(device_runtime_booting());run();assert(!device_runtime_booting());}
int main(void){
    reboot();theme_config_t original=theme_default();assert(!memcmp(&theme,&original,sizeof(theme)));
    assert(theme_valid(&original)&&!theme_valid(NULL));
    assert(theme_color(&original,0x2F3A49)==0x2F3A49&&theme_color(&original,0x70BFFF)==0x70BFFF);
    theme_config_t black={3,0,0,0,0,0};assert(theme_valid(&black));assert(theme_color(&black,0x2F3A49)==0&&theme_color(&black,0xFF626C)==0xFF626C);
    line("PC_THEME_GET","PC_THEME 1 0 6866123 7389183 3095113 15857652 11385027\n");
    line("PC_THEME_SET 900 3 4587387 7274386 198403 10747831 6537085","PC_CFG_STATE 900 1\n");
    line("PC_THEME_SET 900 3 4587387 7274386 198403 10747831 6537085","PC_CFG_STATE 900 1\n");assert(count==1);run();assert(theme.preset==3);reboot();assert(theme.preset==3&&theme.background==198403);
    fail_commit=true;line("PC_THEME_SET 901 0 1 2 3 4 5","PC_CFG_STATE 901 1\n");run();assert(theme.preset==3);line("PC_CFG_STATUS","PC_CFG_STATE 901 3\n");fail_commit=false;
    line("PC_THEME_SET 902 4 1 2 3 4 5","PC_CFG_INVALID\n");line("PC_THEME_SET 902 0 16777216 2 3 4 5","PC_CFG_INVALID\n");line("PC_THEME_SET 902 0 -1 2 3 4 5","PC_CFG_INVALID\n");line("PC_THEME_SET 902 0 4294967296 2 3 4 5","PC_CFG_INVALID\n");
    have_theme=false;reboot();assert(!memcmp(&theme,&original,sizeof(theme)));theme_write=false;
    reboot();line("PC_DISPLAY_GET","PC_DISPLAY 1 75\n");
    line("PC_DISPLAY_SET 240 5","PC_CFG_STATE 240 1\n");run();assert(device_runtime_brightness()==5);reboot();assert(device_runtime_brightness()==5);behavior.off=true;assert(device_runtime_key(100));assert(backlight==5);
    fail_commit=true;line("PC_DISPLAY_SET 241 100","PC_CFG_STATE 241 1\n");run();assert(device_runtime_brightness()==5);fail_commit=false;
    line("PC_DISPLAY_SET 242 0","PC_CFG_INVALID\n");line("PC_DISPLAY_SET 242 101","PC_CFG_INVALID\n");
    have_brightness=false;reboot();writes=commits=0;
    /* Capture the actual BSP output, including the partial final block and drain. */
    assert(codex_notification_samples==9134 && codex_error_samples==9387);
    const int16_t *clips[]={codex_notification_pcm,codex_error_pcm,opencode_complete_pcm,opencode_error_pcm,antigravity_complete_pcm,antigravity_error_pcm,startup_sound_pcm};
    size_t counts[]={9134,9387,5944,7059,41472,17664,startup_sound_samples};unsigned ids[]={1,3,4,5,6,7,8};
    for(unsigned i=0;i<7;++i){expected_pcm=clips[i];expected_count=counts[i];captured_count=0;assert(play_tone(ids[i],35));assert(captured_count==expected_count+1920);}
    expected_pcm=NULL;assert(!play_tone(2,35));
    reboot();line("PC_CFG_GET","PC_CFG 1 0 1 3 35\n");line("PC_CFG_GET2","PC_CFG 2 0 1 3 35 1\n");
    line("PC_SOUND_BANKS","PC_SOUND_BANKS 3\n");
    const char *invalid[]={"PC_CFG_SET 1 1 1 3 35","PC_CFG_SET 1 60 3 1 35","PC_CFG_SET 1 60 1 3 101","PC_CFG_SET 1 4294967296 1 3 35","PC_SOUND 2 30","PC_SOUND 8 30","PC_CFG_SET2 1 60 1 3 35 2","PC_SOUND -1 30","PC_EVT 1 1 4294967297","PC_EVT 1 +1 1"};
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
    uint32_t old[]={600,2,2,55};memcpy(legacy,old,sizeof(old));have_legacy=true;have_stored=false;reboot();
    line("PC_CFG_GET","PC_CFG 1 600 1 3 55\n");assert(config_dirty);
    line("PC_CFG_SET 40 600 1 3 55","PC_CFG_STATE 40 1\n");run();assert(!config_dirty&&stored.done_tone==1&&stored.quota_tone==3);
    /* New flag persists; old clients must preserve it while saving other fields. */
    line("PC_CFG_SET2 41 60 6 7 40 0","PC_CFG_STATE 41 1\n");run();
    unsigned before_boot=audio_samples;reboot();assert(audio_samples==before_boot);
    line("PC_CFG_GET2","PC_CFG 2 60 6 7 40 0\n");
    line("PC_CFG_SET 42 120 4 5 50","PC_CFG_STATE 42 1\n");run();
    line("PC_CFG_GET2","PC_CFG 2 120 4 5 50 0\n");
    fail_commit=true;line("PC_CFG_SET2 43 120 4 5 50 1","PC_CFG_STATE 43 1\n");run();
    assert(config.startup_sound==0&&stored.startup_sound==0);fail_commit=false;
    line("PC_CFG_SET2 44 120 4 5 50 1","PC_CFG_STATE 44 1\n");run();
    before_boot=audio_samples;reboot();assert(audio_samples==before_boot+startup_sound_samples+1920);
    device_runtime_tick(1,NULL);assert(!device_runtime_key(2));run();
    assert(audio_samples==before_boot+startup_sound_samples+1920); /* Waking never replays startup. */
    line("PC_CFG_SET2 45 120 4 5 0 1","PC_CFG_STATE 45 1\n");run();before_boot=audio_samples;reboot();assert(audio_samples==before_boot);
    line("PC_PAGES_GET3","PC_PAGES 3 63 0 1 4 2 3 5\n");
    line("PC_PAGES_GET2","PC_PAGES 2 31 0 1 4 2 3\n");
    stored_pages=(page_config_t){12,{3,2,1,0,0}};have_pages=true;pages_size=20;reboot();
    line("PC_PAGES_GET2","PC_PAGES 2 28 3 2 1 0 4\n");
    stored_pages=(page_config_t){16,{4,3,2,1,0,0}};pages_size=24;reboot();line("PC_PAGES_GET3","PC_PAGES 3 48 4 3 2 1 0 5\n");
    have_pages=false;pages_size=sizeof(page_config_t);reboot();
    line("PC_PAGES_GET","PC_PAGES 1 15 0 1 2 3\n");
    const char *bad_pages[]={"PC_PAGES_SET 100 0 0 1 2 3","PC_PAGES_SET 100 16 0 1 2 3","PC_PAGES_SET 100 15 0 1 2 2","PC_PAGES_SET 100 15 0 1 2 4","PC_PAGES_SET 0 15 0 1 2 3"};
    for(unsigned i=0;i<sizeof(bad_pages)/sizeof(*bad_pages);++i)line(bad_pages[i],"PC_CFG_INVALID\n");
    line("PC_PAGES_SET 100 12 3 2 1 0","PC_CFG_STATE 100 1\n");
    line("PC_PAGES_SET 100 12 3 2 1 0","PC_CFG_STATE 100 1\n");assert(count==1);
    line("PC_CFG_SET2 101 60 1 3 10 1","PC_CFG_BUSY\n");
    line("PC_PAGES_GET","PC_PAGES 1 15 0 1 2 3\n");run();reboot();
    line("PC_PAGES_GET","PC_PAGES 1 12 3 2 1 0\n");line("PC_CFG_GET2","PC_CFG 2 120 4 5 0 1\n");
    fail_commit=true;line("PC_PAGES_SET 102 1 0 1 2 3","PC_CFG_STATE 102 1\n");run();
    line("PC_CFG_STATUS","PC_CFG_STATE 102 3\n");line("PC_PAGES_GET","PC_PAGES 1 12 3 2 1 0\n");fail_commit=false;reboot();
    line("PC_PAGES_GET","PC_PAGES 1 12 3 2 1 0\n");
    line("PC_CFG_SET2 103 30 4 5 100 0","PC_CFG_STATE 103 1\n");run();reboot();
    line("PC_PAGES_GET","PC_PAGES 1 12 3 2 1 0\n");line("PC_CFG_GET2","PC_CFG 2 30 4 5 100 0\n");
    /* Hardware changes are immediate in GET, coalesce previews, and write once after quiet. */
    fake_time_us=200000000;unsigned base_writes=writes,base_formats=format_calls;
    device_runtime_volume_set(95,200000);device_runtime_tick(200000,NULL);assert(count==1);
    device_runtime_volume_set(90,200100);device_runtime_tick(200100,NULL);
    device_runtime_volume_set(85,200200);device_runtime_tick(200200,NULL);assert(count==1&&writes==base_writes);
    line("PC_CFG_GET2","PC_CFG 2 30 4 5 85 0\n");fake_time_us=200200000;run();assert(format_calls==base_formats+1&&last_audio_volume==85);
    device_runtime_volume_set(80,200500);device_runtime_tick(201199,NULL);assert(count==0);
    fake_time_us=201200000;device_runtime_tick(201200,NULL);assert(count==1);run();assert(format_calls==base_formats+2);
    device_runtime_volume_set(75,201300);device_runtime_tick(202299,NULL);assert(writes==base_writes&&count==0);
    fake_time_us=202300000;device_runtime_tick(202300,NULL);assert(count==1);run();assert(writes==base_writes+1&&stored.volume==75&&!volume_unsaved);
    reboot();line("PC_CFG_GET2","PC_CFG 2 30 4 5 75 0\n");line("PC_PAGES_GET","PC_PAGES 1 12 3 2 1 0\n");
    /* Muting cancels queued previews, and mutes active playback at the next PCM chunk. */
    base_formats=format_calls;device_runtime_volume_set(70,203000);device_runtime_tick(203000,NULL);assert(count==1);
    device_runtime_volume_set(0,203010);fake_time_us=203010000;run();assert(format_calls==base_formats);
    fake_time_us=204010000;device_runtime_tick(204010,NULL);run();assert(stored.volume==0);
    unsigned before_preview=audio_samples;device_runtime_volume_set(35,205000);device_runtime_tick(205000,NULL);
    fake_time_us=205000000;mute_at=audio_samples+320;run();assert(last_audio_volume==0&&audio_samples-before_preview==2240&&device_runtime_volume()==0);
    fake_time_us=206000000;device_runtime_tick(206000,NULL);run();assert(stored.volume==0);
    /* Failure preserves live volume and retries, without hammering Flash every tick. */
    device_runtime_volume_set(55,207000);fake_time_us=208000000;fail_commit=true;device_runtime_tick(208000,NULL);run();
    assert(volume_unsaved&&stored.volume==0&&device_runtime_volume()==55);device_runtime_tick(209999,NULL);assert(count==0);
    fake_time_us=210000000;fail_commit=false;device_runtime_tick(210000,NULL);run();assert(stored.volume==55&&!volume_unsaved);
    reboot();line("PC_CFG_GET2","PC_CFG 2 30 4 5 55 0\n");
    /* A newer physical edit wins over an already queued browser save; other fields still save. */
    line("PC_CFG_SET2 110 60 6 7 20 1","PC_CFG_STATE 110 1\n");device_runtime_volume_set(65,211010);fake_time_us=211010000;run();
    line("PC_CFG_GET2","PC_CFG 2 60 6 7 65 1\n");assert(stored.volume==65&&!volume_unsaved);
    /* Edits arriving during a commit survive and receive a later consolidated save. */
    line("PC_CFG_SET2 111 120 4 5 50 0","PC_CFG_STATE 111 1\n");inject_volume=80;fake_time_us=212000000;run();
    line("PC_CFG_GET2","PC_CFG 2 120 4 5 80 0\n");assert(stored.volume==50&&volume_unsaved);
    fake_time_us=213000000;device_runtime_tick(213000,NULL);run();reboot();
    line("PC_CFG_GET2","PC_CFG 2 120 4 5 80 0\n");line("PC_PAGES_GET","PC_PAGES 1 12 3 2 1 0\n");
    /* Preview bank selection: selected completion, quota fallback, then supplied Codex. */
    line("PC_CFG_SET2 112 120 6 7 80 0","PC_CFG_STATE 112 1\n");run();device_runtime_volume_set(75,214000);fake_time_us=214000000;before_preview=audio_samples;device_runtime_tick(214000,NULL);run();assert(audio_samples-before_preview==41472+1920);
    line("PC_CFG_SET2 113 120 0 7 75 0","PC_CFG_STATE 113 1\n");run();device_runtime_volume_set(70,216000);fake_time_us=216000000;before_preview=audio_samples;device_runtime_tick(216000,NULL);run();assert(audio_samples-before_preview==17664+1920);
    line("PC_CFG_SET2 114 120 0 0 70 0","PC_CFG_STATE 114 1\n");run();device_runtime_volume_set(65,218000);fake_time_us=218000000;before_preview=audio_samples;device_runtime_tick(218000,NULL);run();assert(audio_samples-before_preview==9134+1920);
    expected_pcm=NULL;assert(device_runtime_test(909,2));run();assert(diagnostic_result_id==909&&diagnostic_result_ok&&diagnostic_result_samples==6400);
    assert(!device_runtime_test(910,0));
    puts("Volume preview coalescing/rate limit/mute, delayed persistence, failure retries and concurrent web saves: PASS");
    monitor_config_t custom=monitor_default();custom.cards[0].metric=M_TODAY;custom.cards[1].enabled=0;custom.bits=1;custom.decimals=2;
    strcpy(custom.cards[0].title,"今日流量");char hex[MONITOR_CONFIG_BYTES*2+1],command[400];monitor_encode(&custom,hex);
    snprintf(command,sizeof(command),"PC_MON_SET 150 %s",hex);line(command,"PC_CFG_STATE 150 1\n");run();reboot();
    monitor_config_t got=device_runtime_monitor();assert(!memcmp(&got,&custom,sizeof(got)));assert(config.idle_seconds==120&&config.startup_sound==0);assert(pages.enabled==60);
    fail_commit=true;custom.bits=0;monitor_encode(&custom,hex);snprintf(command,sizeof(command),"PC_MON_SET 151 %s",hex);line(command,"PC_CFG_STATE 151 1\n");run();
    assert(device_runtime_monitor().bits==1);fail_commit=false;reboot();assert(device_runtime_monitor().bits==1);
    assert(device_runtime_line("PC_MON_GET",reply,sizeof(reply))>0&&strstr(reply,"PC_MON 1 ")==reply);
    assert(device_runtime_line("PC_MON_READ 0",reply,sizeof(reply))>0&&strstr(reply,"PC_MON_DATA ")==reply);
    line("PC_MON_READ 148","PC_CFG_INVALID\n");
    puts("Independent monitor configuration, chunked reads, NVS persistence and save failure: PASS");
    line("PC_PAGES_SET2 9876 16 4 3 2 1 0","PC_CFG_STATE 9876 1\n");run();reboot();
    line("PC_PAGES_GET2","PC_PAGES 2 16 4 3 2 1 0\n");
    line("PC_PAGES_SET2 9877 32 4 3 2 1 0","PC_CFG_INVALID\n");
    line("PC_KEYS_GET","PC_KEYS 2 1 2 3 0 4 0 0 0 5\n");
    line("PC_KEYS_SET 9880 2 1 0 4 0 3 1 2 0","PC_CFG_STATE 9880 1\n");run();reboot();
    line("PC_KEYS_GET","PC_KEYS 2 2 1 0 4 0 3 1 0 0\n");
    line("PC_KEYS_SET 9881 5 1 0 4 0 3 1 2 0","PC_CFG_INVALID\n");
    fail_commit=true;line("PC_KEYS_SET 9882 1 2 3 0 4 0 0 0 5","PC_CFG_STATE 9882 1\n");run();fail_commit=false;reboot();
    line("PC_KEYS_GET","PC_KEYS 2 2 1 0 4 0 3 1 0 0\n");
    nvs_ready=false;line("PC_CFG_SET 30 60 1 3 10","PC_CFG_UNAVAILABLE\n");
    fail_task=true;assert(!device_runtime_start());line("PC_CFG_GET","PC_CFG_UNAVAILABLE\n");
    puts("Settings worker, persistence failures, audio bounds, wake and event deduplication: PASS");
    return 0;
}
