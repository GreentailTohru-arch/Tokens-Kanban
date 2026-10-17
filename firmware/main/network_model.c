#include "network_model.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>
static int nibble(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  return -1;
}
bool network_hex_decode(const char *hex, char *out, size_t cap, bool empty) {
  if (empty && !strcmp(hex, "-")) {
    if (!cap)
      return false;
    out[0] = 0;
    return true;
  }
  size_t n = strlen(hex);
  if (!n || (n & 1) || n / 2 >= cap)
    return false;
  for (size_t i = 0; i < n; i += 2) {
    int a = nibble(hex[i]), b = nibble(hex[i + 1]);
    if (a < 0 || b < 0 || (!a && !b))
      return false;
    out[i / 2] = (char)((a << 4) | b);
  }
  out[n / 2] = 0;
  return true;
}
void network_hex_encode(const void *data, size_t n, char *out) {
  const unsigned char *p = data;
  const char h[] = "0123456789abcdef";
  for (size_t i = 0; i < n; i++) {
    out[i * 2] = h[p[i] >> 4];
    out[i * 2 + 1] = h[p[i] & 15];
  }
  out[n * 2] = 0;
}
bool network_config_valid(const network_config_t *c) {
  if (c->version != 1 || c->enabled > 1 ||
      !memchr(c->ssid, 0, sizeof(c->ssid)) ||
      !memchr(c->password, 0, sizeof(c->password)))
    return false;
  size_t s = strlen(c->ssid), p = strlen(c->password);
  if (s > 32 || p > 63 || (p && p < 8) || (!s && p))
    return false;
  if (c->enabled && !s)
    return false;
  unsigned token = 0;
  for (unsigned i = 0; i < 16; i++)
    token |= c->token[i];
  return token != 0;
}
bool network_request_parse(const char *line, network_request_t *r) {
  memset(r, 0, sizeof(*r));
  const char *id_start = strchr(line, ' ');
  if (!id_start || id_start[1] < '0' || id_start[1] > '9')
    return false;
  uint64_t parsed = 0;
  for (const char *p = id_start + 1; *p && *p != ' '; p++) {
    if (*p < '0' || *p > '9')
      return false;
    parsed = parsed * 10 + (unsigned)(*p - '0');
    if (parsed > INT_MAX)
      return false;
  }
  if (!parsed)
    return false;
  char ssid[65], pass[129];
  int end = 0;
  unsigned v = 0;
  if (sscanf(line, "PC_WIFI_SET %u %64s %128s%n", &r->id, ssid, pass, &end) ==
          3 &&
      !line[end]) {
    r->command = NET_SET;
    r->config.version = 1;
    r->config.enabled = 1;
    if (!network_hex_decode(ssid, r->config.ssid, sizeof(r->config.ssid),
                            false) ||
        !network_hex_decode(pass, r->config.password,
                            sizeof(r->config.password), true))
      return false;
    size_t p = strlen(r->config.password);
    if (p && (p < 8 || p > 63))
      return false;
  } else if (sscanf(line, "PC_WIFI_ENABLE %u %u%n", &r->id, &v, &end) == 2 &&
             !line[end] && v <= 1 && line[end - 1] == (char)('0' + v) &&
             line[end - 2] == ' ') {
    r->command = NET_ENABLE;
    r->config.enabled = (uint8_t)v;
  } else if (sscanf(line, "PC_WIFI_FORGET %u%n", &r->id, &end) == 1 &&
             !line[end])
    r->command = NET_FORGET;
  else if (sscanf(line, "PC_WIFI_SETUP %u%n", &r->id, &end) == 1 && !line[end])
    r->command = NET_AP;
  else
    return false;
  return r->id == parsed;
}
unsigned network_retry_ms(unsigned attempts) {
  return attempts >= 5 ? 30000u : 1000u << attempts;
}
bool network_token_equal(const char *hex, const uint8_t token[16]) {
  if (strlen(hex) != 32)
    return false;
  unsigned diff = 0;
  for (unsigned i = 0; i < 16; i++) {
    int a = nibble(hex[i * 2]), b = nibble(hex[i * 2 + 1]);
    if (a < 0 || b < 0)
      return false;
    diff |= (unsigned)((a << 4) | b) ^ token[i];
  }
  return diff == 0;
}
size_t network_dns_reply(uint8_t *p, size_t n, size_t cap) {
  if (n < 17 || n > cap || p[4] != 0 || p[5] != 1 || (p[2] & 0xf8))
    return 0;
  size_t end = 12;
  while (end < n && p[end]) {
    unsigned size = p[end];
    if (size > 63 || end + 1 + size >= n)
      return 0;
    end += 1 + size;
  }
  if (end + 5 > n)
    return 0;
  end += 5;
  bool ipv4 =
      p[end - 4] == 0 && p[end - 3] == 1 && p[end - 2] == 0 && p[end - 1] == 1;
  if (ipv4 && end + 16 > cap)
    return 0;
  p[2] = 0x81;
  p[3] = 0x80;
  p[6] = 0;
  p[7] = ipv4 ? 1 : 0;
  memset(p + 8, 0, 4);
  if (ipv4) {
    const uint8_t answer[] = {0xc0, 0x0c, 0, 1, 0,   1,   0, 0,
                              0,    30,   0, 4, 192, 168, 4, 1};
    memcpy(p + end, answer, sizeof(answer));
    end += sizeof(answer);
  }
  return end;
}
