/* Production storage worker: interrupted writes must never replace the committed logo. */
#include <assert.h>
#include <setjmp.h>
#include "../main/boot_logo.c"
static uint8_t flash[0x40000];static esp_partition_t part={sizeof(flash)};static request_t pending;static bool queued,fail_write,fail_erase;static int64_t test_time;static jmp_buf yielded;
const esp_partition_t *esp_partition_find_first(int type,int sub,const char *name){assert(type==1&&sub==0x41&&!strcmp(name,"bootlogo"));return &part;}
esp_err_t esp_partition_mmap(const esp_partition_t *p,size_t off,size_t size,int type,const void **out,esp_partition_mmap_handle_t *h){(void)type;assert(p==&part&&!off&&size==sizeof(flash));*out=flash;*h=1;return ESP_OK;}
esp_err_t esp_partition_write(const esp_partition_t *p,size_t off,const void *data,size_t n){assert(p==&part&&off+n<=sizeof(flash));if(fail_write)return ESP_FAIL;for(size_t i=0;i<n;++i){assert((flash[off+i]&((const uint8_t *)data)[i])==((const uint8_t *)data)[i]);flash[off+i]&=((const uint8_t *)data)[i];}return ESP_OK;}
esp_err_t esp_partition_erase_range(const esp_partition_t *p,size_t off,size_t n){assert(p==&part&&off+n<=sizeof(flash)&&off%4096==0&&n%4096==0);if(fail_erase)return ESP_FAIL;memset(flash+off,255,n);return ESP_OK;}
int64_t esp_timer_get_time(void){return test_time;}
QueueHandle_t xQueueCreate(unsigned n,size_t size){assert(n==1&&size==sizeof(pending));return &pending;}
void vQueueDelete(QueueHandle_t q){assert(q==&pending);}
int xQueueSend(QueueHandle_t q,const void *r,unsigned wait){(void)wait;assert(q==&pending&&!queued);pending=*(const request_t *)r;queued=true;return 1;}
int xQueueReceive(QueueHandle_t q,void *r,unsigned wait){(void)wait;assert(q==&pending);if(!queued)longjmp(yielded,1);*(request_t *)r=pending;queued=false;return 1;}
int xTaskCreate(void (*f)(void *),const char *name,unsigned stack,void *arg,unsigned priority,void *out){(void)f;(void)name;(void)stack;(void)arg;(void)priority;(void)out;return 1;}
static void run(void){if(!setjmp(yielded))worker(NULL);}
static char reply[96];static void line(const char *s){assert(boot_logo_line(s,reply,sizeof(reply))>0);}
static void reboot(void){boot_logo_finish();state=transfer_id=received=total=expected=active_length=active_hash=generation=0;last_frame=-1;queued=false;boot_data=NULL;assert(boot_logo_start());}
static uint8_t blob[]={84,76,71,49,16,0,1,0,100,0,6,0,0,0,255,0x34,0x12,255,0x34,0x12};
static void upload(unsigned id,bool corrupt){char command[180],hex[sizeof(blob)*4+1];uint32_t h=postcard_hash(2166136261u,blob,sizeof(blob));snprintf(command,sizeof(command),"PC_LOGO_BEGIN %u %u %u",id,(unsigned)sizeof(blob),h+(corrupt?1:0));line(command);run();assert(state==2);for(unsigned i=0;i<sizeof(blob);i+=2)snprintf(hex+i*3,7,"00%02x%02x",blob[i],blob[i+1]);snprintf(command,sizeof(command),"PC_LOGO_DATA %u 0 %s",id,hex);line(command);run();assert(state==2&&received==sizeof(blob));snprintf(command,sizeof(command),"PC_LOGO_DONE %u",id);line(command);run();}
int main(void){
#ifdef _WIN32
 _set_error_mode(_OUT_TO_STDERR);
#endif

 boot_logo_index_t index;uint8_t raw[512];assert(boot_logo_validate(blob,sizeof(blob),&index)&&index.side==16&&index.count==1&&index.duration==100);assert(boot_logo_decode(blob+14,6,raw,sizeof(raw))&&raw[0]==0x34&&raw[511]==0x12);
 for(size_t n=0;n<sizeof(blob);++n)assert(!boot_logo_validate(blob,n,NULL));
 blob[10]=7;assert(!boot_logo_validate(blob,sizeof(blob),NULL));blob[10]=6;blob[4]=255;assert(!boot_logo_validate(blob,sizeof(blob),NULL));blob[4]=16;
 memset(flash,255,sizeof(flash));reboot();assert(!pixels);upload(1,false);assert(state==3&&active==0);reboot();assert(pixels);unsigned side=0;assert(boot_logo_frame(0,&side)&&side==16&&pixels[0]==0x34);line("PC_LOGO_BEGIN 2 20 0");assert(!queued);boot_logo_finish();upload(2,true);assert(state==4&&active==0);reboot();assert(pixels&&active==0);boot_logo_finish();
 line("PC_LOGO_BEGIN 3 20 0");run();line("PC_LOGO_DATA 3 0 00544c");run();reboot();assert(active==0&&pixels);boot_logo_finish();fail_erase=true;line("PC_LOGO_BEGIN 4 20 0");run();assert(state==4&&active==0);fail_erase=false;upload(5,false);assert(state==3&&active==1);
 reboot();boot_logo_finish();line("PC_LOGO_BEGIN 6 0 2166136261");run();fail_write=true;line("PC_LOGO_DONE 6");run();assert(state==4&&active==1);fail_write=false;reboot();assert(pixels);boot_logo_finish();line("PC_LOGO_BEGIN 7 0 2166136261");run();line("PC_LOGO_DONE 7");run();assert(state==3);reboot();assert(!pixels&&active_length==0);
 /* TLG2 portrait frames validate independently from legacy square data. */
 uint8_t portrait[32]={84,76,71,50,24,0,1,0,100,0,18,0,0,0};
 for(unsigned i=14;i<32;i+=3){portrait[i]=255;portrait[i+1]=0x34;portrait[i+2]=0x12;}
 assert(boot_logo_validate(portrait,sizeof(portrait),&index)&&index.side==24&&index.height==32);
 portrait[4]=25;assert(!boot_logo_validate(portrait,sizeof(portrait),NULL));portrait[4]=24;
 portrait[3]='1';assert(!boot_logo_validate(portrait,sizeof(portrait),NULL));
 /* Two frames: exact boundary, delayed startup, and long audio must not loop. */
 uint8_t animation[]={255,0x11,0x11,255,0x11,0x11,255,0x22,0x22,255,0x22,0x22};
 boot_data=animation;pixels=malloc(512);assert(pixels);last_frame=-1;
 boot_index=(boot_logo_index_t){.side=16,.height=16,.count=2,.duration=200,
     .frames={{0,6,100},{6,6,100}}};
 assert(boot_logo_frame(0,&side)&&pixels[0]==0x11);
 assert(!boot_logo_frame(99,&side));
 assert(boot_logo_frame(100,&side)&&pixels[0]==0x22);
 assert(!boot_logo_frame(200,&side)&&pixels[0]==0x22);
 assert(!boot_logo_frame(6000,&side)&&pixels[0]==0x22);
 last_frame=-1;assert(boot_logo_frame(UINT64_MAX,&side)&&pixels[0]==0x22);
 boot_logo_finish();
 puts("Boot logo validation, RLE, interruption, flash errors, boot lock, persistence, one-shot animation and default restore: PASS");return 0;
}
