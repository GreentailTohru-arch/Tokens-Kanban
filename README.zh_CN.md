[English](README.md)

# Tokens Kanban

[打开管理网站](https://greentailtohru-arch.github.io/Tokens-Kanban/) · [Tokens Kanban v22](https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/tag/v22)

在 AI Passport 查看 Codex 本机 Token 用量、额度、模型与活动趋势，搭配 TrafficMonitor 系统检测和个人明信片。金额按 API 单价估算，不是订阅账单；统计范围是电脑可读取的本地历史。

## 上手与更新

1. 下载 v22 Windows 包，完整解压后运行 `start-auto.cmd`，通过 USB 或蓝牙同步。设备无需配网，v20 起已移除 Wi-Fi。
2. 双击确定循环开启的页面；首页、统计页短按上下切换周期。首页单击看明细，明信片单击看联系方式。长按上下调音量，确定关闭音量条。
3. 管理网页可配置页面、检测卡片、明信片、亮度、熄屏、录音和开机 Logo，并提供独立主题及可操作的实机外观预览。网页日夜模式独立于设备主题。
4. 更新固件：使用桌面 Chrome／Edge，先退出电脑同步程序（`stop-sync.cmd`），打开线上管理网站 → 关于 → 检查更新 → 安装更新。若自动下载受限，点击窗口中的应用 BIN 下载链接，选回该文件，再选择 USB 并安装。安装完成后重新连接，确认设备显示 v22，再启动同步程序。

网页安装使用 `Tokens-Kanban-v22-app.bin`，校验型号、分区、大小和 SHA256，写入应用区并读回校验；不全片擦除，保留设置、明信片和开机 Logo。`Tokens-Kanban-v22-full.bin` 是供首次安装从 **0x0** 刷写的完整合并镜像，不要在网页的应用 BIN 选择框中选择它。

## 功能与兼容

- 四种设备主题：默认、黑曜、瓷白、绿色 ASCII；支持高级颜色自定义。明信片背景与文字、各页面缩略图同步配色，照片与二维码保持原色。
- 页面可开关及排序，至少开启一页。系统检测最多六卡，使用真实可用数据，缺失显示 `--`；预览明确使用示例数据。
- 屏幕测试有五种纯色、彩条与 16 阶灰度，本地可预览；连接 v22 后可发送实机测试，按键或约 15 秒后退出。
- 仅保留用户提供的 Codex、OpenCode、Antigravity 录音和开机录音；名称不代表其他 AI 工具用量统计。
- TrafficMonitor 桥接仅标注 **1.86 x64**，已装插件无需为本版重启。下载与安装说明在网站“系统检测”页。

## 验证

Build：完整构建、合并镜像和匹配调试归档校验通过。Host tests：协议、四种主题、明信片原色保留、屏幕图案、模拟按键、桌面及手机布局通过。电脑程序已核对资源，并与真实 v20 设备持续 USB 同步，原设置未变、未重启。

Device tests：**v22 尚未刷入实机**。主题实际显示、切页刷新观感、彩条／灰度、真实蓝牙保存及长期运行仍待验证。未提交社区审核。

[完整连接说明](firmware/docs/tokens-connections.zh_CN.md) · [更新记录](CHANGELOG.zh_CN.md) · [源码](firmware/) · BSP 来源：[FoloToy AI Passport](https://gitee.com/FoloToy/ai-passport)
