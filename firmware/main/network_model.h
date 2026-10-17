#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define NETWORK_CONFIG_VERSION 1u
#define NETWORK_TOKEN_BYTES 16
#define NETWORK_TOKEN_HEX 32
#define NETWORK_DISCOVERY_PORT 43759
#define NETWORK_AP_DURATION_MS 600000u
typedef struct {
  uint32_t version;
  uint8_t enabled;
  char ssid[33], password[65];
  uint8_t token[16];
} network_config_t;
typedef struct {
  unsigned command, id;
  network_config_t config;
} network_request_t;
enum { NET_SET = 1, NET_ENABLE, NET_FORGET, NET_AP };
enum {
  NET_OFF,
  NET_WAIT,
  NET_JOINING,
  NET_CONNECTED,
  NET_AUTH_FAILED,
  NET_SETUP,
  NET_UNAVAILABLE
};
bool network_config_valid(const network_config_t *config);
bool network_request_parse(const char *line, network_request_t *request);
bool network_hex_decode(const char *hex, char *out, size_t capacity,
                        bool empty);
void network_hex_encode(const void *data, size_t length, char *out);
unsigned network_retry_ms(unsigned attempts);
size_t network_dns_reply(uint8_t *packet, size_t length, size_t capacity);
bool network_token_equal(const char *hex, const uint8_t token[16]);
