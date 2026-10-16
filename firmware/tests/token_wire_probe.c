#include "token_model.h"
#include <stdio.h>
int main(void) {
    token_stream_t stream={0};token_snapshot_t snapshot;int byte;
    while((byte=getchar())!=EOF) {
        if(token_stream_push(&stream,(char)byte)&&token_parse(stream.data,&snapshot)) {
            printf("%lld %lld %lld %lld %lld %lld %lld %lld %lld %lld\n",(long long)snapshot.v[T_SEQ],
                (long long)token_total(&snapshot,0),(long long)token_total(&snapshot,1),(long long)token_total(&snapshot,2),
                (long long)snapshot.v[T_DAY_CENTS],(long long)snapshot.v[T_DAY_COST_STATUS],
                (long long)snapshot.v[T_WEEK_CENTS],(long long)snapshot.v[T_WEEK_COST_STATUS],
                (long long)snapshot.v[T_ALL_CENTS],(long long)snapshot.v[T_ALL_COST_STATUS]);
            if(snapshot.v[T_ANALYTICS]) printf("A %lld %lld %lld %u %u\n",
                (long long)snapshot.v[T_MODELS],(long long)snapshot.v[T_MODELS+1],
                (long long)snapshot.v[T_PEAK],snapshot.heat[180],snapshot.trend[59]);
        }
    }
    return 0;
}
