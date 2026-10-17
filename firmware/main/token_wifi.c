#include "token_wifi.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "mbedtls/md.h"
#include "network_http.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "token_model.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
static QueueHandle_t commands;
static portMUX_TYPE guard = portMUX_INITIALIZER_UNLOCKED;
static network_config_t config;
static token_wifi_status_t status;
static token_network_receiver_t network_receiver;
static esp_netif_t *station, *access_point;
static bool radio_ready, radio_started, ap_requested, handler_wifi, handler_ip;
static volatile bool got_ip, lost_ip;
static volatile int disconnect_reason;
static uint64_t retry_at, ap_until;
static unsigned attempts;
static int discovery = -1, dns = -1;
static uint64_t now_ms(void) { return (uint64_t)esp_timer_get_time() / 1000; }
void token_wifi_status(token_wifi_status_t *out) {
  portENTER_CRITICAL(&guard);
  *out = status;
  portEXIT_CRITICAL(&guard);
}
void token_wifi_access(char out[33]) {
  uint8_t token[16];
  portENTER_CRITICAL(&guard);
  memcpy(token, config.token, 16);
  portEXIT_CRITICAL(&guard);
  network_hex_encode(token, 16, out);
}
bool token_wifi_authorized(const char *hex) {
  uint8_t token[16];
  portENTER_CRITICAL(&guard);
  memcpy(token, config.token, 16);
  portEXIT_CRITICAL(&guard);
  return network_token_equal(hex, token);
}
void token_wifi_management_url(char *out, size_t cap, bool setup) {
  token_wifi_status_t s;
  token_wifi_status(&s);
  char token[33];
  token_wifi_access(token);
  if (setup || (s.ap && s.state != NET_CONNECTED))
    snprintf(out, cap, "http://192.168.4.1/#%s", token);
  else if (s.state == NET_CONNECTED)
    snprintf(out, cap, "http://%s/#%s", s.ip, token);
  else
    snprintf(out, cap, "https://greentailtohru-arch.github.io/Tokens-Kanban/");
}
static bool save_config(const network_config_t *next) {
  nvs_handle_t n;
  esp_err_t e = nvs_open("tk_network", NVS_READWRITE, &n);
  if (e != ESP_OK)
    return false;
  e = nvs_set_blob(n, "config", next, sizeof(*next));
  if (e == ESP_OK)
    e = nvs_commit(n);
  nvs_close(n);
  return e == ESP_OK;
}
static void publish_config(const network_config_t *next) {
  portENTER_CRITICAL(&guard);
  config = *next;
  status.enabled = next->enabled;
  status.configured = next->ssid[0] != 0;
  memcpy(status.ssid, next->ssid, sizeof(status.ssid));
  portEXIT_CRITICAL(&guard);
}
static void set_state(unsigned state, int reason) {
  portENTER_CRITICAL(&guard);
  status.state = state;
  status.reason = reason;
  portEXIT_CRITICAL(&guard);
}
static void wifi_event(void *arg, esp_event_base_t base, int32_t id,
                       void *data) {
  (void)arg;
  if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = data;
    portENTER_CRITICAL(&guard);
    snprintf(status.ip, sizeof(status.ip), IPSTR, IP2STR(&event->ip_info.ip));
    portEXIT_CRITICAL(&guard);
    got_ip = true;
  }
  if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
    disconnect_reason = ((wifi_event_sta_disconnected_t *)data)->reason;
    lost_ip = true;
  }
}
static void close_socket(int *socket) {
  if (*socket >= 0) {
    close(*socket);
    *socket = -1;
  }
}
static int udp_socket(unsigned port) {
  int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (fd < 0)
    return -1;
  struct sockaddr_in a = {.sin_family = AF_INET,
                          .sin_port = htons((uint16_t)port),
                          .sin_addr.s_addr = htonl(INADDR_ANY)};
  if (bind(fd, (struct sockaddr *)&a, sizeof(a)) < 0) {
    close(fd);
    return -1;
  }
  fcntl(fd, F_SETFL, O_NONBLOCK);
  return fd;
}
static void handle_discovery(void) {
  if (discovery < 0)
    return;
  char packet[96], id[13], nonce[17];
  struct sockaddr_in peer;
  socklen_t length = sizeof(peer);
  int n = recvfrom(discovery, packet, sizeof(packet) - 1, 0,
                   (struct sockaddr *)&peer, &length);
  if (n <= 0)
    return;
  packet[n] = 0;
  int end = 0;
  if (sscanf(packet,
             "TK_DISCOVER 1 %12[0123456789abcdef] %16[0123456789abcdef]%n", id,
             nonce, &end) != 2 ||
      packet[end] || strlen(id) != 12 || strlen(nonce) != 16)
    return;
  token_wifi_status_t s;
  token_wifi_status(&s);
  if (s.state != NET_CONNECTED || strcmp(id, s.id))
    return;
  char message[64], hex[65], answer[160];
  uint8_t digest[32], key[16];
  int size = snprintf(message, sizeof(message), "%s %s %s", id, s.ip, nonce);
  portENTER_CRITICAL(&guard);
  memcpy(key, config.token, 16);
  portEXIT_CRITICAL(&guard);
  if (mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), key,
                      sizeof(key), (const unsigned char *)message, (size_t)size,
                      digest) != 0)
    return;
  network_hex_encode(digest, 32, hex);
  int bytes = snprintf(answer, sizeof(answer), "TK_DEVICE 1 %s v19 %s %s", s.id,
                       s.ip, hex);
  sendto(discovery, answer, (size_t)bytes, 0, (struct sockaddr *)&peer, length);
}
static void handle_dns(void) {
  if (dns < 0)
    return;
  uint8_t p[512];
  struct sockaddr_in peer;
  socklen_t length = sizeof(peer);
  int n = recvfrom(dns, p, sizeof(p), 0, (struct sockaddr *)&peer, &length);
  if (n <= 0)
    return;
  size_t end = network_dns_reply(p, (size_t)n, sizeof(p));
  if (end)
    sendto(dns, p, end, 0, (struct sockaddr *)&peer, length);
}
static void radio_release(void) {
  if (handler_wifi)
    esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
                                 wifi_event);
  if (handler_ip)
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event);
  handler_wifi = handler_ip = false;
  esp_wifi_deinit();
  radio_ready = false;
  if (station)
    esp_netif_destroy_default_wifi(station);
  if (access_point)
    esp_netif_destroy_default_wifi(access_point);
  station = access_point = NULL;
}
static void radio_stop(void) {
  network_http_stop();
  close_socket(&discovery);
  close_socket(&dns);
  if (radio_started) {
    esp_wifi_disconnect();
    esp_wifi_stop();
    radio_started = false;
  }
  got_ip = lost_ip = false;
  portENTER_CRITICAL(&guard);
  status.ap = 0;
  status.ip[0] = 0;
  portEXIT_CRITICAL(&guard);
}
static bool radio_prepare(void) {
  if (radio_ready)
    return true;
  esp_err_t e = esp_netif_init();
  if (e != ESP_OK && e != ESP_ERR_INVALID_STATE)
    return false;
  e = esp_event_loop_create_default();
  if (e != ESP_OK && e != ESP_ERR_INVALID_STATE)
    return false;
  esp_netif_config_t sta_config=ESP_NETIF_DEFAULT_WIFI_STA();
  esp_netif_config_t ap_config=ESP_NETIF_DEFAULT_WIFI_AP();
  station = esp_netif_new(&sta_config);
  access_point = esp_netif_new(&ap_config);
  if (!station || !access_point) {
    radio_release();
    return false;
  }
  if(esp_netif_attach_wifi_station(station)!=ESP_OK||esp_netif_attach_wifi_ap(access_point)!=ESP_OK||
     esp_wifi_set_default_wifi_sta_handlers()!=ESP_OK||esp_wifi_set_default_wifi_ap_handlers()!=ESP_OK){radio_release();return false;}
  wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
  e = esp_wifi_init(&init);
  if (e != ESP_OK) {
    esp_netif_destroy_default_wifi(station);
    esp_netif_destroy_default_wifi(access_point);
    station = access_point = NULL;
    return false;
  }
  handler_wifi =
      esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
                                 wifi_event, NULL) == ESP_OK;
  handler_ip = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                          wifi_event, NULL) == ESP_OK;
  if (!handler_wifi || !handler_ip ||
      esp_wifi_set_storage(WIFI_STORAGE_RAM) != ESP_OK) {
    radio_release();
    return false;
  }
  radio_ready = true;
  return true;
}
static bool radio_apply(void) {
  radio_stop();
  network_config_t c;
  token_wifi_status_t s;
  portENTER_CRITICAL(&guard);
  c = config;
  s = status;
  portEXIT_CRITICAL(&guard);
  if (!c.enabled && !ap_requested) {
    if (radio_ready)
      radio_release();
    set_state(NET_OFF, 0);
    return true;
  }
  if (!radio_prepare()) {
    set_state(NET_UNAVAILABLE, 1);
    return false;
  }
  wifi_mode_t mode = ap_requested ? (c.enabled ? WIFI_MODE_APSTA : WIFI_MODE_AP)
                                  : WIFI_MODE_STA;
  if (esp_wifi_set_mode(mode) != ESP_OK) {
    set_state(NET_UNAVAILABLE, 2);
    return false;
  }
  if (c.enabled) {
    wifi_config_t sta = {0};
    memcpy(sta.sta.ssid, c.ssid, strlen(c.ssid));
    memcpy(sta.sta.password, c.password, strlen(c.password));
    sta.sta.threshold.authmode = WIFI_AUTH_OPEN;
    sta.sta.pmf_cfg.capable = true;
    if (esp_wifi_set_config(WIFI_IF_STA, &sta) != ESP_OK) {
      set_state(NET_UNAVAILABLE, 3);
      return false;
    }
  }
  if (ap_requested) {
    wifi_config_t ap = {0};
    memcpy(ap.ap.ssid, s.ap_ssid, strlen(s.ap_ssid));
    ap.ap.ssid_len = (uint8_t)strlen(s.ap_ssid);
    memcpy(ap.ap.password, s.ap_password, strlen(s.ap_password));
    ap.ap.channel = 1;
    ap.ap.max_connection = 1;
    ap.ap.authmode = WIFI_AUTH_WPA2_PSK;
    if (esp_wifi_set_config(WIFI_IF_AP, &ap) != ESP_OK) {
      set_state(NET_UNAVAILABLE, 4);
      return false;
    }
  }
  if (esp_wifi_start() != ESP_OK) {
    set_state(NET_UNAVAILABLE, 5);
    return false;
  }
  radio_started = true;
  if (!network_http_start(network_receiver)) {
    radio_stop();
    set_state(NET_UNAVAILABLE, 6);
    return false;
  }
  discovery = udp_socket(NETWORK_DISCOVERY_PORT);
  if (ap_requested)
    dns = udp_socket(53);
  portENTER_CRITICAL(&guard);
  status.ap = ap_requested ? 1 : 0;
  portEXIT_CRITICAL(&guard);
  attempts = 0;
  retry_at = now_ms() + 100;
  if (c.enabled)
    set_state(NET_JOINING, 0);
  else
    set_state(NET_SETUP, 0);
  return true;
}
static void process_request(const network_request_t *r) {
  network_config_t next;
  portENTER_CRITICAL(&guard);
  next = config;
  portEXIT_CRITICAL(&guard);
  bool ok = true;
  if (r->command == NET_AP) {
    ap_requested = true;
    ap_until = now_ms() + NETWORK_AP_DURATION_MS;
  } else {
    if (r->command == NET_SET) {
      memcpy(next.ssid, r->config.ssid, sizeof(next.ssid));
      memcpy(next.password, r->config.password, sizeof(next.password));
      next.enabled = 1;
    } else if (r->command == NET_ENABLE)
      next.enabled = r->config.enabled;
    else if (r->command == NET_FORGET) {
      next.enabled = 0;
      memset(next.ssid, 0, sizeof(next.ssid));
      memset(next.password, 0, sizeof(next.password));
    }
    ok = network_config_valid(&next) && save_config(&next);
    if (ok)
      publish_config(&next);
  }
  if (ok)
    ok = radio_apply();
  portENTER_CRITICAL(&guard);
  status.request_state = ok ? 2 : 3;
  portEXIT_CRITICAL(&guard);
}
static void network_worker(void *arg) {
  (void)arg;
  network_config_t saved = {0};
  nvs_handle_t n;
  size_t size = sizeof(saved);
  bool valid = false;
  if (nvs_open("tk_network", NVS_READONLY, &n) == ESP_OK) {
    valid = nvs_get_blob(n, "config", &saved, &size) == ESP_OK &&
            size == sizeof(saved) && network_config_valid(&saved);
    nvs_close(n);
  }
  if (!valid) {
    memset(&saved, 0, sizeof(saved));
    saved.version = 1;
    esp_fill_random(saved.token, sizeof(saved.token));
    if (!save_config(&saved)) {
      set_state(NET_UNAVAILABLE, 7);
      vTaskDelete(NULL);
      return;
    }
  }
  publish_config(&saved);
  (void)radio_apply();
  for (;;) {
    network_request_t r;
    if (xQueueReceive(commands, &r, pdMS_TO_TICKS(50)) == pdTRUE)
      process_request(&r);
    uint64_t now = now_ms();
    network_config_t c;
    portENTER_CRITICAL(&guard);
    c = config;
    portEXIT_CRITICAL(&guard);
    if (got_ip) {
      got_ip = false;
      attempts = 0;
      retry_at = 0;
      set_state(NET_CONNECTED, 0);
    }
    if (lost_ip) {
      lost_ip = false;
      wifi_ap_record_t current_ap;
      if (esp_wifi_sta_get_ap_info(&current_ap) != ESP_OK) {
        portENTER_CRITICAL(&guard);
        status.ip[0] = 0;
        portEXIT_CRITICAL(&guard);
        if (radio_started && c.enabled) {
          retry_at = now + network_retry_ms(attempts++);
          set_state(disconnect_reason == WIFI_REASON_AUTH_FAIL ||
                            disconnect_reason == WIFI_REASON_HANDSHAKE_TIMEOUT
                        ? NET_AUTH_FAILED
                        : NET_WAIT,
                    disconnect_reason);
        }
      }
    }
    if (radio_started && c.enabled && retry_at && now >= retry_at) {
      retry_at = 0;
      esp_err_t e = esp_wifi_connect();
      if (e != ESP_OK) {
        retry_at = now + network_retry_ms(attempts++);
        set_state(NET_WAIT, (int)e);
      } else
        set_state(NET_JOINING, 0);
    }
    if (ap_requested && now >= ap_until) {
      ap_requested = false;
      (void)radio_apply();
    }
    handle_discovery();
    handle_dns();
  }
}
bool token_wifi_start(token_network_receiver_t receiver) {
  network_receiver = receiver;
  uint8_t mac[6];
  if (nvs_flash_init() != ESP_OK || esp_read_mac(mac, ESP_MAC_BASE) != ESP_OK)
    return false;
  snprintf(status.id, sizeof(status.id), "%02x%02x%02x%02x%02x%02x", mac[0],
           mac[1], mac[2], mac[3], mac[4], mac[5]);
  snprintf(status.ap_ssid, sizeof(status.ap_ssid), "Tokens-Kanban-%02X%02X",
           mac[4], mac[5]);
  uint8_t random[6];
  esp_fill_random(random, sizeof(random));
  network_hex_encode(random, sizeof(random), status.ap_password);
  commands = xQueueCreate(2, sizeof(network_request_t));
  if (!commands)
    return false;
  if (xTaskCreate(network_worker, "tk_network", 4096, NULL, 3, NULL) !=
      pdPASS) {
    vQueueDelete(commands);
    commands = NULL;
    return false;
  }
  return true;
}
void token_wifi_setup_request(void) {
  network_request_t r = {.command = NET_AP,
                         .id = (unsigned)(now_ms() % 2147483646) + 1};
  if (commands) {
    portENTER_CRITICAL(&guard);
    status.request_id = r.id;
    status.request_state = 1;
    portEXIT_CRITICAL(&guard);
    if (xQueueSend(commands, &r, 0) != pdTRUE) {
      portENTER_CRITICAL(&guard);
      status.request_state = 3;
      portEXIT_CRITICAL(&guard);
    }
  }
}
size_t token_wifi_line(const char *line, char *reply, size_t cap,
                       unsigned transport) {
  if (strncmp(line, "PC_WIFI", 7))
    return 0;
  if (!commands)
    return snprintf(reply, cap, "PC_WIFI_UNAVAILABLE\n");
  token_wifi_status_t s;
  token_wifi_status(&s);
  if (!strcmp(line, "PC_WIFI_GET"))
    return snprintf(reply, cap, "PC_WIFI 1 %u %u %u %u %s %d\n", s.state,
                    s.enabled, s.configured, s.ap, s.ip[0] ? s.ip : "-",
                    s.reason);
  if (!strcmp(line, "PC_WIFI_SSID")) {
    char hex[65];
    network_hex_encode(s.ssid, strlen(s.ssid), hex);
    return snprintf(reply, cap, "PC_WIFI_SSID %s\n", hex[0] ? hex : "-");
  }
  if (!strcmp(line, "PC_WIFI_ACCESS")) {
    char hex[33];
    token_wifi_access(hex);
    return snprintf(reply, cap, "PC_WIFI_ACCESS %s %s\n", s.id, hex);
  }
  if (!strcmp(line, "PC_WIFI_AP")) {
    char hex[49];
    network_hex_encode(s.ap_ssid, strlen(s.ap_ssid), hex);
    return snprintf(reply, cap, "PC_WIFI_AP %s %s\n", hex, s.ap_password);
  }
  if (!strcmp(line, "PC_WIFI_STATUS"))
    return snprintf(reply, cap, "PC_WIFI_STATE %u %u\n", s.request_id,
                    s.request_state);
  network_request_t r;
  if (!network_request_parse(line, &r))
    return snprintf(reply, cap, "PC_WIFI_INVALID\n");
  if (r.command == NET_SET && transport == TOKEN_BLE)
    return snprintf(reply, cap, "PC_WIFI_USB_REQUIRED\n");
  portENTER_CRITICAL(&guard);
  if (status.request_state == 1) {
    portEXIT_CRITICAL(&guard);
    return snprintf(reply, cap, "PC_WIFI_BUSY\n");
  }
  status.request_id = r.id;
  status.request_state = 1;
  portEXIT_CRITICAL(&guard);
  if (xQueueSend(commands, &r, 0) != pdTRUE) {
    portENTER_CRITICAL(&guard);
    status.request_state = 3;
    portEXIT_CRITICAL(&guard);
    return snprintf(reply, cap, "PC_WIFI_STATE %u 3\n", r.id);
  }
  return snprintf(reply, cap, "PC_WIFI_STATE %u 1\n", r.id);
}
