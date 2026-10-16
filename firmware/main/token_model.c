#include "token_model.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
uint32_t token_checksum(const char *s, size_t n) {
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; ++i) h = (h ^ (unsigned char)s[i]) * 16777619u;
    return h;
}
bool token_parse(const char *line, token_snapshot_t *out) {
    if (!line || !out) return false;
    bool analytics=!strncmp(line,"CT4 ",4);
    int fields = !strncmp(line,"CT1 ",4) ? 19 : !strncmp(line,"CT2 ",4) ? 20 : !strncmp(line,"CT3 ",4) ? 26 : analytics ? TOKEN_FIELDS : 0;
    if (!fields) return false;
    if (strlen(line) >= TOKEN_FRAME_MAX) return false;
    const char *star = strchr(line, '*');
    if (!star || strlen(star + 1) != 8) return false;
    for (int i = 1; i <= 8; ++i)
        if (!((star[i] >= '0' && star[i] <= '9') || (star[i] >= 'a' && star[i] <= 'f'))) return false;
    char *end;
    uint32_t crc = (uint32_t)strtoul(star + 1, &end, 16);
    if (*end || crc != token_checksum(line, (size_t)(star - line))) return false;
    token_snapshot_t tmp = {0};
    for (int i=T_DAY_CENTS;i<=T_ALL_CENTS;i+=2) tmp.v[i]=-1;
    for (int i=T_MODELS+1;i<=T_MODEL_FIELDS_END;i+=2) tmp.v[i]=-1;
    const char *p = line + 4;
    for (int i = 0; i < fields; ++i) {
        if (!(*p >= '0' && *p <= '9') && *p != '-') return false;
        errno = 0; long long v = strtoll(p, &end, 10);
        if (errno || end == p || v < -1 || v > 1000000000000000LL) return false;
        tmp.v[i] = v;
        if (i == fields - 1 && !analytics) { if (end != star) return false; }
        else { if (*end != ' ') return false; p = end + 1; }
    }
    if (analytics) {
        if ((size_t)(star-p)!=TOKEN_HEAT_DAYS+1+TOKEN_TREND_DAYS*3) return false;
        for (int i=0;i<TOKEN_HEAT_DAYS;++i) {
            if (p[i]<'0' || p[i]>'5') return false;
            tmp.heat[i]=(uint8_t)(p[i]-'0');
        }
        p+=TOKEN_HEAT_DAYS;if (*p++!=' ') return false;
        for (int i=0;i<TOKEN_TREND_DAYS;++i) {
            unsigned point=0;
            for (int j=0;j<3;++j) {
                char c=*p++;unsigned digit;
                if (c>='0' && c<='9') digit=(unsigned)(c-'0');
                else if (c>='a' && c<='f') digit=(unsigned)(c-'a'+10);
                else return false;
                point=point*16+digit;
            }
            if (point>1000) return false;
            tmp.trend[i]=(uint16_t)point;
        }
        if (tmp.v[T_ANALYTICS]<0 || tmp.v[T_ANALYTICS]>1 || tmp.v[T_ACTIVE_DAYS]<0 || tmp.v[T_ACTIVE_DAYS]>182 || tmp.v[T_PEAK]<0) return false;
        for (int i=T_MODELS;i<=T_MODEL_FIELDS_END;i+=2)
            if (tmp.v[i]<0 || tmp.v[i]>7 || (tmp.v[i+1]<0 && tmp.v[i]!=0)) return false;
        for (int period=0;period<3;++period) {
            int i=T_MODELS+period*4;
            int64_t total=token_total(&tmp,(unsigned)period);
            int64_t a=tmp.v[i+1],b=tmp.v[i+3];
            if ((a>=0 || b>=0) && (total<0 || (a>0?a:0)+(b>0?b:0)>total)) return false;
        }
        for (int i=T_HEAT_MONTH0;i<=T_HEAT_MONTH2;++i)
            if (tmp.v[i]<1 || tmp.v[i]>12) return false;
        for (int i=T_TREND_DATE0;i<=T_TREND_DATE2;++i)
            if (tmp.v[i]/100<1 || tmp.v[i]/100>12 || tmp.v[i]%100<1 || tmp.v[i]%100>31) return false;
    }
    if (tmp.v[T_SEQ] < 0 || tmp.v[T_PLAN] < 0 || tmp.v[T_PLAN] > 5) return false;
    if (tmp.v[T_ACTIVITY] < TOKEN_UNKNOWN || tmp.v[T_ACTIVITY] > TOKEN_BUSY) return false;
    for (int i=T_DAY_CENTS;i<=T_ALL_CENTS;i+=2) {
        int64_t status=tmp.v[i+1];
        if (status<0 || status>2 || (status==0 ? tmp.v[i]!=-1 : tmp.v[i]<0)) return false;
    }
    for (int i = T_DAY_IN; i <= T_ALL_IN; i += 3) {
        bool missing = tmp.v[i] == -1;
        if ((tmp.v[i+1] == -1) != missing || (tmp.v[i+2] == -1) != missing) return false;
        if (!missing && tmp.v[i+1] > tmp.v[i]) return false;
    }
    for (int i = T_USED1; i <= T_USED2; i += 3) {
        if (tmp.v[i] > 100 || tmp.v[i+1] > 525600 || tmp.v[i+2] > 31536000) return false;
        if (tmp.v[i] >= 0 && tmp.v[i+1] <= 0) return false;
    }
    *out = tmp; return true;
}
bool token_stream_push(token_stream_t *s, char c) {
    if (c == '\r') return false;
    if (c == '\n') {
        bool ready = !s->dropping && s->len > 0;
        s->data[s->len] = '\0'; s->len = 0; s->dropping = false; return ready;
    }
    if ((unsigned char)c < 32 || (unsigned char)c > 126) s->dropping = true;
    if (!s->dropping) {
        if (s->len + 1 < sizeof(s->data)) s->data[s->len++] = c;
        else s->dropping = true;
    }
    return false;
}
void token_number(char *out, size_t size, int64_t n, bool compact) {
    if (n < 0) { snprintf(out, size, "--"); return; }
    if (compact && n >= 1000000000000LL) { snprintf(out, size, "%.1fT", (double)n / 1e12); return; }
    if (compact && n >= 1000000000LL) { snprintf(out, size, "%.2fB", (double)n / 1e9); return; }
    if (compact && n >= 1000000LL) { snprintf(out, size, "%.2fM", (double)n / 1e6); return; }
    char raw[24], grouped[32]; snprintf(raw, sizeof(raw), "%" PRId64, n);
    size_t len = strlen(raw), j = 0;
    for (size_t i = 0; i < len; ++i) {
        if (i && (len-i)%3 == 0) grouped[j++] = ',';
        grouped[j++] = raw[i];
    }
    grouped[j] = 0; snprintf(out, size, "%s", grouped);
}
int64_t token_total(const token_snapshot_t *s, unsigned period) {
    if (period > 2) return -1;
    int i = T_DAY_IN + (int)period * 3;
    return s->v[i] < 0 ? -1 : s->v[i] + s->v[i+2];
}
bool token_online(bool received, uint64_t now_ms, uint64_t received_ms) {
    return received && now_ms >= received_ms && now_ms - received_ms < 15000;
}
int64_t token_remaining(int64_t seconds, uint64_t elapsed_ms) {
    if (seconds < 0) return -1;
    uint64_t elapsed = elapsed_ms / 1000;
    return elapsed >= (uint64_t)seconds ? 0 : seconds - (int64_t)elapsed;
}
int token_signal(const token_snapshot_t *s, bool received, uint64_t elapsed_ms) {
    if (!token_online(received,elapsed_ms,0)) return TOKEN_UNKNOWN;
    bool fresh=s->v[T_QUOTA_AGE]>=0 && s->v[T_QUOTA_AGE]+(int64_t)(elapsed_ms/1000)<180;
    if (fresh) {
        for (int i=T_USED1;i<=T_USED2;i+=3)
            if (s->v[i]==100 && token_remaining(s->v[i+2],elapsed_ms)!=0) return TOKEN_LIMITED;
    }
    return s->v[T_ACTIVITY]>=TOKEN_IDLE && s->v[T_ACTIVITY]<=TOKEN_BUSY ? (int)s->v[T_ACTIVITY] : TOKEN_UNKNOWN;
}
