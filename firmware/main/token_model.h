#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define TOKEN_FIELDS 47
#define TOKEN_FRAME_MAX 1024
#define TOKEN_HEAT_DAYS 182
#define TOKEN_TREND_DAYS 60
enum { T_SEQ, T_DAY_IN, T_DAY_CACHE, T_DAY_OUT, T_WEEK_IN, T_WEEK_CACHE,
       T_WEEK_OUT, T_ALL_IN, T_ALL_CACHE, T_ALL_OUT, T_USED1, T_WINDOW1,
       T_RESET1, T_USED2, T_WINDOW2, T_RESET2, T_CREDITS, T_PLAN, T_QUOTA_AGE, T_ACTIVITY,
       T_DAY_CENTS, T_DAY_COST_STATUS, T_WEEK_CENTS, T_WEEK_COST_STATUS, T_ALL_CENTS, T_ALL_COST_STATUS,
       T_MODELS, T_MODEL_FIELDS_END=T_MODELS+11, T_ACTIVE_DAYS,
       T_HEAT_MONTH0, T_HEAT_MONTH1, T_HEAT_MONTH2,
       T_TREND_DATE0, T_TREND_DATE1, T_TREND_DATE2, T_PEAK, T_ANALYTICS };
enum { TOKEN_UNKNOWN, TOKEN_IDLE, TOKEN_BUSY, TOKEN_LIMITED };
enum { TOKEN_USB, TOKEN_BLE, TOKEN_WIFI };
typedef struct { int64_t v[TOKEN_FIELDS]; uint8_t heat[TOKEN_HEAT_DAYS];
                 uint16_t trend[TOKEN_TREND_DAYS]; unsigned transport; } token_snapshot_t;
typedef struct { unsigned page, period; bool show_cost, contact; } token_nav_t;
enum { TOKEN_KEY_UP, TOKEN_KEY_DOWN, TOKEN_KEY_OK };
typedef struct { char data[TOKEN_FRAME_MAX]; size_t len; bool dropping; } token_stream_t;
uint32_t token_checksum(const char *s, size_t n);
bool token_parse(const char *line, token_snapshot_t *out);
bool token_stream_push(token_stream_t *s, char c);
void token_number(char *out, size_t size, int64_t n, bool compact);
int64_t token_total(const token_snapshot_t *s, unsigned period);
bool token_online(bool received, uint64_t now_ms, uint64_t received_ms);
int64_t token_remaining(int64_t seconds, uint64_t elapsed_ms);
int token_signal(const token_snapshot_t *s, bool received, uint64_t elapsed_ms);
