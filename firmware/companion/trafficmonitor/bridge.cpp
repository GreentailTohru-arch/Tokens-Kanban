/* Data comes only from the official TrafficMonitor V1.86 plugin callback. */
#include "win_api.h"
#include "vendor/PluginInterface.h"
#include "bridge_packet.h"
static bool same(const wchar_t *a,const wchar_t *b){while(*a && *a==*b){++a;++b;}return *a==*b;}
class PassportBridge final:public ITMPlugin {
    ITrafficMonitor *app=nullptr;
    bool ready=false;
    uint32_t sequence=0;
public:
    IPluginItem* GetItem(int) override {return nullptr;}
    void DataRequired() override {}
    const wchar_t* GetInfo(PluginInfoIndex index) override {
        switch(index) {
            case TMI_NAME:return L"Tokens Kanban Bridge";
            case TMI_DESCRIPTION:return L"Read-only TrafficMonitor 1.86 telemetry to AI Passport";
            case TMI_AUTHOR:return L"Tokens Kanban";
            case TMI_VERSION:return L"15.0.0";
            case TMI_COPYRIGHT:return L"Tokens Kanban; TrafficMonitor interface by Zhong Yang";
            case TMI_URL:return L"https://github.com/GreentailTohru-arch/Tokens-Kanban";
            default:return L"";
        }
    }
    void OnInitialize(ITrafficMonitor *host) override {
        ready=false;app=host;
        // Newer versions move temperature acquisition to a separate plugin.
        // Never treat zero-filled legacy fields from an unverified host as sensors.
        if(!host || !host->GetVersion())return;
        const wchar_t *version=host->GetVersion();
        if(!same(version,L"1.86") && !same(version,L"V1.86"))return;
        ready=true;
    }
    void OnMonitorInfo(const MonitorInfo &m) override {
        if(!ready)return;
        BridgePacket packet{};const char magic[8]={'T','M','K','1','5',0,0,0};
        for(unsigned i=0;i<8;++i)packet.magic[i]=magic[i];
        packet.version=2;packet.size=sizeof(packet);packet.pid=GetCurrentProcessId();
        packet.sequence=++sequence;packet.tick_ms=GetTickCount64();
        int64_t raw[]={m.cpu_usage,m.gpu_usage,m.memory_usage,m.cpu_temperature,m.gpu_temperature,
            m.down_speed<=1000000000000ULL?static_cast<int64_t>(m.down_speed):-1,
            m.up_speed<=1000000000000ULL?static_cast<int64_t>(m.up_speed):-1,-1,m.hdd_usage,-1,-1};
        double ghz=app->GetMonitorValue(ITrafficMonitor::MI_CPU_FREQ);
        if(ghz>0 && ghz<=100)raw[7]=static_cast<int64_t>(ghz*1000+0.5);
        const ITrafficMonitor::MonitorItem today[]={ITrafficMonitor::MI_TODAY_UP_TRAFFIC,ITrafficMonitor::MI_TODAY_DOWN_TRAFFIC};
        for(unsigned i=0;i<2;++i){double bytes=app->GetMonitorValue(today[i]);if(bytes>=0 && bytes<=9007199254740991.0)raw[9+i]=static_cast<int64_t>(bytes);}
        const ITrafficMonitor::MonitorItem items[]={ITrafficMonitor::MI_CPU,ITrafficMonitor::MI_GPU_USAGE,
            ITrafficMonitor::MI_MEMORY,ITrafficMonitor::MI_CPU_TEMP,ITrafficMonitor::MI_GPU_TEMP,ITrafficMonitor::MI_DOWN,ITrafficMonitor::MI_UP,ITrafficMonitor::MI_CPU_FREQ,ITrafficMonitor::MI_HDD_USAGE,ITrafficMonitor::MI_TODAY_UP_TRAFFIC,ITrafficMonitor::MI_TODAY_DOWN_TRAFFIC};
        for(unsigned i=0;i<11;++i) {
            int64_t limit=i<3||i==8?100:i<5?150:i==7?100000:i>=9?9007199254740991LL:1000000000000LL;
            packet.values[i]=(raw[i]>=0 && raw[i]<=limit && (i<3 || i>4 || raw[i]>0))?raw[i]:-1;
            // Compare callback against the host's official getter, and retain its
            // formatted taskbar text for diagnostics. No screen/process-memory reads.
            double host_value=app->GetMonitorValue(items[i]);
            double difference=(i==7?host_value*1000:host_value)-static_cast<double>(raw[i]);
            if(difference>-1.0 && difference<1.0)packet.matches|=1u<<i;
            const wchar_t *text=app->GetMonitorValueString(items[i],false);
            if(text)for(unsigned j=0;j<47 && text[j];++j)packet.display[i][j]=text[j];
        }
        // Local message IPC only. No sensor polling, files, network sockets, or threads.
        void *slot=CreateFileW(L"\\\\.\\mailslot\\TokensKanban-TrafficMonitor-v1",0x40000000,2,nullptr,3,0x80,nullptr);
        if(slot!=reinterpret_cast<void*>(-1LL)) {unsigned long written=0;(void)WriteFile(slot,&packet,sizeof(packet),&written,nullptr);CloseHandle(slot);}
    }
};
constinit PassportBridge plugin;
extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance() {return &plugin;}
extern "C" int __stdcall DllMain(void*,unsigned long,void*) {return 1;}
// No CRT initialization or dependency: this plugin owns no heap, threads or handles.
extern "C" void* memset(void *target,int value,unsigned long long count) {volatile unsigned char *p=(volatile unsigned char*)target;for(unsigned long long i=0;i<count;++i)p[i]=(unsigned char)value;return target;}
extern "C" int _fltused=0;
