#include <assert.h>
#include <setjmp.h>
#include "../main/wallpaper.c"
static unsigned char flash[2*SLOT];
static esp_partition_t part={sizeof(flash)};
static request_t request;
static bool queued,fail_write;
static unsigned shown_flags;
static const unsigned char *shown;
static jmp_buf yielded;
const esp_partition_t *esp_partition_find_first(int type,int subtype,const char *name){assert(type==1&&subtype==0x42&&!strcmp(name,"wallpaper"));return &part;}
esp_err_t esp_partition_mmap(const esp_partition_t *p,size_t off,size_t n,int kind,const void **out,esp_partition_mmap_handle_t *h){(void)kind;(void)h;assert(p==&part&&off==0&&n==sizeof(flash));*out=flash;return ESP_OK;}
esp_err_t esp_partition_write(const esp_partition_t *p,size_t off,const void *data,size_t n){assert(p==&part&&off+n<=sizeof(flash));if(fail_write)return ESP_FAIL;memcpy(flash+off,data,n);return ESP_OK;}
esp_err_t esp_partition_erase_range(const esp_partition_t *p,size_t off,size_t n){assert(p==&part&&off+n<=sizeof(flash));memset(flash+off,255,n);return ESP_OK;}
int64_t esp_timer_get_time(void){return 1000;}
QueueHandle_t xQueueCreate(unsigned n,size_t size){assert(n==1&&size==sizeof(request));return &request;}
void vQueueDelete(QueueHandle_t q){(void)q;}
int xQueueSend(QueueHandle_t q,const void *data,unsigned wait){(void)q;(void)wait;if(queued)return 0;request=*(const request_t *)data;queued=true;return 1;}
int xQueueReceive(QueueHandle_t q,void *out,unsigned wait){(void)q;(void)wait;if(!queued)longjmp(yielded,1);*(request_t *)out=request;queued=false;return 1;}
int xTaskCreate(void (*fn)(void *),const char *name,unsigned stack,void *arg,unsigned priority,void *out){(void)name;(void)stack;(void)arg;(void)priority;(void)out;assert(fn==worker);return 1;}
void vTaskDelay(unsigned ticks){(void)ticks;}
bool bsp_lvgl_lock(unsigned timeout){(void)timeout;return true;}
void bsp_lvgl_unlock(void){}
void token_ui_wallpaper(const uint8_t *data,unsigned flags){shown=data;shown_flags=flags;}
static void run(void){if(!setjmp(yielded))worker(NULL);}
static void line(const char *in){char out[600];assert(wallpaper_line(in,out,sizeof(out)));}
int main(void){
    memset(flash,255,sizeof(flash));assert(wallpaper_start());assert(active==-1&&current.flags==4);
    line("PC_WALL_BEGIN 1 1 0 0");assert(!queued);
    line("PC_WALL_OPTIONS 1 16");assert(!queued);
    line("PC_WALL_OPTIONS 1 3");run();assert(state==3&&shown==NULL&&shown_flags==3);
    static unsigned char pixels[BYTES];for(unsigned i=0;i<BYTES;++i)pixels[i]=(unsigned char)i;
    unsigned h=postcard_hash(2166136261u,pixels,sizeof(pixels));char cmd[620];snprintf(cmd,sizeof(cmd),"PC_WALL_BEGIN 2 %u %u 7",BYTES,h);line(cmd);run();
    line("PC_WALL_DATA 999 0 ff0000");assert(!queued);
    for(unsigned offset=0;offset<BYTES;offset+=256){size_t n=(size_t)snprintf(cmd,sizeof(cmd),"PC_WALL_DATA 2 %u 7f",offset);for(unsigned i=0;i<256;++i)n+=(size_t)snprintf(cmd+n,sizeof(cmd)-n,"%02x",pixels[offset+i]);line(cmd);run();assert(state==2&&received==offset+256);}
    line("PC_WALL_DONE 2");run();assert(state==3&&shown_flags==7&&!memcmp(shown,pixels,BYTES));
    char response[256];size_t reply_length=wallpaper_line("PC_WALL_READ 0",response,sizeof(response));assert(reply_length>0&&reply_length<256&&!strncmp(response,"PC_WALL_DATA 0 ",15));
    int old=active;line("PC_WALL_OPTIONS 3 12");run();assert(state==3&&active!=old&&shown_flags==12&&!memcmp(shown,pixels,BYTES));
    old=active;fail_write=true;line("PC_WALL_OPTIONS 4 0");run();assert(state==4&&active==old&&shown_flags==12);fail_write=false;
    /* Incomplete upload and bad hash must never displace the committed image. */
    line("PC_WALL_BEGIN 5 0 0 0");run();line("PC_WALL_DONE 5");run();assert(state==4&&active==old);
    assert(wallpaper_start());assert(active==old&&shown_flags==12&&!memcmp(shown,pixels,BYTES));
    line("PC_WALL_BEGIN 6 0 2166136261 0");run();line("PC_WALL_DONE 6");run();assert(state==3&&shown==NULL&&shown_flags==0);
    assert(wallpaper_start());assert(current.length==0&&current.flags==0);
    puts("Wallpaper: image/readback, flags-only copy, interrupted/corrupt writes and reboot persistence PASS");return 0;
}
