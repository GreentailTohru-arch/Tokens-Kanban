[English](README.md)

# Tokens Kanban 的 TrafficMonitor 桥接插件（组件 v15）

x64 桥接插件直接复用 **TrafficMonitor 1.86** 已采集的数据，不重复轮询传感器或网卡。
不添加显示项目、不改 TrafficMonitor 配置、不开放网络端口、不读取其他进程内存，
不另装硬件监控软件或驱动。独立温度插件不在本次适配范围，桥接会拒绝未核实的主程序版本。

## 安装与使用

1. 保留现有 TrafficMonitor 1.86 x64 及其选定网卡，将 `TokensKanbanBridge.dll`
   复制到程序旁的 `plugins` 文件夹。
2. 退出并重新打开一次 TrafficMonitor 以加载 DLL。正在运行时先征得使用者同意，
   沿用原来的管理员权限和设置。
3. 运行同版配套的 `CodexTokenDisplay.exe`（v15 或以后版本），默认优先 USB，也可用 `--transport ble`。
   同时只能运行一个同步接收程序。
4. 在 Passport 上双击确定，在已开启页面中切到系统监测。其他页面的上下键、单击
   操作保持不变。浏览器左侧第一项“页面管理”提供四页预览、开关和上移／下移；
   保存后应用顺序，至少保留一页。

`CodexTokenDisplay.exe --system-only` 只输出一次诊断，不连接设备、不读取 Codex 日志。
内容包括十一项数值、TrafficMonitor 原始任务栏格式化文字和十一位一致性标记；`2047`
表示回调值与主程序取值全部一致（允许主程序整摄氏度精度）。不能与另一份桥接
接收器同时运行。TrafficMonitor 不在运行时，正常同步程序仍可独立同步 Token 和额度。

未知值用 `-1` 表示，屏幕显示 `--`；真实的 0% 占用和 0 流量仍有效。根据 V1.86 的
温度约定，温度小于等于零表示不可用。来源进程退出或数据超过五秒时，全部系统数据
失效，在下一次五秒同步时反映到设备；电脑停止同步时，设备十五秒后将系统数据置为
过期。恢复接收器或 TrafficMonitor 后自动恢复，无需重启设备。网络字节速率来自
主程序当前选择的网卡，按同样的 1024 进制显示，紧凑界面保留一位小数。无需配网。

## 接口核对与构建

`vendor/PluginInterface.h` 原样保留官方
[V1.86](https://github.com/zhongyang219/TrafficMonitor/tree/02a817a069bac6bf4d263b5209d9c1b07fe2f950)
的 API **7**，旁边保留原始许可证。主程序采集完成后调用 `OnMonitorInfo`，传递内置
温度和网卡字节速率；桥接在同一次回调中核对 `GetMonitorValue`，并记录
`GetMonitorValueString`。依据为
[TrafficMonitorDlg.cpp](https://github.com/zhongyang219/TrafficMonitor/blob/V1.86/TrafficMonitor/TrafficMonitorDlg.cpp#L1513)
与 [TrafficMonitor.cpp](https://github.com/zhongyang219/TrafficMonitor/blob/V1.86/TrafficMonitor/TrafficMonitor.cpp#L1401)。
V1.86 可能自带 LibreHardwareMonitor 库，桥接只使用主程序公开插件接口，不调用该库。

本机 Windows 邮槽 `\\.\mailslot\TokensKanban-TrafficMonitor-v1` 每次更新传递固定
1180 字节消息。接收端检查版本、长度、时间、范围、来源进程名称及存活状态，不申请
`PROCESS_VM_READ` 权限；重复或旧消息不能覆盖新数据。DLL 没有后台线程、长期持有
的句柄、堆分配或 CRT 依赖。必须使用 Windows x64 MSVC ABI，不能换成 MinGW C++ DLL。

使用已安装的 x64 MSVC 编译器（C++20）构建：

```powershell
./build.ps1 -OutputDirectory ../../build/trafficmonitor
```

构建采用有文档依据的最小 Win32 声明和生成的 Kernel32 导入库，不需要安装 Windows
SDK／CRT；启用 ASLR 和 DEP。固件仓库内执行
`python tests/trafficmonitor_bridge_probe.py <DLL>` 可运行真实 DLL 的隔离测试。
该测试使用模拟主程序，不能代替真实 TrafficMonitor 的一致性核对或屏幕实机验收。

## v15 可配置卡片与真实数据范围

网页左栏为页面管理、系统检测、明信片、设置。系统检测内最多六张卡片，可选项目、
拖动或按钮排序、开关和删除；全部关闭显示空状态。卡片与页面开关、顺序独立保存，
重连读取设备实际值。页面管理至少保留一页。

CPU 频率通过公开 GHz getter 转为 MHz，不用回调中截断的整数 GHz。硬盘利用率沿用
主程序选定硬盘。公开内存接口只有已用百分比，不反推容量。V1.86 温度为内置采集，
不假定独立插件向主程序转发数据。今日上传／下载是主程序恢复的当日历史加上当天
所选网卡记录的流量；换网卡不会改写先前历史，随主程序本地日期重置。仅标为今日
总流量，不是所有时间累计，也不保证只含当前网卡的今日流量。组合数据需两项均有效。

字节／比特数值按八倍换算，采用 1024 进制；真实零值有效。可设自动／固定单位、
0～2 小数、单位／百分号／空格和简洁显示。窄卡片自动精简长数值，避免裁切。
标题限六字，方向标签限两字；网页和固件共享 GB2312、ASCII 与箭头字形清单，
无法显示的文字明确提示并阻止保存。预览始终为示例，不冒充实时设备数据。
v15 接收端兼容旧 764 字节包，新增项不可用；新版包 1180 字节。新固件协商 SM2
十一项，与支持监测的旧固件协商 SM1 七项；原 Token 协议不变。

已获准加载到本机 TrafficMonitor 1.86 x64，对照同次公开 getter 和格式化文字核对
十一项真实数据。接收器断开重连、真实缓存五秒过期、旧 PID 已退出均已检查。
缺失传感器与拒绝其他版本通过真实 DLL 的隔离模拟验证。v15 尚未刷入 AI Passport，
构建和模拟不代表新固件实机通过。未获新许可不再次重启 TrafficMonitor。

v16 发布包沿用已验证的 v15 桥接 DLL，同一字节文件；仅调整系统检测时钟、网页预览和
电脑程序包版本，不需要再次加载插件。
