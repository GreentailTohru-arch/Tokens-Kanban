[English](README.md)

# Tokens Kanban

[打开设备管理网页](https://greentailtohru-arch.github.io/Tokens-Kanban/)

把 Codex 用量、剩余额度和运行状态带在身边，翻页查看模型排行、活动热图与趋势，
或展示带头像和联系二维码的个人明信片。金额是 API 单价估算，并非订阅扣款。
统计仅覆盖可读取的本机 Codex 历史，不代表跨设备终身用量。

## 开始使用

1. Windows 电脑安装并登录 Codex，下载并解压
   [v9 配套程序](https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/tag/v9)。
   设备无需配网，实时账户额度需要电脑联网。
2. USB 连接后运行 `start-auto.cmd`；蓝牙运行 `start-bluetooth.cmd`，等待 `Connected`。
   双击确定切换首页、统计和明信片；前两页上下切换周期。首页单击看明细和金额估算，
   明信片单击看联系方式。
3. 先关闭同步程序，在 Chrome／Edge 打开管理网页。左侧切换“明信片／设置”，
   通过 USB 或蓝牙连接，编辑后点“保存到设备”。网页断开后再启动同步。

头像和二维码使用小窗取景。图片在浏览器本地处理，直接发往设备，不会经网页上传至
GitHub。已有卡片在保存成功前不会替换。默认头像是原创左右对称矢量图，
源文件为 `default-avatar.svg`。

## 设置与音效

可设置自动熄屏、音量，并分别选择结束／额度音效：关闭、Codex、OpenCode 或
Antigravity。仅使用提供的录音，无合成预设。这些是 Codex 事件的音效套装，
不代表接入 OpenCode 或 Antigravity 用量。固件与电脑程序均建议使用 v9。
“试听”在设备播放且不保存；“保存到设备”后正常重启仍保留。默认不自动熄屏、
Codex／Codex、35% 音量。任意键唤醒熄屏且首次不翻页，同步和声音继续运行。
只有新鲜的 Codex 任务完成事件触发结束提示，启动历史和取消不会触发；
额度在每段观察到的耗尽期只提醒一次。

## 源码与验证

[firmware/](firmware/) 为完整 v9 源码，包含原始录音、测试、同步程序及复用 BSP。
[实现与测试详情](firmware/docs/codex-token-display.zh_CN.md)。构建、主机和浏览器测试通过；
设备已验收指定音效及有限时长的蓝牙同步。长期、距离和压力测试未验证，刷入后 USB
占用，未采集 v9 启动串口日志。
[v9 Release](https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/tag/v9)
提供从 0x0 刷写的合并镜像及 Windows 程序。完整合并刷新前请记录个人设置，
不要全片擦除。

BSP 来自 [FoloToy AI Passport](https://gitee.com/FoloToy/ai-passport)，保留相应授权与声明。
默认头像为原创 MIT 授权作品。
