#include "network_http.h"
#include "esp_http_server.h"
#include "lwip/sockets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static httpd_handle_t server;
static token_network_receiver_t receiver;
static bool setup_request(httpd_req_t *req) {
  struct sockaddr_in local;
  socklen_t n = sizeof(local);
  return getsockname(httpd_req_to_sockfd(req), (struct sockaddr *)&local, &n) ==
             0 &&
         local.sin_addr.s_addr == htonl(0xc0a80401u);
}
static bool authorized(httpd_req_t *req) {
  char value[48];
  return httpd_req_get_hdr_value_str(req, "Authorization", value,
                                     sizeof(value)) == ESP_OK &&
         !strncmp(value, "Bearer ", 7) && token_wifi_authorized(value + 7);
}
static esp_err_t rpc_handler(httpd_req_t *req) {
  if (!authorized(req)) {
    httpd_resp_set_status(req, "401 Unauthorized");
    return httpd_resp_sendstr(req, "Access token required");
  }
  if (!req->content_len || req->content_len > 1024) {
    httpd_resp_set_status(req, "413 Content Too Large");
    return httpd_resp_sendstr(req, "Invalid command size");
  }
  char *body = malloc(req->content_len + 1);
  if (!body)
    return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                               "Memory unavailable");
  size_t read = 0;
  int timeouts = 0;
  while (read < req->content_len) {
    int n = httpd_req_recv(req, body + read, req->content_len - read);
    if (n == HTTPD_SOCK_ERR_TIMEOUT && timeouts++ < 2)
      continue;
    if (n <= 0) {
      free(body);
      return ESP_FAIL;
    }
    read += (size_t)n;
  }
  body[read] = 0;
  bool invalid = memchr(body, 0, read) != NULL;
  while (read && (body[read - 1] == '\n' || body[read - 1] == '\r'))
    body[--read] = 0;
  if (invalid || strchr(body, '\n') || strchr(body, '\r')) {
    free(body);
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                               "One command required");
  }
  char reply[96];
  size_t n = receiver(body, reply, sizeof(reply));
  free(body);
  if (!n || n >= sizeof(reply))
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                               "Unsupported command");
  httpd_resp_set_type(req, "text/plain; charset=utf-8");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, reply, (ssize_t)n);
}
static esp_err_t info_handler(httpd_req_t *req) {
  token_wifi_status_t s;
  token_wifi_status(&s);
  char body[128];
  snprintf(body, sizeof(body),
           "{\"kind\":\"Wi-Fi\",\"connected\":true,\"version\":\"v19\",\"id\":"
           "\"%s\"}",
           s.id);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_sendstr(req, body);
}
static esp_err_t page_handler(httpd_req_t *req) {
  char uri[256];
  size_t uri_length=strlen(req->uri);
  if(uri_length>=sizeof(uri))return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"URI too long");
  memcpy(uri,req->uri,uri_length+1);
  char *query = strchr(uri, '?');
  if (query)
    *query = 0;
  const network_asset_t *asset = NULL;
  for (size_t i = 0; i < network_assets_count; i++)
    if (!strcmp(uri, network_assets[i].uri)) {
      asset = &network_assets[i];
      break;
    }
  if (!asset) {
    if (setup_request(req)) {
      httpd_resp_set_status(req, "302 Found");
      httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/");
      return httpd_resp_sendstr(req, "");
    }
    return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
  }
  httpd_resp_set_type(req, asset->mime);
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  httpd_resp_set_hdr(req, "X-Content-Type-Options", "nosniff");
  httpd_resp_set_hdr(req, "Referrer-Policy", "no-referrer");
  httpd_resp_set_hdr(
      req, "Content-Security-Policy",
      "default-src 'self'; img-src 'self' blob: data:; style-src 'self'; "
      "script-src 'self'; connect-src 'self' https://api.github.com "
      "https://raw.githubusercontent.com https://github.com "
      "https://release-assets.githubusercontent.com "
      "https://objects.githubusercontent.com; frame-ancestors 'none'");
  if (asset->compressed)
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  if (!strcmp(uri, "/")) {
    const char *data = (const char *)asset->data,
               *marker = strstr(data, "@PASSPORT_TOKEN@");
    if (!marker)
      return ESP_FAIL;
    char token[33] = {0};
    if (setup_request(req))
      token_wifi_access(token);
    if (httpd_resp_send_chunk(req, data, marker - data) != ESP_OK)
      return ESP_FAIL;
    if (httpd_resp_send_chunk(req, token, strlen(token)) != ESP_OK)
      return ESP_FAIL;
    size_t offset = (size_t)(marker - data) + strlen("@PASSPORT_TOKEN@");
    if (httpd_resp_send_chunk(req, data + offset, asset->length - offset) !=
        ESP_OK)
      return ESP_FAIL;
    return httpd_resp_send_chunk(req, NULL, 0);
  }
  return httpd_resp_send(req, (const char *)asset->data,
                         (ssize_t)asset->length);
}
bool network_http_start(token_network_receiver_t callback) {
  if (server)
    return true;
  receiver = callback;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.stack_size = 6144;
  config.max_open_sockets = 2;
  config.backlog_conn = 2;
  config.max_uri_handlers = 4;
  config.lru_purge_enable = true;
  config.recv_wait_timeout = 3;
  config.send_wait_timeout = 3;
  config.uri_match_fn = httpd_uri_match_wildcard;
  if (httpd_start(&server, &config) != ESP_OK) {
    server = NULL;
    return false;
  }
  const httpd_uri_t
      rpc = {.uri = "/api/rpc", .method = HTTP_POST, .handler = rpc_handler},
      info = {.uri = "/api/info", .method = HTTP_GET, .handler = info_handler},
      assets = {.uri = "/*", .method = HTTP_GET, .handler = page_handler};
  if (httpd_register_uri_handler(server, &rpc) != ESP_OK ||
      httpd_register_uri_handler(server, &info) != ESP_OK ||
      httpd_register_uri_handler(server, &assets) != ESP_OK) {
    network_http_stop();
    return false;
  }
  return true;
}
void network_http_stop(void) {
  if (server) {
    httpd_stop(server);
    server = NULL;
  }
}
