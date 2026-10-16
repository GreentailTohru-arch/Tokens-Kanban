/* Fault injection against the actual upload worker and flash state machine. */
#include <assert.h>
#include <setjmp.h>
#include "../main/postcard.c"
static uint8_t flash[0x80000];
static esp_partition_t part={sizeof(flash)};
static request_t pending;
static bool queued,fail_write,fail_erase,fail_task;
static jmp_buf yielded;
static int64_t test_time;
static unsigned shown_seconds,shown_month,shown_day,shown_weekday;
static bool shown_dated;
static const uint8_t *shown;
const esp_partition_t *esp_partition_find_first(int type,int sub,const char *name) {
    assert(type==1 && sub==0x40 && !strcmp(name,"postcard"));return &part;
}
esp_err_t esp_partition_mmap(const esp_partition_t *p,size_t off,size_t size,int type,const void **out,esp_partition_mmap_handle_t *h) {
    (void)type;assert(p==&part && off==0 && size==sizeof(flash));*out=flash;*h=1;return ESP_OK;
}
esp_err_t esp_partition_write(const esp_partition_t *p,size_t off,const void *data,size_t n) {
    assert(p==&part && off+n<=sizeof(flash));if(fail_write)return ESP_FAIL;
    for(size_t i=0;i<n;i++){assert((flash[off+i]&((const uint8_t *)data)[i])==((const uint8_t *)data)[i]);flash[off+i]&=((const uint8_t *)data)[i];}return ESP_OK;
}
esp_err_t esp_partition_erase_range(const esp_partition_t *p,size_t off,size_t n) {
    assert(p==&part && off+n<=sizeof(flash) && off%4096==0 && n%4096==0);
    if(fail_erase)return ESP_FAIL;
    memset(flash+off,255,n);return ESP_OK;
}
int64_t esp_timer_get_time(void){return test_time;}
QueueHandle_t xQueueCreate(unsigned n,size_t size){assert(n==1 && size==sizeof(pending));return &pending;}
void vQueueDelete(QueueHandle_t q){assert(q==&pending);}
int xQueueSend(QueueHandle_t q,const void *data,unsigned wait){(void)wait;assert(q==&pending && !queued);memcpy(&pending,data,sizeof(pending));queued=true;return 1;}
int xQueueReceive(QueueHandle_t q,void *data,unsigned wait){(void)wait;assert(q==&pending);if(!queued)longjmp(yielded,1);memcpy(data,&pending,sizeof(pending));queued=false;return 1;}
int xTaskCreate(void (*f)(void *),const char *name,unsigned stack,void *arg,unsigned priority,void *out){(void)f;(void)name;(void)stack;(void)arg;(void)priority;(void)out;return fail_task?0:1;}
void vTaskDelay(unsigned n){(void)n;}
bool bsp_lvgl_lock(unsigned n){(void)n;return true;}
void bsp_lvgl_unlock(void){}
void token_ui_postcard_image(const uint8_t *data){shown=data;}
void token_ui_postcard_clock(uint64_t seconds,bool dated,bool contact){(void)contact;shown_seconds=seconds%86400;shown_dated=dated;if(dated)assert(postcard_calendar(seconds,&shown_month,&shown_day,&shown_weekday));}
static void run(void){if(!setjmp(yielded))worker(NULL);}
static char reply[64];
static void line(const char *s){assert(postcard_line(s,reply,sizeof(reply))>0);}
static void reboot(void){state=transfer_id=received_bytes=0;queued=false;clock_set=false;shown=NULL;assert(postcard_start());}
static void upload(unsigned id,unsigned color,bool bad_hash){
    uint8_t raw[256];for(unsigned i=0;i<256;i+=2){raw[i]=color;raw[i+1]=0;}
    uint32_t h=2166136261u;for(unsigned i=0;i<POSTCARD_BYTES;i+=256)h=postcard_hash(h,raw,256);
    char command[100];snprintf(command,sizeof(command),"PC_BEGIN %u %u",id,h+(bad_hash?1:0));line(command);run();assert(state==2);
    for(unsigned off=0;off<POSTCARD_BYTES;off+=256){snprintf(command,sizeof(command),"PC_DATA %u %u ff%02x00",id,off,color);line(command);run();assert(received_bytes==off+256 && state==2);}
    snprintf(command,sizeof(command),"PC_DONE %u",id);line(command);run();
}
int main(void){
    uint8_t out[256];size_t count=0;
    assert(postcard_decode("ff3412",out,sizeof(out),&count)&&count==256&&out[254]==0x34&&out[255]==0x12);
    assert(postcard_decode("0134127856",out,sizeof(out),&count)&&count==4&&out[2]==0x78);
    const char *bad[]={"","0","00","0000","zz0000","ff341200","ff3412ff3412","ff3412000000"};
    for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++)assert(!postcard_decode(bad[i],out,sizeof(out),&count));
    memset(flash,255,sizeof(flash));reboot();assert(atomic_load(&active_slot)==-1 && !shown);
    line("PC_TIME 86390");test_time=20000000;postcard_update_ui(false);assert(shown_seconds==10);
    line("PC_TIME 86400");assert(!strcmp(reply,"PC_INVALID\n"));

    assert(!shown_dated);
    const char *bad_clock[]={"PC_CLOCK -1","PC_CLOCK 0","PC_CLOCK 946684799","PC_CLOCK 4102444800","PC_CLOCK 1792108800 extra"};
    for(unsigned i=0;i<sizeof(bad_clock)/sizeof(bad_clock[0]);++i){line(bad_clock[i]);assert(!strcmp(reply,"PC_INVALID\n"));}
    line("PC_CLOCK 1792195199");assert(!strcmp(reply,"PC_CLOCK_OK\n"));test_time+=2000000;postcard_update_ui(false);assert(shown_dated&&shown_seconds==1&&shown_month==10&&shown_day==17&&shown_weekday==6);
    line("PC_CLOCK 1835395199");assert(!strcmp(reply,"PC_CLOCK_OK\n"));test_time+=2000000;postcard_update_ui(false);assert(shown_dated&&shown_seconds==1&&shown_month==2&&shown_day==29&&shown_weekday==2);
    line("PC_CLOCK 1835481599");assert(!strcmp(reply,"PC_CLOCK_OK\n"));test_time+=2000000;postcard_update_ui(false);assert(shown_dated&&shown_seconds==1&&shown_month==3&&shown_day==1&&shown_weekday==3);
    line("PC_CLOCK 1798761599");assert(!strcmp(reply,"PC_CLOCK_OK\n"));test_time+=2000000;postcard_update_ui(false);assert(shown_dated&&shown_seconds==1&&shown_month==1&&shown_day==1&&shown_weekday==5);
    line("PC_TIME 1");postcard_update_ui(false);assert(!shown_dated);
    unsigned month,day,weekday;
    assert(postcard_calendar(0ULL,&month,&day,&weekday)&&month==1&&day==1&&weekday==4);
    assert(postcard_calendar(951782400ULL,&month,&day,&weekday)&&month==2&&day==29&&weekday==2);
    assert(postcard_calendar(4107542400ULL,&month,&day,&weekday)&&month==3&&day==1&&weekday==1);
    assert(postcard_calendar(7258032000ULL,&month,&day,&weekday)&&month==12&&day==31&&weekday==2);
    assert(!postcard_calendar(7258118400ULL,&month,&day,&weekday));
    upload(1,0x22,false);assert(state==3&&atomic_load(&active_slot)==0&&shown[0]==0x22);
    reboot();assert(shown[0]==0x22);postcard_update_ui(false);assert(shown_seconds==0);
    upload(2,0x33,true);assert(state==4&&atomic_load(&active_slot)==0&&shown[0]==0x22);
    reboot();assert(shown[0]==0x22);
    line("PC_BEGIN 3 1");run();line("PC_DATA 4 0 ff4400");assert(!queued);
    line("PC_DATA 3 256 ff4400");assert(!queued);
    line("PC_DATA 3 0 ff4400");run();assert(received_bytes==256);
    line("PC_DATA 3 0 ff4400");assert(!queued&&received_bytes==256);
    line("PC_DONE 3");assert(!queued);reboot();assert(shown[0]==0x22);
    fail_erase=true;line("PC_BEGIN 5 0");run();assert(state==4&&shown[0]==0x22);fail_erase=false;
    line("PC_BEGIN 6 0");run();fail_write=true;line("PC_DATA 6 0 ff5500");run();assert(state==4&&shown[0]==0x22);fail_write=false;
    upload(7,0x66,false);assert(state==3&&atomic_load(&active_slot)==1&&shown[0]==0x66);
    reboot();assert(shown[0]==0x66);
    fail_task=true;assert(!postcard_start() && !queue);line("PC_HELLO");assert(!strcmp(reply,"PC_UNAVAILABLE\n"));
    puts("Postcard codec, ordering, interrupted upload, flash failures, persistence and clock: PASS");
    return 0;
}
