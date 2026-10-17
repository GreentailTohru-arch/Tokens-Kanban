<p align="right"><strong>简体中文</strong> · <a href="macos-companion.md">English</a></p>

# macOS 电脑同步预览版

此源码运行包复用 v23 设备协议、Codex 本地统计、USB／蓝牙自动选择和本机管理网页。
它不是签名的独立 App。目前没有进行 Mac 运行或硬件验证，跨平台逻辑测试不能代替实机验收。

## 开始使用

1. 如未安装 Python，请从 <https://www.python.org/downloads/macos/> 安装 Python 3.10 或更新版本。
   将整个压缩包解压到固定、可写的位置，保留全部内容。
2. 打开 `start.command`。首次需要联网，将固定版本的依赖安装到包内 `app/.runtime` 虚拟环境，
   后续复用。如果解压后丢失执行权限，可打开终端，输入 `bash `，将 `start.command` 拖入窗口并回车。
3. 选 **1** 或回车开始同步。一台 Passport 接 USB，或按提示允许终端使用蓝牙。
   优先 USB，拔线后尝试蓝牙。通过自动打开的本机网页管理共享连接。
4. 选 **2** 开启下次登录后台同步，选 **3** 取消，选 **4** 停止正在运行的同步。
   前台同步也可按 Ctrl+C 停止。保留程序目录；移动后重新准备环境并启用自启。
   取消自启不会停止当前进程。

本地统计读取 `CODEX_HOME` 或 `~/.codex`。额度需要 PATH 中可找到且已登录的 Codex CLI；
无法获取时显示 `--`。启动器补充常见 Homebrew 可执行目录，不修改用户的 shell 配置。
包内不含个人密码或密钥。Apple Silicon 和 Intel 使用各自电脑安装的 Python 与依赖，两种架构均待验证。

## 限制与登录启动

TrafficMonitor 和现有桥接依赖 Windows 接口。Mac 的系统监测项目（含温度）暂时显示 `--`，
不会伪造数据或安装额外传感器采集器。当前固件可能提示“TrafficMonitor 未连接”。
Token 与管理功能独立运行。

登录启动仅写入当前用户、本程序拥有的 LaunchAgent：
`~/Library/LaunchAgents/io.github.greentailtohru-arch.tokens-kanban.sync.plist`。
下次登录生效，不自动打开网页，没有 KeepAlive 反复重启设置，也不加载、卸载或修改其他任务。
先确认前台连接成功，再启用。后台蓝牙权限可能与终端不同，仍需真实 Mac 测试；
若被拒绝，使用前台同步并取消自启，不绕过 macOS 的保护机制。
启动错误记录在 `~/Library/Logs/TokensKanban`，常规后台日志和连接状态在 `~/.local/share/TokensKanban`。

使用网页刷机前停止同步以释放串口。浏览器直接管理 USB／蓝牙及刷机受浏览器支持限制，尚未在 Mac 验证。
不要让两台电脑的同步程序同时连接同一设备。

## 验证范围

跨平台测试覆盖启动项序列化、空格和中文路径、同名启动项保护、重复启用／取消、Mac USB 识别、
Windows 传感器不可用处理。Mac 安装、蓝牙权限、USB／蓝牙切换、登录启动和设备显示仍待实测。

参考：[Apple LaunchAgents](https://developer.apple.com/library/archive/documentation/MacOSX/Conceptual/BPSystemStartup/Chapters/CreatingLaunchdJobs.html)、
[Bleak macOS 权限](https://bleak.readthedocs.io/en/latest/backends/macos.html)。
