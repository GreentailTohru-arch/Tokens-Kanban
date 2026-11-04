<p align="right"><a href="README.md">EN</a> | <strong>CN</strong></p>

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/brand-dark.svg">
    <img src="docs/brand-light.svg" alt="Tokens Kanban logo" width="112" height="112">
  </picture>
</p>

<h1 align="center">Tokens Kanban&nbsp;<img src="docs/assets/badges/rtos.svg" alt="RTOS" width="49" height="22"></h1>

<p align="center"><em>在 FoloToy AI Passport 上实时查看 Codex 用量、额度和运行状态。</em></p>

<p align="center">
  <a href="https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/latest"><img src="docs/assets/badges/release.svg" height="20" alt="Latest release v33"></a>
  <a href="https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/tag/v33"><img src="docs/assets/badges/windows.svg" height="20" alt="Windows sync"></a>
  <a href="https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/tag/v33"><img src="docs/assets/badges/macos.svg" height="20" alt="macOS sync"></a>
  <a href="LICENSE"><img src="docs/assets/badges/license.svg" height="20" alt="MIT license"></a>
</p>

<p align="center">
  <a href="https://greentailtohru-arch.github.io/Tokens-Kanban/"><img src="docs/assets/device-manager-cta.svg" width="250" height="55" alt="打开设备管理网页"></a>
</p>

## 界面展示

<table align="center">
  <tr>
    <td align="center" width="33%"><img src="docs/assets/device-screens/usage.png" width="224" alt="用量首页与额度"><br><strong>用量首页</strong></td>
    <td align="center" width="33%"><img src="docs/assets/device-screens/analytics.png" width="224" alt="统计分析与活动趋势"><br><strong>统计分析</strong></td>
    <td align="center" width="33%"><img src="docs/assets/device-screens/pet.png" width="224" alt="虚拟宠物"><br><strong>虚拟宠物</strong></td>
  </tr>
  <tr>
    <td align="center" width="33%"><img src="docs/assets/device-screens/system-monitor.png" width="224" alt="系统检测与示例数据"><br><strong>系统检测</strong></td>
    <td align="center" width="33%"><img src="docs/assets/device-screens/postcard.png" width="224" alt="使用默认头像的明信片"><br><strong>明信片</strong></td>
    <td align="center" width="33%"><img src="docs/assets/device-screens/wallpaper-clock.png" width="224" alt="显示时间和日期的全屏壁纸"><br><strong>全屏壁纸</strong></td>
  </tr>
</table>

## 主要功能

- 查看 Codex Token 用量、模型分布、活动趋势和账户额度。费用按 API 单价估算，与订阅账单分开。
- 自定义页面顺序、主题、壁纸、明信片和开机 Logo。虚拟宠物随 Codex 状态变化。
- Windows 可通过可选的 TrafficMonitor 1.86 x64 桥接插件显示系统指标。

## 快速开始

1. 从[最新发布页](https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/latest)下载并完整解压对应系统的同步包。
2. 运行启动脚本并选择 **1** 开始同步。Windows 运行 `start.cmd`。macOS 运行 `start.command`，需要 Python 3.10 或更高版本，首次启动需联网安装依赖。详见 [macOS 使用指南](firmware/docs/macos-companion.zh_CN.md)。
3. 通过 USB 或低功耗蓝牙连接 AI Passport。打开[设备管理网页](https://greentailtohru-arch.github.io/Tokens-Kanban/)预览和配置设备界面。

双击 **OK** 前进一页，三击返回一页。在用量首页和统计分析页，短按**上键**或**下键**切换统计周期，长按可调节音量。

## 固件安装与更新

- **更新已有固件。** 停止电脑同步，通过 USB 连接设备，在桌面版 Chrome 或 Edge 打开设备管理网页，选择「关于与更新 → 检查更新 → 安装更新」。如果网页提示手动选择文件，请选用以 `-app.bin` 结尾的应用镜像。
- **首次完整安装。** 使用支持 ESP32-C3 的刷写工具，将以 `-full.bin` 结尾的完整镜像写入 `0x0`。完整刷写可能重置已保存的设置。

## 开发

固件和电脑同步程序位于 [firmware/](firmware/)。设备管理网页分为 [GitHub Pages 版](docs/)和[同步包内置版](firmware/companion/postcard/)，修改网页时需同步更新两处。

配置 [ESP-IDF 5.5.3](firmware/docs/development/engineering/environment-setup.zh_CN.md)，进入 `firmware/` 运行 `./tools/validate.sh`，构建并验证固件。详见[构建与测试指南](firmware/docs/development/engineering/build-and-test.zh_CN.md)和[更新日志](docs/changelog/CHANGELOG.zh_CN.md)。

项目基于 [FoloToy AI Passport](https://github.com/FoloToy/ai-passport)，采用 [MIT 许可证](LICENSE)。
