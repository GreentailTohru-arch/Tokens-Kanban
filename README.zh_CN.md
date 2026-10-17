[English](README.md)

# Tokens Kanban

[设备管理网页](https://greentailtohru-arch.github.io/Tokens-Kanban/?v=wifi-19) · [已发布 v18 下载](https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/tag/v18) · [Wi-Fi 使用说明](firmware/docs/tokens-network.zh_CN.md)

把 Codex 用量、额度和活动带在身边，自由配置最多六张电脑检测卡片，也能展示个人明信片。美元金额按公开 API 单价估算，不是订阅扣款。系统检测复用 TrafficMonitor 数据和已选网卡；不可用数据明确显示 --。

## 当前开发版 v19

最新源码和网页加入保存 Wi-Fi 后自动重连、十分钟配网热点、手机扫码管理。新电脑程序让同步与管理网页共用一条连接，连接成功自动打开网页，避免蓝牙相互占用；重复启动会复用现有管理页。**公开固件／电脑安装包仍为 v18；v19 已在本地构建，待 Wi-Fi 实机测试。网页遇到旧固件会明确禁用 Wi-Fi 设置。**

1. 完整解压对应 Windows 包，运行 `start-auto.cmd`，等待 Connected，持续运行以同步 Codex／TrafficMonitor。v19 自动打开共用连接的本机管理页；v18 的同步和直接网页连接分开，线上网页直接占用端口前需先停止 v18 同步。
2. v19 首次用 USB 连接，设置 → Wi-Fi → 填 2.4 GHz 网络与密码 → 保存并自动连接。成功后让 USB／蓝牙再保持一轮同步以配对电脑，拔线后自动发现同网设备；`start-wifi.cmd` 仅用已配对 Wi-Fi。完整路径与重试见说明。
3. v19 长按设备确定显示二维码，上下切到配网并开启加密热点。手机扫码加入，使用自动弹出页或 `http://192.168.4.1/` 填写网络；换网后手机加入同一网络，再扫设备管理二维码。二维码含管理权限，不要公开。被动 NFC 标签可用手机一次写入公开网站，固件不能改写标签。
4. 双击确定循环开启页面；首页／统计页短按上下切周期；首页单击看明细／估算金额，明信片单击看联系方式。长按上下调音量，确定关闭音量条。管理二维码为临时浮层，不增加页面；原设置、明信片、录音与 Logo 分区保留。

## 管理与自定义

六页统一界面：体验模拟 → 页面管理 → 系统检测 → 明信片 → 设置 → 关于。设备至少开一页，系统检测最多六卡，可排序、选内容或全部关闭；预览明确使用示例数据。手机使用侧边抽屉，顶部始终一行。错误红色、其余绿色；只有未保存时关闭才提示。

明信片头像、二维码及开机 Logo 在小窗取景；GIF 使用本地解码器，可恢复原默认 Logo。保留亮度、熄屏、统一音量、用户提供的 Codex／OpenCode／Antigravity 录音与开机音效开关，不支持的字形会阻止保存。网页模拟电源单击熄屏／唤醒、长按关闭；实机独立电源键的单击事件没有 BSP 接口。

桥接仅适配并验证 **TrafficMonitor 1.86 x64**。DLL 装在电脑 `plugins` 目录，网页不能直接安装；已装用户无需为 v19 重启 TrafficMonitor。RAM 只显示真实百分比，今日流量不是累计流量。[桥接安装说明](firmware/companion/trafficmonitor/README.zh_CN.md)。

关于页检测 GitHub 公开版本，提供对应下载和核对型号／分区／SHA256 的 USB 安装。桌面刷写仍需安全或本机网页、Web Serial 与 USB，手机 Wi-Fi 管理不提供无线刷机。0x0 合并固件可能重置保存数据，保留数据应核对兼容分区后只更新应用，不全片擦除。

## 验证

v19 完整构建、合并镜像与归档、C／Python 测试、真实 LVGL 渲染／字体和二维码解码、GIF 导入、网页回归及 320／390 布局通过。最终 Windows EXE 的全部资源已逐字节核对；在真实 v18 设备上，网页 USB 管理与同步并行，原设置／页面／卡片／名字／Logo 读回一致，运行时间连续、未重启。

**v19 实机测试：未执行。** 真实 Wi-Fi 配网／开机重连、手机自动弹出、无线共存和长期内存、手机实扫及 NFC 写入仍待验证。此前 v18 显示／声音／操作验收及未变 DLL 的 TrafficMonitor 结果不代替新版无线验证；本轮未重启 TrafficMonitor，未提交社区审核。

[源码](firmware/) · [开发历史](firmware/docs/codex-token-display.zh_CN.md) · BSP 来源：[FoloToy AI Passport](https://gitee.com/FoloToy/ai-passport)。保留原许可证及用户录音；音效名称不代表增加其他工具的用量统计。
