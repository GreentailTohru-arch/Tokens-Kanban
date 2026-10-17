/* Production diagnostics worker/protocol with deterministic BSP/NVS mocks. */
#include <assert.h>
#include <setjmp.h>
#include "../main/device_diagnostics.c"
static diagnostic_request_t jobs[2];static unsigned jobs_count,writes,audio_requests;static bool fail_commit,have_name;
static char stored_name[33],staged_name[33];static uint64_t clock_us=1000000,idle_us=100000;
static jmp_buf yielded;
int64_t esp_timer_get_time(void){return (int64_t)clock_us;}
uint64_t ulTaskGetIdleRunTimeCounter(void){return idle_us;}
uint32_t esp_rom_get_cpu_ticks_per_us(void){return 160;}
size_t heap_caps_get_total_size(unsigned c){(void)c;return 200000;}
size_t heap_caps_get_free_size(unsigned c){(void)c;return 80000;}
size_t heap_caps_get_minimum_free_size(unsigned c){(void)c;return 60000;}
size_t heap_caps_get_largest_free_block(unsigned c){(void)c;return 40000;}
esp_err_t esp_flash_get_size(void *chip,uint32_t *n){(void)chip;*n=8388608;return ESP_OK;}
const esp_partition_t *esp_partition_find_first(int type,int subtype,const char *label){(void)subtype;(void)label;static esp_partition_t a={0x770000},p={0x80000};return type==0?&a:&p;}
void esp_chip_info(esp_chip_info_t *c){*c=(esp_chip_info_t){1,101};}
esp_err_t esp_read_mac(uint8_t *m,int type){(void)type;memcpy(m,"\x98\xc3\x77\xf4\xde\xb0",6);return ESP_OK;}
int esp_app_get_elf_sha256(char *out,size_t n){snprintf(out,n,"137ca20932c9064f22eecf763333ae2e0437d2fc5cdce9622db0401ceb7c608f");return 64;}
int bsp_battery_soc(void){return 86;}int bsp_battery_mv(void){return 4050;}int bsp_button_read_mv(void){return 3300;}
esp_err_t nvs_get_stats(const char *part,nvs_stats_t *s){(void)part;*s=(nvs_stats_t){12,500};return ESP_OK;}
esp_err_t nvs_open(const char *ns,int mode,nvs_handle_t *h){(void)mode;assert(!strcmp(ns,"tokens_ui"));*h=1;return ESP_OK;}
esp_err_t nvs_get_str(nvs_handle_t h,const char *key,char *out,size_t *size){(void)h;assert(!strcmp(key,"name1"));if(!have_name)return ESP_FAIL;assert(*size>=strlen(stored_name)+1);strcpy(out,stored_name);*size=strlen(out)+1;return ESP_OK;}
esp_err_t nvs_set_str(nvs_handle_t h,const char *key,const char *value){(void)h;assert(!strcmp(key,"name1"));strcpy(staged_name,value);writes++;return ESP_OK;}
esp_err_t nvs_commit(nvs_handle_t h){(void)h;if(fail_commit)return ESP_FAIL;strcpy(stored_name,staged_name);have_name=true;return ESP_OK;}
void nvs_close(nvs_handle_t h){(void)h;}
QueueHandle_t xQueueCreate(unsigned n,size_t size){assert(n==2&&size==sizeof(diagnostic_request_t));return jobs;}
void vQueueDelete(QueueHandle_t q){(void)q;}
int xQueueSend(QueueHandle_t q,const void *data,unsigned wait){(void)wait;assert(q==jobs);if(jobs_count==2)return 0;jobs[jobs_count++]=*(diagnostic_request_t *)data;return 1;}
int xQueueReceive(QueueHandle_t q,void *out,unsigned wait){(void)wait;assert(q==jobs);if(!jobs_count)longjmp(yielded,1);*(diagnostic_request_t *)out=jobs[0];memmove(jobs,jobs+1,--jobs_count*sizeof(*jobs));return 1;}
int xTaskCreate(void (*f)(void *),const char *name,unsigned stack,void *arg,unsigned pri,void *task){(void)f;(void)name;(void)stack;(void)arg;(void)pri;(void)task;return pdPASS;}
bool device_runtime_test(unsigned id,unsigned kind){(void)id;assert(kind==2||kind==3);audio_requests++;return true;}
static void run(void){if(!setjmp(yielded))diagnostic_worker(NULL);}
static char reply[96];
static void line(const char *command,const char *expected){size_t n=device_diagnostics_line(command,reply,sizeof(reply));assert(n==strlen(reply)&&n<sizeof(reply));if(strcmp(reply,expected)){fprintf(stderr,"%s: got %s expected %s",command,reply,expected);assert(0);}}
int main(void){
    assert(device_diagnostics_start(true,true));line("PC_INFO","PC_INFO 1\n");sample();assert(snapshot.cpu==-1);clock_us+=1000000;idle_us+=600000;sample();assert(snapshot.cpu==400);
    line("PC_INFO_SYSTEM","PC_SYSTEM 2000 400 160 200000 80000 60000 40000\n");
    line("PC_INFO_STORAGE","PC_STORAGE 8388608 7798784 524288 12 500\n");line("PC_INFO_BATTERY","PC_BATTERY 86 4050 500\n");
    line("PC_INFO_NAME","PC_NAME 596f7527732041492050617373706f7274\n");
    line("PC_NAME_SET 1 4d792050617373706f7274","PC_NAME_STATE 1 1\n");line("PC_NAME_SET 1 4d792050617373706f7274","PC_NAME_STATE 1 1\n");assert(jobs_count==1);
    line("PC_NAME_SET 2 4149","PC_NAME_BUSY\n");run();assert(writes==1);line("PC_INFO_NAME","PC_NAME 4d792050617373706f7274\n");
    fail_commit=true;line("PC_NAME_SET 3 4149","PC_NAME_STATE 3 1\n");run();line("PC_NAME_STATE","PC_NAME_STATE 3 3\n");line("PC_INFO_NAME","PC_NAME 4d792050617373706f7274\n");fail_commit=false;
    strcpy(device_name,DEVICE_DEFAULT_NAME);assert(device_diagnostics_start(true,true));assert(!strcmp(device_name,"My Passport"));
    line("PC_NAME_SET -1 4149","PC_NAME_INVALID\n");line("PC_NAME_SET 5 00","PC_NAME_INVALID\n");line("PC_NAME_SET 5 e4b8","PC_NAME_INVALID\n");
    line("PC_TEST 20 2","PC_TEST_STATE 20 1 2 -1 -1 0\n");line("PC_TEST 20 2","PC_TEST_STATE 20 1 2 -1 -1 0\n");assert(audio_requests==1);
    line("PC_TEST 21 3","PC_TEST_BUSY\n");device_diagnostics_test_complete(999,true,1,1,6400);assert(test_state==1);device_diagnostics_test_complete(20,true,123,45,6400);line("PC_TEST_STATE","PC_TEST_STATE 20 2 2 123 45 6400\n");
    line("PC_TEST 20 2","PC_TEST_STATE 20 2 2 123 45 6400\n");assert(audio_requests==1);
    line("PC_TEST 22 1","PC_TEST_STATE 22 1 1 -1 -1 0\n");run();line("PC_TEST_STATE","PC_TEST_STATE 22 2 1 -1 -1 0\n");
    has_battery=false;sample();line("PC_INFO_BATTERY","PC_BATTERY -1 -1 500\n");line("PC_TEST 23 1","PC_TEST_STATE 23 1 1 -1 -1 0\n");run();line("PC_TEST_STATE","PC_TEST_STATE 23 3 1 -1 -1 0\n");
    device_diagnostics_key(0,BSP_BTN_PRESS);device_diagnostics_key(0,BSP_BTN_RELEASE);line("PC_INFO_KEYS","PC_KEYS 3300 1 0 0 0 4\n");
    line("PC_SCREEN 3","PC_SCREEN_OK\n");assert(device_diagnostics_screen(2000)==3);assert(device_diagnostics_screen_stop());assert(device_diagnostics_screen(2001)==-1);
    line("PC_SCREEN 4","PC_SCREEN_OK\n");assert(device_diagnostics_screen(17000)==-1);line("PC_SCREEN 5","PC_SCREEN_OK\n");assert(device_diagnostics_screen(2000)==5);line("PC_SCREEN 6","PC_SCREEN_OK\n");assert(device_diagnostics_screen(2000)==6);line("PC_SCREEN 7","PC_SCREEN_INVALID\n");line("PC_SCREEN -1","PC_SCREEN_OK\n");
    puts("Actual diagnostics worker: name persistence/failure/dedup, telemetry, missing battery, test serialization, key counters and screen expiry PASS");return 0;
}
