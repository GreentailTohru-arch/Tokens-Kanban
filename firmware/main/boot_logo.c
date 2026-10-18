#include "boot_logo.h"
#include "boot_logo_model.h"
#include "postcard_codec.h"
#include "esp_partition.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define SLOT 0x20000u
#define START 0x1000u
#define MAGIC 0x31474c54u
typedef struct {uint32_t magic,generation,length,hash;} header_t;
typedef struct {unsigned command,id,offset,length,hash;char data[520];} request_t;
static const esp_partition_t *partition;
static const uint8_t *mapped,*boot_data;
static esp_partition_mmap_handle_t mapping;
static QueueHandle_t queue;
static int active=-1;
static uint32_t generation,active_length,active_hash;
static unsigned state,transfer_id,received,total,expected,target;
static int64_t touched;
static portMUX_TYPE mutex=portMUX_INITIALIZER_UNLOCKED;
static atomic_bool playing;
static boot_logo_index_t boot_index;
static uint8_t *pixels;
static int last_frame=-1;
static bool valid(unsigned slot,header_t *h){memcpy(h,mapped+slot*SLOT,sizeof(*h));return h->magic==MAGIC&&h->length<=BOOT_LOGO_MAX_BYTES&&h->hash==postcard_hash(2166136261u,mapped+slot*SLOT+START,h->length)&&(!h->length||boot_logo_validate(mapped+slot*SLOT+START,h->length,NULL));}
static void worker(void *unused){
    (void)unused;request_t r;uint8_t bytes[256];
    for(;;){if(xQueueReceive(queue,&r,portMAX_DELAY)!=pdTRUE)continue;esp_err_t error=ESP_OK;unsigned next=received,result=2;
        if(r.command==1){target=active==0?1:0;next=0;total=r.length;expected=r.hash;error=esp_partition_erase_range(partition,target*SLOT,SLOT);}
        else if(r.command==2){size_t n=0;if(!postcard_decode(r.data,bytes,sizeof(bytes),&n)||r.offset!=next||n>total-next)error=ESP_ERR_INVALID_ARG;else{error=esp_partition_write(partition,target*SLOT+START+next,bytes,n);if(error==ESP_OK)next+=n;}}
        else{const uint8_t *data=mapped+target*SLOT+START;
            if(next!=total||postcard_hash(2166136261u,data,total)!=expected||(total&&!boot_logo_validate(data,total,NULL)))error=ESP_ERR_INVALID_CRC;
            else{header_t h={MAGIC,generation+1,total,expected};error=esp_partition_write(partition,target*SLOT+4,((uint8_t *)&h)+4,sizeof(h)-4);if(error==ESP_OK)error=esp_partition_write(partition,target*SLOT,&h.magic,4);
                if(error==ESP_OK){active=(int)target;generation=h.generation;portENTER_CRITICAL(&mutex);active_length=total;active_hash=expected;portEXIT_CRITICAL(&mutex);result=3;}}
        }
        portENTER_CRITICAL(&mutex);received=next;state=error==ESP_OK?result:4;touched=esp_timer_get_time();portEXIT_CRITICAL(&mutex);
    }
}
bool boot_logo_start(void){
    partition=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,0x41,"bootlogo");if(!partition||partition->size!=SLOT*2)return false;
    if(esp_partition_mmap(partition,0,partition->size,ESP_PARTITION_MMAP_DATA,(const void **)&mapped,&mapping)!=ESP_OK)return false;
    header_t a,b;bool va=valid(0,&a),vb=valid(1,&b);active=vb&&(!va||b.generation>a.generation)?1:va?0:-1;
    if(active>=0){header_t h=active?b:a;generation=h.generation;active_length=h.length;active_hash=h.hash;
        if(h.length&&boot_logo_validate(mapped+active*SLOT+START,h.length,&boot_index)){pixels=malloc(boot_index.side*boot_index.height*2u);if(pixels){boot_data=mapped+active*SLOT+START;atomic_store(&playing,true);}}}
    queue=xQueueCreate(1,sizeof(request_t));if(!queue)return false;
    if(xTaskCreate(worker,"boot_logo",4096,NULL,3,NULL)==pdPASS)return true;
    vQueueDelete(queue);queue=NULL;return false;
}
const uint8_t *boot_logo_frame(uint64_t elapsed,unsigned *side){
    if(!pixels)return NULL;
    /* Play once, then hold the final frame while startup audio finishes. */
    unsigned at=elapsed>=boot_index.duration?boot_index.duration-1:(unsigned)elapsed,i=0;
    while(i+1<boot_index.count&&at>=boot_index.frames[i].duration){at-=boot_index.frames[i].duration;++i;}
    if((int)i==last_frame)return NULL;
    const boot_frame_t *f=&boot_index.frames[i];
    if(!boot_logo_decode(boot_data+f->offset,f->length,pixels,boot_index.side*boot_index.height*2u))return NULL;
    last_frame=(int)i;*side=boot_index.side;return pixels;
}
void boot_logo_finish(void){if(pixels){free(pixels);pixels=NULL;}atomic_store(&playing,false);}
size_t boot_logo_line(const char *line,char *reply,size_t cap){
    if(strncmp(line,"PC_LOGO",7))return 0;
    if(!queue)return snprintf(reply,cap,"PC_LOGO_UNAVAILABLE\n");
    if(!strcmp(line,"PC_LOGO_CAPS"))return snprintf(reply,cap,"PC_LOGO_CAPS 2 240 320\n");
    if(!strcmp(line,"PC_LOGO_GET")){portENTER_CRITICAL(&mutex);unsigned n=active_length,h=active_hash;portEXIT_CRITICAL(&mutex);return snprintf(reply,cap,"PC_LOGO 1 %u %u\n",n,h);}
    bool status=!strcmp(line,"PC_LOGO_STATUS");request_t r={0};int end=0;
    if(!strncmp(line,"PC_LOGO_BEGIN ",14)&&line[14]>='0'&&line[14]<='9'&&sscanf(line,"PC_LOGO_BEGIN %u %u %u%n",&r.id,&r.length,&r.hash,&end)==3&&!line[end]&&r.id&&r.length<=BOOT_LOGO_MAX_BYTES)r.command=1;
    else if(sscanf(line,"PC_LOGO_DATA %u %u %519s%n",&r.id,&r.offset,r.data,&end)==3&&!line[end])r.command=2;
    else if(sscanf(line,"PC_LOGO_DONE %u%n",&r.id,&end)==1&&!line[end])r.command=3;
    if(!r.command&&!status)return snprintf(reply,cap,"PC_LOGO_INVALID\n");
    portENTER_CRITICAL(&mutex);bool begin=r.command==1&&state!=1&&!atomic_load(&playing)&&(state!=2||esp_timer_get_time()-touched>60000000);
    bool next=r.command>1&&state==2&&r.id==transfer_id&&(r.command==3?received==total:r.offset==received);
    if(begin||next){if(begin){transfer_id=r.id;received=0;}state=1;touched=esp_timer_get_time();if(xQueueSend(queue,&r,0)!=pdTRUE)state=4;}
    unsigned id=transfer_id,s=state,n=received;portEXIT_CRITICAL(&mutex);
    return snprintf(reply,cap,"PC_LOGO_STATE %u %u %u\n",id,s,n);
}

unsigned boot_logo_height(void){return boot_index.height;}
unsigned boot_logo_duration(void){return pixels&&boot_index.count>1?boot_index.duration:1200;}
