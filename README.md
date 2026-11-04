<p align="right"><strong>EN</strong> | <a href="README.zh_CN.md">CN</a></p>

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/brand-dark.svg">
    <img src="docs/brand-light.svg" alt="Tokens Kanban logo" width="112" height="112">
  </picture>
</p>

<h1 align="center">Tokens Kanban&nbsp;<img src="docs/assets/badges/rtos.svg" alt="RTOS" width="49" height="22"></h1>

<p align="center"><em>A live Codex usage, quota, and activity display for FoloToy AI Passport.</em></p>

<p align="center">
  <a href="https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/latest"><img src="docs/assets/badges/release.svg" height="24" alt="Latest release v33"></a>
  <a href="https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/latest"><img src="docs/assets/badges/downloads.svg" height="24" alt="v33 asset downloads at the time of this README update: 13"></a>
  <a href="https://greentailtohru-arch.github.io/Tokens-Kanban/assets/badges/windows.svg"><img src="docs/assets/badges/windows.svg" height="24" alt="Windows sync"></a>
  <a href="https://greentailtohru-arch.github.io/Tokens-Kanban/assets/badges/macos.svg"><img src="docs/assets/badges/macos.svg" height="24" alt="macOS sync"></a>
  <a href="LICENSE"><img src="docs/assets/badges/license.svg" height="24" alt="MIT license"></a>
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

## What you can see

- **Codex usage** — token counts, model and activity trends, quota, and API-price estimates based on readable local history. Estimates are separate from subscription charges.
- **Virtual pet** — idle, thinking, and quota animations. The disconnected state shares the quota animation and has its own label.
- **Personal display** — configurable page order, themes, wallpaper, postcard, and startup logo.
- **System metrics** — supported Windows metrics through the separate TrafficMonitor 1.86 x64 bridge.

## Get started

| Task | What to use |
| --- | --- |
| Sync on Windows | Extract the Windows sync ZIP, run `start.cmd`, and select **1**. Use **2** for login startup and **4** to stop synchronization. |
| Sync on macOS | Extract the macOS ZIP and run `start.command` with Python 3.10 or later. The first launch installs dependencies. |
| Manage or preview the device | Open the device manager. **Page Management** controls which device pages appear and in what order. |
| Update an existing installation | Stop the sync program, connect via USB, and open **About & Updates → Install update** in desktop Chrome or Edge. |
| Install from scratch | Use the full firmware image with a compatible ESP32-C3 flashing tool. The image starts at `0x0` and can reset saved settings. |

The sync program supports USB and Bluetooth Low Energy. The browser updater uses the application image (`app.bin`) at `0x10000` and validates the target partition and image checksum. Find both firmware images, sync packages, and SHA-256 checksums in the latest release.

On the device, double-press OK to advance and triple-press it to go back. On the usage and analytics pages, press Up or Down to change the time range. Hold Up or Down to adjust volume.

## Repository layout

| Path | Contents |
| --- | --- |
| [`firmware/`](firmware/) | ESP-IDF 5.5.3 firmware, desktop companion, build tools, and host tests |
| [`docs/`](docs/) | GitHub Pages site and [version history](docs/changelog/CHANGELOG.md) |
| [`update.json`](update.json) | Firmware update manifest read by the manager from the release tag |

The browser manager also runs locally from the desktop companion. Its source is under [`firmware/companion/postcard/`](firmware/companion/postcard/). Keep the published Pages copy and the companion copy in sync when changing the manager. To build the firmware, follow the [ESP-IDF setup guide](firmware/docs/development/engineering/environment-setup.md), then run `./tools/validate.sh` from `firmware/`. See the [build and test guide](firmware/docs/development/engineering/build-and-test.md) for details.

Released firmware has passed build and host checks. v33 was flashed to an AI Passport and reported the expected version over USB. The Windows bundle's local manager was verified. macOS runtime testing is pending.

This project builds on [FoloToy AI Passport](https://github.com/FoloToy/ai-passport). See the [MIT license](LICENSE).
