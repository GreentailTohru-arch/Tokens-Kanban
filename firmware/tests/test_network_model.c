#include "network_model.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
  network_request_t r;
  network_config_t c = {.version = 1, .token = {1}};
  assert(network_config_valid(&c));
  c.enabled = 1;
  assert(!network_config_valid(&c));
  strcpy(c.ssid, "home");
  assert(network_config_valid(&c));
  strcpy(c.password, "short");
  assert(!network_config_valid(&c));
  strcpy(c.password, "12345678");
  assert(network_config_valid(&c));
  assert(
      network_request_parse("PC_WIFI_SET 1 e4b8ade69687 3132333435363738", &r));
  assert(!strcmp(r.config.ssid, "中文"));
  assert(!strcmp(r.config.password, "12345678"));
  assert(network_request_parse("PC_WIFI_SET 2 6162 -", &r));
  assert(!r.config.password[0]);
  const char *bad[] = {"PC_WIFI_SET 0 61 -",          "PC_WIFI_SET -1 61 -",
                       "PC_WIFI_SET 2147483648 61 -", "PC_WIFI_SET 1 00 -",
                       "PC_WIFI_SET 1 6 -",           "PC_WIFI_SET 1 61 3132",
                       "PC_WIFI_SET 1 61 - extra",    "PC_WIFI_ENABLE 1 -0",
                       "PC_WIFI_ENABLE 1 2",          "PC_WIFI_SETUP 1\n",
                       "PC_WIFI_FORGET 1 xxx",        "PC_WIFI_GET"};
  for (unsigned i = 0; i < sizeof(bad) / sizeof(*bad); i++)
    assert(!network_request_parse(bad[i], &r));
  assert(network_request_parse("PC_WIFI_ENABLE 9 1", &r) &&
         r.command == NET_ENABLE && r.config.enabled == 1);
  assert(network_request_parse("PC_WIFI_FORGET 9", &r));
  assert(network_request_parse("PC_WIFI_SETUP 9", &r));
  assert(network_retry_ms(0) == 1000 && network_retry_ms(4) == 16000 &&
         network_retry_ms(5) == 30000 && network_retry_ms(99) == 30000);
  char token[33];
  network_hex_encode(c.token, 16, token);
  assert(network_token_equal(token, c.token));
  token[1] = '2';
  assert(!network_token_equal(token, c.token));
  assert(!network_token_equal("", c.token));
  uint8_t dns[128] = {0x12, 0x34, 1,   0,   0,   1, 0, 0, 0, 0, 0,
                      0,    3,    'w', 'w', 'w', 0, 0, 1, 0, 1};
  assert(network_dns_reply(dns, 21, sizeof(dns)) == 37 && dns[7] == 1 &&
         dns[33] == 192 && dns[36] == 1);
  dns[2] = 1;
  dns[7] = 0;
  dns[18] = 28;
  assert(network_dns_reply(dns, 21, sizeof(dns)) == 21 && dns[7] == 0);
  dns[12] = 0xc0;
  assert(!network_dns_reply(dns, 21, sizeof(dns)));
  assert(!network_dns_reply(dns, 0, sizeof(dns)));
  puts("Network protocol / retry / token / captive DNS: PASS");
  return 0;
}
