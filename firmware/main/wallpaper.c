#include "wallpaper.h"
#include "postcard_codec.h"
#include "token_ui.h"
#include "bsp_display.h"
#include "esp_partition.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdio.h>
#include <string.h>
#define SLOT 0x40000u
#define START 0x1000u
#define BYTES (240u*320u*2u)
#define MAGIC 0x31505754u
typedef struct {unsigned magic,generation,length,hash,flags;} header_t;
typedef struct {unsigned command,id,offset,length,hash,flags;char data[520];} request_t;
static const esp_partition_t *partition;
static const unsigned char *mapped;
static esp_partition_mmap_handle_t mapping;
static QueueHandle_t queue;
static portMUX_TYPE mutex=portMUX_INITIALIZER_UNLOCKED;
static header_t current={.flags=4};
static int active=-1;
static unsigned state,id,received;
static int64_t touched;
static bool valid(unsigned slot,header_t *h){
    memcpy(h,mapped+slot*SLOT,sizeof(*h));
    return h->magic==MAGIC && (h->length==0||h->length==BYTES) && h->flags<8 &&
        h->hash==postcard_hash(2166136261u,mapped+slot*SLOT+START,h->length);
}
static void publish(const unsigned char *data,unsigned flags){
    while(!bsp_lvgl_lock(100))vTaskDelay(pdMS_TO_TICKS(10));
    token_ui_wallpaper(data,flags);bsp_lvgl_unlock();
}
static void worker(void *arg){
    (void)arg;request_t r;static header_t pending;static unsigned target,next;unsigned char bytes[256];
    for(;;){
        if(xQueueReceive(queue,&r,portMAX_DELAY)!=pdTRUE)continue;
        esp_err_t error=ESP_OK;unsigned result=2;
        if(r.command==4){
            target=active==0?1:0;pending=current;pending.magic=MAGIC;pending.generation++;pending.flags=r.flags;
            pending.hash=pending.length?pending.hash:2166136261u;next=pending.length;
            error=esp_partition_erase_range(partition,target*SLOT,SLOT);
            for(unsigned at=0;error==ESP_OK&&at<next;at+=sizeof(bytes)){
                unsigned n=next-at;if(n>sizeof(bytes))n=sizeof(bytes);
                memcpy(bytes,mapped+active*SLOT+START+at,n);error=esp_partition_write(partition,target*SLOT+START+at,bytes,n);
            }
            if(error==ESP_OK){error=esp_partition_write(partition,target*SLOT+4,((const unsigned char *)&pending)+4,sizeof(pending)-4);
                if(error==ESP_OK)error=esp_partition_write(partition,target*SLOT,&pending.magic,4);
                if(error==ESP_OK){publish(next?mapped+target*SLOT+START:NULL,pending.flags);portENTER_CRITICAL(&mutex);active=(int)target;current=pending;portEXIT_CRITICAL(&mutex);result=3;}}
        }else if(r.command==1){
            target=active==0?1:0;next=0;pending=(header_t){MAGIC,current.generation+1,r.length,r.hash,r.flags};
            error=esp_partition_erase_range(partition,target*SLOT,SLOT);
        }else if(r.command==2){
            size_t count=0;
            if(!postcard_decode(r.data,bytes,sizeof(bytes),&count)||!count||r.offset!=next||count>pending.length-next)error=ESP_ERR_INVALID_ARG;
            else{error=esp_partition_write(partition,target*SLOT+START+next,bytes,count);if(error==ESP_OK)next+=count;}
        }else{
            const unsigned char *data=mapped+target*SLOT+START;
            if(next!=pending.length||postcard_hash(2166136261u,data,next)!=pending.hash)error=ESP_ERR_INVALID_CRC;
            else{
                error=esp_partition_write(partition,target*SLOT+4,((const unsigned char *)&pending)+4,sizeof(pending)-4);
                if(error==ESP_OK)error=esp_partition_write(partition,target*SLOT,&pending.magic,4);
                if(error==ESP_OK){publish(next?data:NULL,pending.flags);portENTER_CRITICAL(&mutex);active=(int)target;current=pending;portEXIT_CRITICAL(&mutex);result=3;}
            }
        }
        portENTER_CRITICAL(&mutex);received=next;state=error==ESP_OK?result:4;touched=esp_timer_get_time();portEXIT_CRITICAL(&mutex);
    }
}
bool wallpaper_start(void){
    partition=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,0x42,"wallpaper");
    if(!partition||partition->size!=2*SLOT)return false;
    if(esp_partition_mmap(partition,0,partition->size,ESP_PARTITION_MMAP_DATA,(const void **)&mapped,&mapping)!=ESP_OK)return false;
    header_t a,b;bool va=valid(0,&a),vb=valid(1,&b);active=vb&&(!va||b.generation>a.generation)?1:va?0:-1;
    if(active>=0){current=active?b:a;publish(current.length?mapped+active*SLOT+START:NULL,current.flags);}
    queue=xQueueCreate(1,sizeof(request_t));if(!queue)return false;
    if(xTaskCreate(worker,"wallpaper",4096,NULL,3,NULL)==pdPASS)return true;
    vQueueDelete(queue);queue=NULL;return false;
}
size_t wallpaper_line(const char *line,char *reply,size_t cap){
    if(strncmp(line,"PC_WALL",7))return 0;
    if(!queue)return snprintf(reply,cap,"PC_WALL_UNAVAILABLE\n");
    if(!strcmp(line,"PC_WALL_GET")){portENTER_CRITICAL(&mutex);header_t h=current;portEXIT_CRITICAL(&mutex);return snprintf(reply,cap,"PC_WALL 1 %u %u %u\n",h.length,h.length?h.hash:2166136261u,h.flags);}
    unsigned offset;int end=0;
    if(sscanf(line,"PC_WALL_READ %u%n",&offset,&end)==1&&!line[end]){
        portENTER_CRITICAL(&mutex);
        if(active<0||offset>=current.length){portEXIT_CRITICAL(&mutex);return snprintf(reply,cap,"PC_WALL_INVALID\n");}
        unsigned n=current.length-offset;if(n>96)n=96;
        if(cap<32+n*2){portEXIT_CRITICAL(&mutex);return 0;}
        size_t used=(size_t)snprintf(reply,cap,"PC_WALL_DATA %u ",offset);
        for(unsigned i=0;i<n;++i)used+=(size_t)snprintf(reply+used,cap-used,"%02x",mapped[active*SLOT+START+offset+i]);
        portEXIT_CRITICAL(&mutex);return used+(size_t)snprintf(reply+used,cap-used,"\n");
    }
    request_t r={0};bool status=!strcmp(line,"PC_WALL_STATUS");end=0;
    if(sscanf(line,"PC_WALL_BEGIN %u %u %u %u%n",&r.id,&r.length,&r.hash,&r.flags,&end)==4&&!line[end]&&r.id&&(r.length==0||r.length==BYTES)&&r.flags<8)r.command=1;
    else{end=0;if(sscanf(line,"PC_WALL_OPTIONS %u %u%n",&r.id,&r.flags,&end)==2&&!line[end]&&r.id&&r.flags<8)r.command=4;
      else{end=0;if(sscanf(line,"PC_WALL_DATA %u %u %519s%n",&r.id,&r.offset,r.data,&end)==3&&!line[end])r.command=2;
        else{end=0;if(sscanf(line,"PC_WALL_DONE %u%n",&r.id,&end)==1&&!line[end])r.command=3;}}
    }
    if(!status&&!r.command)return snprintf(reply,cap,"PC_WALL_INVALID\n");
    portENTER_CRITICAL(&mutex);
    bool begin=(r.command==1||r.command==4)&&state!=1&&(state!=2||esp_timer_get_time()-touched>60000000);
    bool next=(r.command==2||r.command==3)&&state==2&&r.id==id&&(r.command==3||r.offset==received);
    if(begin||next){unsigned old=state,old_id=id,old_count=received;state=1;if(begin){id=r.id;received=0;}touched=esp_timer_get_time();if(xQueueSend(queue,&r,0)!=pdTRUE){state=old;id=old_id;received=old_count;}}
    unsigned s=state,i=id,n=received;portEXIT_CRITICAL(&mutex);
    return snprintf(reply,cap,"PC_WALL_STATE %u %u %u\n",i,s,n);
}
