#pragma once
#include "network_model.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct {
  unsigned state, enabled, configured, ap, request_id, request_state;
  int reason;
  char ip[16], id[13], ssid[33], ap_ssid[25], ap_password[13];
} token_wifi_status_t;
typedef size_t (*token_network_receiver_t)(const char *, char *, size_t);
bool token_wifi_start(token_network_receiver_t receiver);
size_t token_wifi_line(const char *line, char *reply, size_t cap,
                       unsigned transport);
void token_wifi_status(token_wifi_status_t *out);
bool token_wifi_authorized(const char *token);
void token_wifi_access(char out[33]);
void token_wifi_setup_request(void);
void token_wifi_management_url(char *out, size_t cap, bool setup);
