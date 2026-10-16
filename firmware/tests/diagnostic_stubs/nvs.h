#include "platform.h"
typedef unsigned nvs_handle_t;
#define NVS_READWRITE 1
#define NVS_READONLY 0
typedef struct {unsigned used_entries,free_entries;} nvs_stats_t;
esp_err_t nvs_open(const char *,int,nvs_handle_t *);
esp_err_t nvs_get_str(nvs_handle_t,const char *,char *,size_t *);
esp_err_t nvs_set_str(nvs_handle_t,const char *,const char *);
esp_err_t nvs_commit(nvs_handle_t);
void nvs_close(nvs_handle_t);
esp_err_t nvs_get_stats(const char *,nvs_stats_t *);
