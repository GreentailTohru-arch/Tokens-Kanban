[简体中文](README.zh_CN.md)

# Tokens Kanban

[Device manager](https://greentailtohru-arch.github.io/Tokens-Kanban/) · [Tokens Kanban v23](https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/tag/v23)

View local Codex token usage, quota, model/activity trends, TrafficMonitor system metrics and a personal postcard on AI Passport. Costs are API-price estimates, not subscription bills; usage covers readable local history.

## v32 · 2026-10-18

- Triple-press OK on any page to return to the previous enabled page; double press advances. Triple presses no longer first trigger double navigation.
- Reserve OK triple for navigation and remove it from pet customization, preserving other bindings. Match navigation in the web simulator.
- Read postcard images and text from the device while retaining drafts if reading fails.
- Replace the separate local-shortcut save button with unified pet-button/top-save handling for computer and device settings.


## v31 · 2026-10-18

- Configure independent custom shortcuts for up to eight pet gestures. Editing or clearing one binding leaves the others unchanged.
- Each gesture opens the full keyboard picker and displays its own confirmed combination; adjustment lives in the dropdown and the close button matches other dialogs. Migrate the legacy shared binding without changing dictation preferences.
- Update both firmware and the Windows companion to distinguish custom actions. OK double press still changes pages; OK hold still dictates.
- Retain v30 postcard-text readback, temperature-unit and adaptive monitor-layout fixes.


## v30 · 2026-10-18

- Read postcard text and wallpaper settings when connecting to avoid overwriting them with defaults. Legacy image-only postcards cannot restore text; enter and save it once.
- Fix the system monitor temperature unit (°C) and adapt typography and spacing for one to six cards.
- Choose virtual-pet custom shortcuts in a full desktop-keyboard dialog. Select a single key or modifier combination and confirm; the dialog no longer records physical key presses.
- Refresh the public website and bundled local manager. All custom gestures share one shortcut, executed by the Windows companion in the focused application.


## v29 · 2026-10-18

- This v29 release unifies usage and virtual-pet status labels: idle, running, operating, quota low and dictating.
- Fix Windows undo, continue after stopping, and hold-to-dictate; these three actions were user-tested. Match the dictation binding to Codex; the tested local binding is Alt+H.
- Replace fixed Appshot with a custom shortcut. Click the local manager field and press keys to record single keys or modifier combinations, including function, navigation and numpad keys. Clear disables it. All custom-shortcut gestures share one binding.
- The Windows companion sends the configured shortcut to the foreground app. System-reserved keys and application support for synthetic input remain limitations. Browser recording, saving and program tests passed; arbitrary physical-device combinations require acceptance testing.
- Retain pet, wallpaper, themes, postcards, monitoring and USB/Bluetooth handoff; refresh the bundled manager.
- Include app/full firmware, Windows/MacOS companions, the separate TrafficMonitor bridge, manager web, source, update manifest and unified checksums.
- Every confirmed save plays a visible blue completion sweep, including fast saves; failures do not show success.


## Start and update

1. Extract the v23 Windows package and run `start.cmd` for USB or Bluetooth sync. No Wi-Fi setup is needed; Wi-Fi was removed in v20.
2. Double-press OK to cycle enabled pages. Short Up/Down changes periods on Home/Analytics. Single OK reveals Home details or postcard contacts. Hold Up/Down for volume; OK dismisses the overlay.
3. Manage pages, metric cards, postcard, brightness, screen timeout, supplied recordings and boot logo on the website. Device themes and the interactive real-photo preview are independent from the website day/night appearance.
4. To update, use desktop Chrome/Edge, stop the companion with `start.cmd → 4`, then open the online manager → About → Check updates → Install update. If automatic download is blocked, follow the dialog's application BIN link and select that file. Select USB and install. Reconnect to confirm v23, then restart sync.

The updater verifies chip, partition, size and SHA256, writes `Tokens-Kanban-v23-app.bin` to the application partition and verifies readback without erasing settings, postcard or boot logo. `Tokens-Kanban-v23-full.bin` is a merged image for first installation at **0x0**, not the application BIN requested by the update dialog.

## Features and validation

- Default, Obsidian, Porcelain and green ASCII device palettes with advanced colors. All page thumbnails and postcard background/text follow the palette; photos and QR codes retain their colors.
- Reorder/enable pages with at least one enabled. Up to six metric cards use verified data or `--`; previews are labeled examples.
- Five solid screen colors, color bars and 16-step grayscale previews. v23 supports device pattern tests, ending on a keypress or after about 15 seconds.
- Supplied Codex/OpenCode/Antigravity and startup recordings only. Sound-bank names do not imply usage integrations.
- TrafficMonitor bridge targets **1.86 x64** only; no plugin restart is needed for this release. See System Monitor for downloads and installation.

Build and host tests pass, including merged-image/archive checks, Bluetooth-name UTF-8 boundaries, save failures and persistence models, website compatibility, desktop/mobile layouts and verification of the actual application image by the web updater. Windows packaging and its executable entry point were checked. **v23 has not been flashed to a device.** Real Bluetooth renaming, client name caches, uninterrupted sync during renaming and reboot persistence remain unverified. macOS connection and login startup require testing on a Mac. No community submission was made.

[Connection guide](firmware/docs/tokens-connections.md) · [Changelog](CHANGELOG.md) · [Source](firmware/) · BSP: [FoloToy AI Passport](https://gitee.com/FoloToy/ai-passport)

## v23 sync packages

Windows: start.cmd menu 1 starts, 2 enables login startup, 3 disables it, 4 stops sync. macOS: Python 3.10+ and start.command; real-Mac operation is unverified and system metrics are unavailable. The plugin is a separate download.

Saved device names also update Bluetooth; long advertisements preserve UTF-8 boundaries. See [name linking](firmware/docs/bluetooth-device-name.md).

## v28 · 2026-10-18

- v28 asset refresh: optional wallpaper navigation, consistent previews, a 15-second idle option, and top progress for all saves.
- Request immediate device page refresh; visible scanning still requires physical acceptance.
- Add virtual pet and fullscreen wallpaper pages. New defaults: usage, analytics, virtual pet, system monitor, postcard, wallpaper; saved custom order is retained.
- Crop and compress wallpapers, use a theme background, and toggle time/date together independently for postcard and wallpaper.
- Support full-screen boot-logo framing and fix duplicate custom logo playback, boot flashing and power-button screen-off behavior.
- Unify web previews, crop dialogs, mobile layouts and navigation; add RTOS branding and version badges.
- Pet buttons are configurable. Sending, undo and stop/resume depend on Codex accessibility support; screenshot and hold-to-dictate still require hardware acceptance and are not advertised as verified.
- Include app/full firmware, Windows/MacOS sync packages, separate TrafficMonitor bridge, source and unified checksums.
