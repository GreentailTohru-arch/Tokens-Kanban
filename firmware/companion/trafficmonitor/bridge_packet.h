#pragma once
using uint32_t=unsigned int;
using uint64_t=unsigned long long;
using int64_t=long long;

#pragma pack(push,1)
struct BridgePacket {
    char magic[8];
    uint32_t version,size,pid,sequence;
    uint64_t tick_ms;
    int64_t values[11];
    uint32_t matches;
    wchar_t display[11][48];
};
#pragma pack(pop)
static_assert(sizeof(wchar_t)==2,"Windows UTF-16 ABI required");
static_assert(sizeof(BridgePacket)==1180,"Pinned IPC ABI");
