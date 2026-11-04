<p align="right"><strong>EN</strong> | <a href="README.zh_CN.md">CN</a></p>

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/brand-dark.svg">
    <img src="docs/brand-light.svg" alt="Tokens Kanban logo" width="112" height="112">
  </picture>
</p>

<h1 align="center">Tokens Kanban&nbsp;<img src="docs/assets/badges/rtos.svg" alt="RTOS" width="58" height="20"></h1>

<p align="center"><em>A live Codex usage, quota, and activity display for FoloToy AI Passport.</em></p>

<p align="center">
  <a href="https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/latest"><img src="docs/assets/badges/release.svg" height="20" alt="Latest release v33"></a>
  <a href="https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/latest"><img src="docs/assets/badges/downloads.svg" height="20" alt="v33 asset downloads at the time of this README update: 13"></a>
  <a href="https://greentailtohru-arch.github.io/Tokens-Kanban/assets/badges/windows.svg"><img src="docs/assets/badges/windows.svg" height="20" alt="Windows sync"></a>
  <a href="https://greentailtohru-arch.github.io/Tokens-Kanban/assets/badges/macos.svg"><img src="docs/assets/badges/macos.svg" height="20" alt="macOS sync"></a>
  <a href="LICENSE"><img src="docs/assets/badges/license.svg" height="20" alt="MIT license"></a>
</p>

<p align="center">
  <a href="https://greentailtohru-arch.github.io/Tokens-Kanban/"><img src="docs/assets/device-manager-cta.svg" width="276" height="54" alt="Open Device Manager"></a>
</p>

## Showcase

<table align="center">
  <tr>
    <td align="center" width="33%"><img src="docs/assets/device-screens/usage.png" width="224" alt="Usage and quota"><br><strong>Usage Overview</strong></td>
    <td align="center" width="33%"><img src="docs/assets/device-screens/analytics.png" width="224" alt="Model usage and activity trends"><br><strong>Analytics</strong></td>
    <td align="center" width="33%"><img src="docs/assets/device-screens/pet.png" width="224" alt="Virtual pet"><br><strong>Virtual Pet</strong></td>
  </tr>
  <tr>
    <td align="center" width="33%"><img src="docs/assets/device-screens/system-monitor.png" width="224" alt="System monitor with sample data"><br><strong>System Monitor</strong></td>
    <td align="center" width="33%"><img src="docs/assets/device-screens/postcard.png" width="224" alt="Postcard with the default avatar"><br><strong>Postcard</strong></td>
    <td align="center" width="33%"><img src="docs/assets/device-screens/wallpaper-clock.png" width="224" alt="Fullscreen wallpaper showing the time and date"><br><strong>Fullscreen Wallpaper</strong></td>
  </tr>
</table>

## Features

- View Codex token usage, model breakdowns, activity trends, and account limits. Cost estimates use API pricing and are separate from subscription charges.
- Customize page order, themes, wallpaper, postcards, and the startup logo. The virtual pet reflects Codex activity.
- Display Windows system metrics with the optional TrafficMonitor 1.86 x64 bridge.

## Get started

1. Download and extract the sync package for your operating system from the [latest release](https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/latest).
2. Run the launcher and select **1** to start syncing. On Windows, use `start.cmd`. On macOS, use `start.command` with Python 3.10 or later. The first macOS launch needs internet access to install dependencies. See the [macOS setup guide](firmware/docs/macos-companion.md).
3. Connect the AI Passport by USB or Bluetooth Low Energy. Use the [Device Manager](https://greentailtohru-arch.github.io/Tokens-Kanban/) to preview and configure the display.

Double-press **OK** for the next page and triple-press it for the previous page. On the usage and analytics pages, press **Up** or **Down** to change the time range. Hold either button to adjust volume.

## Firmware

- **Update an existing installation.** Stop desktop sync, connect by USB, and open **About & Updates → Check for updates → Install update** in the Device Manager using desktop Chrome or Edge. If prompted for a file, select the application image ending in `-app.bin`.
- **Install from scratch.** Flash the image ending in `-full.bin` at `0x0` with an ESP32-C3 flashing tool. A full flash can reset saved settings.

## Development

Firmware and the desktop companion live in [firmware/](firmware/). The Device Manager has a [GitHub Pages copy](docs/) and a [bundled copy](firmware/companion/postcard/). Keep both in sync when changing the web UI.

Set up [ESP-IDF 5.5.3](firmware/docs/development/engineering/environment-setup.md), then run `./tools/validate.sh` from `firmware/` to build and validate the firmware. See the [build and test guide](firmware/docs/development/engineering/build-and-test.md) and [changelog](docs/changelog/CHANGELOG.md).

Based on [FoloToy AI Passport](https://github.com/FoloToy/ai-passport). Licensed under [MIT](LICENSE).
