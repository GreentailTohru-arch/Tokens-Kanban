#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_INVALID_ARG 1
#define ESP_ERR_INVALID_CRC 2
static inline const char *esp_err_to_name(esp_err_t e) {(void)e;return "injected";}
#define ESP_LOGW(...) ((void)0)
typedef struct {size_t size;} esp_partition_t;
typedef unsigned esp_partition_mmap_handle_t;
#define ESP_PARTITION_TYPE_DATA 1
#define ESP_PARTITION_MMAP_DATA 0
const esp_partition_t *esp_partition_find_first(int type,int subtype,const char *name);
esp_err_t esp_partition_mmap(const esp_partition_t *,size_t,size_t,int,const void **,esp_partition_mmap_handle_t *);
esp_err_t esp_partition_write(const esp_partition_t *,size_t,const void *,size_t);
esp_err_t esp_partition_erase_range(const esp_partition_t *,size_t,size_t);
int64_t esp_timer_get_time(void);
typedef void *QueueHandle_t;
typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(p) ((void)(p))
#define portEXIT_CRITICAL(p) ((void)(p))
#define pdTRUE 1
#define pdPASS 1
#define pdMS_TO_TICKS(n) (n)
#define portMAX_DELAY 0xffffffffu
QueueHandle_t xQueueCreate(unsigned,size_t);
void vQueueDelete(QueueHandle_t);
int xQueueReceive(QueueHandle_t,void *,unsigned);
int xQueueSend(QueueHandle_t,const void *,unsigned);
int xTaskCreate(void (*)(void *),const char *,unsigned,void *,unsigned,void *);
void vTaskDelay(unsigned);
bool bsp_lvgl_lock(unsigned);
void bsp_lvgl_unlock(void);
