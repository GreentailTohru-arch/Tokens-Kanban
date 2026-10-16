#pragma once
#include "system_model.h"
#define MONITOR_SLOTS 6
#define MONITOR_CONFIG_BYTES 148
enum { M_CPU,M_GPU,M_RAM,M_CPU_TEMP,M_GPU_TEMP,M_CPU_FREQ,M_HDD,M_UP,M_DOWN,M_SPEED,M_TODAY,M_PAIR,M_METRICS,M_NONE=255 };
typedef struct { uint8_t metric,enabled;char title[19]; } monitor_card_t;
typedef struct {
    uint8_t version,bits,unit,show_unit,show_percent,space,decimals,compact;
    char up_label[7],down_label[7];
    monitor_card_t cards[MONITOR_SLOTS];
} monitor_config_t;
_Static_assert(sizeof(monitor_config_t)==MONITOR_CONFIG_BYTES,"Stable config encoding");
monitor_config_t monitor_default(void);
bool monitor_valid(const monitor_config_t *config);
bool monitor_decode(const char *hex,monitor_config_t *out);
void monitor_encode(const monitor_config_t *config,char *out);
uint32_t monitor_hash(const monitor_config_t *config);
unsigned monitor_count(const monitor_config_t *config);
const char *monitor_title(const monitor_card_t *card);
void monitor_format(const monitor_config_t *config,unsigned metric,const system_snapshot_t *s,bool live,char *first,char *second,size_t size);
void monitor_layout(unsigned count,unsigned index,int *x,int *y,int *w,int *h);
