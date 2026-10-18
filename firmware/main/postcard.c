#include "postcard.h"
#include "postcard_codec.h"
#include "token_ui.h"
#include "bsp_display.h"
#include "esp_partition.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define SLOT_SIZE 0x40000u
#define DATA_START 0x1000u
#define MAGIC 0x504f5331u
#define META_MAGIC 0x50435431u
#define META_MAX 512u
typedef struct { uint32_t magic,length,hash; } metadata_t;
typedef struct { uint32_t magic,generation,length,hash; } header_t;
typedef struct { unsigned command,id,offset,hash; char data[520]; } request_t;
static const esp_partition_t *partition;
static const uint8_t *mapped;
static esp_partition_mmap_handle_t mapping;
static QueueHandle_t queue;
static atomic_int active_slot=-1;
static uint32_t generation;
static portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
/* state: 0 idle, 1 worker busy, 2 receiving, 3 committed, 4 failed. */
static unsigned state,transfer_id,received_bytes;
static int64_t touched,clock_at;
static uint64_t clock_seconds;
static bool clock_set,clock_dated;
static bool publish(const uint8_t *data) {
    if(!bsp_lvgl_lock(100)) return false;
    token_ui_postcard_image(data);bsp_lvgl_unlock();return true;
}
static bool valid_slot(unsigned slot,header_t *header) {
    memcpy(header,mapped+slot*SLOT_SIZE,sizeof(*header));
    return header->magic==MAGIC && header->length==POSTCARD_BYTES &&
        postcard_hash(2166136261u,mapped+slot*SLOT_SIZE+DATA_START,POSTCARD_BYTES)==header->hash;
}
static void worker(void *arg) {
    (void)arg;request_t req;static unsigned target,expected_hash,meta_length;static uint8_t meta[META_MAX];uint8_t bytes[POSTCARD_BLOCK_BYTES];
    for(;;) {
        if(xQueueReceive(queue,&req,portMAX_DELAY)!=pdTRUE) continue;
        unsigned next=received_bytes,new_state=2;esp_err_t err=ESP_OK;
        if(req.command==1) {
            target=atomic_load(&active_slot)==0?1:0;expected_hash=req.hash;next=0;meta_length=0;
            err=esp_partition_erase_range(partition,target*SLOT_SIZE,SLOT_SIZE);
        } else if(req.command==2) {
            size_t count=0;
            if(!postcard_decode(req.data,bytes,sizeof(bytes),&count) || req.offset!=next ||
                count>POSTCARD_BYTES-next) err=ESP_ERR_INVALID_ARG;
            else {
                err=esp_partition_write(partition,target*SLOT_SIZE+DATA_START+next,bytes,count);
                if(err==ESP_OK) next+=(unsigned)count;
            }
        } else if(req.command==4) {
            size_t count=0;
            if(!postcard_decode(req.data,bytes,sizeof(bytes),&count) || req.offset!=meta_length || count>META_MAX-meta_length) err=ESP_ERR_INVALID_ARG;
            else {memcpy(meta+meta_length,bytes,count);meta_length+=(unsigned)count;}
        } else {
            uint32_t hash=postcard_hash(2166136261u,mapped+target*SLOT_SIZE+DATA_START,POSTCARD_BYTES);
            if(next!=POSTCARD_BYTES || hash!=expected_hash) err=ESP_ERR_INVALID_CRC;
            else {
                header_t h={MAGIC,generation+1,POSTCARD_BYTES,hash};
                /* Validity marker is written last; incomplete uploads never replace the active card. */
                metadata_t m={META_MAGIC,meta_length,postcard_hash(2166136261u,meta,meta_length)};
                if(meta_length) {
                    err=esp_partition_write(partition,target*SLOT_SIZE+32,&m,sizeof(m));
                    if(err==ESP_OK)err=esp_partition_write(partition,target*SLOT_SIZE+44,meta,meta_length);
                }
                if(err==ESP_OK)err=esp_partition_write(partition,target*SLOT_SIZE+4,((uint8_t *)&h)+4,sizeof(h)-4);
                if(err==ESP_OK) err=esp_partition_write(partition,target*SLOT_SIZE,&h.magic,4);
                if(err==ESP_OK) {
                    /* Main/UI must stop reading old slot before another upload may erase it. */
                    while(!publish(mapped+target*SLOT_SIZE+DATA_START)) vTaskDelay(pdMS_TO_TICKS(10));
                    generation=h.generation;atomic_store(&active_slot,(int)target);new_state=3;
                }
            }
        }
        if(err!=ESP_OK) { new_state=4;ESP_LOGW("postcard","Upload failed: %s",esp_err_to_name(err)); }
        portENTER_CRITICAL(&mux);received_bytes=next;state=new_state;touched=esp_timer_get_time();portEXIT_CRITICAL(&mux);
    }
}
bool postcard_start(void) {
    partition=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,0x40,"postcard");
    if(!partition || partition->size!=2*SLOT_SIZE) return false;
    if(esp_partition_mmap(partition,0,partition->size,ESP_PARTITION_MMAP_DATA,(const void **)&mapped,&mapping)!=ESP_OK) return false;
    header_t a,b;bool va=valid_slot(0,&a),vb=valid_slot(1,&b);
    int slot=vb&&(!va||b.generation>a.generation)?1:va?0:-1;
    atomic_store(&active_slot,slot);generation=slot==1?b.generation:slot==0?a.generation:0;
    if(slot>=0 && !publish(mapped+slot*SLOT_SIZE+DATA_START)) return false;
    queue=xQueueCreate(1,sizeof(request_t));
    if(!queue) return false;
    if(xTaskCreate(worker,"postcard",4096,NULL,3,NULL)==pdPASS) return true;
    vQueueDelete(queue);queue=NULL;return false;
}
size_t postcard_line(const char *line,char *reply,size_t capacity) {
    if(strncmp(line,"PC_",3)) return 0;
    if(!queue) return snprintf(reply,capacity,"PC_UNAVAILABLE\n");
    if(!strcmp(line,"PC_HELLO")) return snprintf(reply,capacity,"PC_READY 1\n");
    if(!strcmp(line,"PC_CARD_INFO") || !strncmp(line,"PC_CARD_READ ",13) || !strncmp(line,"PC_CARD_META ",13)) {
        /* Serialize against uploads so a committed slot cannot be erased mid-read. */
        portENTER_CRITICAL(&mux);unsigned st=state;portEXIT_CRITICAL(&mux);
        if(st==1 || st==2)return snprintf(reply,capacity,"PC_CARD_BUSY\n");
        int slot=atomic_load(&active_slot);header_t h={0};metadata_t m={0};
        const uint8_t *base=slot<0?NULL:mapped+(unsigned)slot*SLOT_SIZE;
        if(base){memcpy(&h,base,sizeof(h));memcpy(&m,base+32,sizeof(m));}
        if(m.magic!=META_MAGIC || m.length>META_MAX || !m.length || postcard_hash(2166136261u,base+44,m.length)!=m.hash)m.length=0;
        if(!strcmp(line,"PC_CARD_INFO"))return snprintf(reply,capacity,"PC_CARD_INFO 1 %u %u %u %u\n",(unsigned)h.generation,(unsigned)h.length,(unsigned)h.hash,(unsigned)m.length);
        unsigned gen,off;int end=0;bool meta_read=!strncmp(line,"PC_CARD_META ",13);
        if(sscanf(line+13,"%u %u%n",&gen,&off,&end)!=2 || line[13+end] || !base || gen!=h.generation)return snprintf(reply,capacity,"PC_CARD_CHANGED\n");
        unsigned len=meta_read?m.length:h.length;
        if(off>=len || capacity<240)return snprintf(reply,capacity,"PC_INVALID\n");
        unsigned n=len-off;if(n>96)n=96;size_t used=(size_t)snprintf(reply,capacity,"PC_CARD_DATA %u ",off);
        const uint8_t *data=base+(meta_read?44:DATA_START)+off;
        for(unsigned i=0;i<n;i++)used+=(size_t)snprintf(reply+used,capacity-used,"%02x",data[i]);
        return used+(size_t)snprintf(reply+used,capacity-used,"\n");
    }
    unsigned sec;int end=0;
    if(sscanf(line,"PC_TIME %u%n",&sec,&end)==1 && !line[end] && sec<86400) {
        portENTER_CRITICAL(&mux);clock_seconds=sec;clock_at=esp_timer_get_time();clock_set=true;clock_dated=false;portEXIT_CRITICAL(&mux);
        return snprintf(reply,capacity,"PC_TIME_OK\n");
    }
    unsigned long long local_seconds;end=0;
    if(sscanf(line,"PC_CLOCK %llu%n",&local_seconds,&end)==1 && !line[end] &&
        local_seconds>=946684800ULL && local_seconds<4102444800ULL) {
        portENTER_CRITICAL(&mux);clock_seconds=local_seconds;clock_at=esp_timer_get_time();clock_set=true;clock_dated=true;portEXIT_CRITICAL(&mux);
        return snprintf(reply,capacity,"PC_CLOCK_OK\n");
    }
    request_t req={0};bool status=!strcmp(line,"PC_STATUS");
    end=0;
    if(sscanf(line,"PC_BEGIN %u %u%n",&req.id,&req.hash,&end)==2 && !line[end]) req.command=1;
    else {
        end=0;
        if(sscanf(line,"PC_DATA %u %u %519s%n",&req.id,&req.offset,req.data,&end)==3 && !line[end]) req.command=2;
        else {end=0;if(sscanf(line,"PC_DONE %u%n",&req.id,&end)==1 && !line[end]) req.command=3;}
    }
    if(!req.command){end=0;if(sscanf(line,"PC_META %u %u %519s%n",&req.id,&req.offset,req.data,&end)==3 && !line[end])req.command=4;}
    if(!status && !req.command) return snprintf(reply,capacity,"PC_INVALID\n");
    portENTER_CRITICAL(&mux);
    bool begin=req.command==1 && state!=1 && (state!=2 || esp_timer_get_time()-touched>60000000);
    bool next=req.command>1 && state==2 && req.id==transfer_id &&
        (req.command==4 || (req.command==3?received_bytes==POSTCARD_BYTES:req.offset==received_bytes));
    unsigned old_state=state,old_id=transfer_id,old_count=received_bytes;
    if(begin || next) {state=1;if(begin) {transfer_id=req.id;received_bytes=0;}}
    unsigned st=state,id=transfer_id,count=received_bytes;
    portEXIT_CRITICAL(&mux);
    if(begin || next) {
        if(xQueueSend(queue,&req,0)!=pdTRUE) {
            portENTER_CRITICAL(&mux);state=old_state;transfer_id=old_id;received_bytes=old_count;portEXIT_CRITICAL(&mux);
            return snprintf(reply,capacity,"PC_RETRY\n");
        }
    }
    return snprintf(reply,capacity,"PC_STATE %u %u %u\n",id,st,count);
}
void postcard_update_ui(bool contact) {
    portENTER_CRITICAL(&mux);
    uint64_t seconds=clock_set?clock_seconds+(uint64_t)((esp_timer_get_time()-clock_at)/1000000):0;
    bool dated=clock_set && clock_dated;
    portEXIT_CRITICAL(&mux);
    token_ui_postcard_clock(seconds,dated,contact);
}
