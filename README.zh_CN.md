[English](README.md)

# Tokens Kanban

[设备管理网页](https://greentailtohru-arch.github.io/Tokens-Kanban/) ·
[Tokens Kanban v16 下载](https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/tag/v16)

在 AI Passport 查看 Codex 用量、额度、模型活动与趋势，配置电脑系统检测卡片，或展示
个人明信片。金额按 API 单价估算，不是订阅扣款；统计仅含可读取的本机 Codex 历史。

## 开始使用

1. Windows 安装并登录 Codex，完整解压 v16 Windows 包。系统检测需将
   TokensKanbanBridge.dll 放入 TrafficMonitor 1.86 x64 的 plugins 文件夹，然后重启
   主程序一次。插件安装在电脑端，网页不能直接安装，不改主程序设置和网卡选择。
2. USB 连接后运行 start-auto.cmd；蓝牙运行 start-bluetooth.cmd，等待 Connected。
   设备无需配网，实时账户额度需要电脑联网。双击确定按保存顺序循环已开启页；首页和
   统计页上下切换周期。首页单击看明细／金额，明信片单击看联系方式。长按上下显示
   简约音量条并调节，确定关闭；熄屏首次操作只唤醒。
3. 关闭同步程序，在桌面 Chrome／Edge 打开管理网页并连接 USB／蓝牙，编辑后点保存
   到设备。网页断开后再启动同步。页面管理提供预览、开关和顺序，至少留一页；系统
   检测可配置最多六张卡片、文字和单位，允许全部关闭。重连读取设备实际配置，预览
   明确使用示例数据。手机布局可查看，不保证手机系统支持网页设备连接接口。

## 数据与个性化

可选 CPU／GPU／RAM 占用、CPU／GPU 温度、CPU 频率、硬盘占用、上传／下载／总网速
以及今日总流量。沿用 TrafficMonitor 选定网卡及历史，今日流量不是累计流量，也不
保证只含当前网卡。RAM 只提供百分比，不反推容量。缺失显示 --，来源退出／过期显示
不可用，恢复后自动重连。目前只验证 TrafficMonitor 1.86 x64；详见
[桥接说明](firmware/companion/trafficmonitor/README.zh_CN.md)。

明信片头像与二维码小窗取景在浏览器本地处理并直接发到设备，不上传 GitHub；成功
保存前保留原卡片。设置保留熄屏、共享音量、用户提供的 Codex／OpenCode／Antigravity
录音和开机音效开关。音效套装名称不代表接入对应工具的用量。自定义卡片文字检查设备
字形，无法显示的字会明确提示并阻止保存。

## 验证与源码

v16 完整构建及归档、真实 TrafficMonitor 同次 getter／格式化数据、接收器重连与
过期检查、隔离 DLL 缺失传感器模拟、网页 USB／BLE 模拟保存、原生 LVGL 排版与字体
检查通过。v16 尚未刷入；实际显示／按键／音量、断电持久化和长期无线仍待验证。
旧版实机验收不代替 v16。

[完整源码](firmware/)与[应用说明](firmware/docs/codex-token-display.zh_CN.md)保留
原 BSP 和提供的录音。Release 含从 0x0 刷写的完整固件、插件、电脑程序、源码 ZIP、
安装说明与 SHA256。完整刷新可能重置设置；保留数据需核对分区后只更新匹配应用，
不要全片擦除。本轮没有提交新的社区审核。

BSP 来自 [FoloToy AI Passport](https://gitee.com/FoloToy/ai-passport)，保留随附许可与
声明。原创对称默认头像按 MIT 授权。

系统检测页左上角显示电脑同步的本地 HH:MM 时间，未同步时为 00:00，右上角电量不变。
桥接 DLL 沿用已实测的 v15 同一文件，不需要再次重启 TrafficMonitor。
