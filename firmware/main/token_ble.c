#include "token_ble.h"
#include "token_model.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_att.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include <string.h>

/* 9e7a0001-4b51-4a4c-9b50-2f53f077c301, RX=0002, TX=0003. */
#define UUID(n) BLE_UUID128_INIT(0x01,0xc3,0x77,0xf0,0x53,0x2f,0x50,0x9b,0x4c,0x4a,0x51,0x4b,n,0x00,0x7a,0x9e)
static const ble_uuid128_t service_uuid=UUID(1), rx_uuid=UUID(2), tx_uuid=UUID(3);
static uint16_t tx_handle;
static uint8_t address_type;
static token_stream_t stream;
static int64_t last_write;
static token_ble_receive_t receiver;
static const char *TAG="token_ble";
static int gap_event(struct ble_gap_event *event,void *arg);

static void reply_notify(uint16_t connection,const char *data,size_t len) {
    size_t limit=ble_att_mtu(connection)-3;
    if(limit<1) return;
    for(size_t pos=0;pos<len;) {
        size_t n=len-pos; if(n>limit) n=limit;
        struct os_mbuf *om=ble_hs_mbuf_from_flat(data+pos,n);
        if(!om) return;
        /* NimBLE consumes the mbuf on success and failure. */
        if(ble_gatts_notify_custom(connection,tx_handle,om)) return;
        pos+=n;
    }
}
static int access_value(uint16_t connection,uint16_t attribute,struct ble_gatt_access_ctxt *ctxt,void *arg) {
    (void)attribute; (void)arg;
    if(ctxt->op==BLE_GATT_ACCESS_OP_READ_CHR)
        return os_mbuf_append(ctxt->om,"CT_READY 4\n",11)==0?0:BLE_ATT_ERR_INSUFFICIENT_RES;
    if(ctxt->op!=BLE_GATT_ACCESS_OP_WRITE_CHR) return BLE_ATT_ERR_UNLIKELY;
    uint16_t length=OS_MBUF_PKTLEN(ctxt->om);
    if(!length || length>=TOKEN_FRAME_MAX) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    int64_t now=esp_timer_get_time();
    if(now-last_write>2000000) memset(&stream,0,sizeof(stream));
    last_write=now;
    char bytes[64];
    for(uint16_t pos=0;pos<length;) {
        uint16_t n=length-pos; if(n>sizeof(bytes)) n=sizeof(bytes);
        if(os_mbuf_copydata(ctxt->om,pos,n,bytes)) return BLE_ATT_ERR_UNLIKELY;
        for(uint16_t i=0;i<n;++i) if(token_stream_push(&stream,bytes[i])) {
            char reply[48]; size_t len=receiver(stream.data,reply,sizeof(reply));
            if(len) reply_notify(connection,reply,len);
        }
        pos+=n;
    }
    return 0;
}
static const struct ble_gatt_svc_def services[]={
    {.type=BLE_GATT_SVC_TYPE_PRIMARY,.uuid=&service_uuid.u,.characteristics=(struct ble_gatt_chr_def[]){
        {.uuid=&rx_uuid.u,.access_cb=access_value,.flags=BLE_GATT_CHR_F_WRITE},
        {.uuid=&tx_uuid.u,.access_cb=access_value,.flags=BLE_GATT_CHR_F_READ|BLE_GATT_CHR_F_NOTIFY,.val_handle=&tx_handle},
        {0}}},
    {0}
};
static int advertise(void) {
    struct ble_hs_adv_fields fields={0};
    fields.flags=BLE_HS_ADV_F_DISC_GEN|BLE_HS_ADV_F_BREDR_UNSUP;
    fields.uuids128=(ble_uuid128_t *)&service_uuid; fields.num_uuids128=1; fields.uuids128_is_complete=1;
    int rc=ble_gap_adv_set_fields(&fields); if(rc) return rc;
    struct ble_hs_adv_fields scan={0};
    scan.name=(const uint8_t *)"Codex Passport";scan.name_len=strlen((const char *)scan.name);scan.name_is_complete=1;
    rc=ble_gap_adv_rsp_set_fields(&scan); if(rc) return rc;
    struct ble_gap_adv_params params={0};
    params.conn_mode=BLE_GAP_CONN_MODE_UND;params.disc_mode=BLE_GAP_DISC_MODE_GEN;
    return ble_gap_adv_start(address_type,NULL,BLE_HS_FOREVER,&params,gap_event,NULL);
}
static int gap_event(struct ble_gap_event *event,void *arg) {
    (void)arg;
    bool restart=event->type==BLE_GAP_EVENT_DISCONNECT || event->type==BLE_GAP_EVENT_ADV_COMPLETE;
    if(event->type==BLE_GAP_EVENT_CONNECT) restart=event->connect.status!=0;
    if(event->type==BLE_GAP_EVENT_DISCONNECT || event->type==BLE_GAP_EVENT_CONNECT) {
        memset(&stream,0,sizeof(stream));last_write=0;
    }
    if(restart) { int rc=advertise();if(rc) ESP_LOGW(TAG,"Advertising retry failed: %d",rc); }
    return 0;
}
static void on_sync(void) {
    int rc=ble_hs_util_ensure_addr(0);
    if(!rc) rc=ble_hs_id_infer_auto(0,&address_type);
    if(!rc) rc=advertise();
    if(rc) ESP_LOGE(TAG,"BLE sync failed: %d",rc);
    else ESP_LOGI(TAG,"Codex Passport advertising");
}
static void on_reset(int reason) {
    memset(&stream,0,sizeof(stream));last_write=0;
    ESP_LOGW(TAG,"Host reset: %d",reason);
}
static void host_task(void *arg) {
    (void)arg;nimble_port_run();vTaskDelete(NULL);
}
esp_err_t token_ble_start(token_ble_receive_t receive) {
    if(!receive || receiver) return ESP_ERR_INVALID_STATE;
    esp_err_t err=nvs_flash_init();
    if(err!=ESP_OK) return err; /* Never erase saved data to repair NVS. */
    err=nimble_port_init();if(err!=ESP_OK) return err;
    receiver=receive;
    ble_svc_gap_init();ble_svc_gatt_init();
    int rc=ble_svc_gap_device_name_set("Codex Passport");
    if(!rc) rc=ble_gatts_count_cfg(services);
    if(!rc) rc=ble_gatts_add_svcs(services);
    ble_hs_cfg.reset_cb=on_reset;ble_hs_cfg.sync_cb=on_sync;
    if(!rc && xTaskCreatePinnedToCore(host_task,"token_ble",NIMBLE_HS_STACK_SIZE,NULL,
            configMAX_PRIORITIES-4,NULL,NIMBLE_CORE)==pdPASS) return ESP_OK;
    ESP_LOGE(TAG,"BLE startup failed: %d",rc);
    esp_err_t cleanup=nimble_port_deinit();
    if(cleanup!=ESP_OK) ESP_LOGE(TAG,"BLE cleanup failed: %s",esp_err_to_name(cleanup));
    else receiver=NULL;
    return rc?ESP_FAIL:ESP_ERR_NO_MEM;
}
