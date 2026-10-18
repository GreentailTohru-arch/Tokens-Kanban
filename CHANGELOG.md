[简体中文](CHANGELOG.zh_CN.md)

# Changelog

## v29 · 2026-10-18

- This v29 release unifies usage and virtual-pet status labels: idle, running, operating, quota low and dictating.
- Fix Windows undo, continue after stopping, and hold-to-dictate; these three actions were user-tested. Match the dictation binding to Codex; the tested local binding is Alt+H.
- Replace fixed Appshot with a custom shortcut. Click the local manager field and press keys to record single keys or modifier combinations, including function, navigation and numpad keys. Clear disables it. All custom-shortcut gestures share one binding.
- The Windows companion sends the configured shortcut to the foreground app. System-reserved keys and application support for synthetic input remain limitations. Browser recording, saving and program tests passed; arbitrary physical-device combinations require acceptance testing.
- Retain pet, wallpaper, themes, postcards, monitoring and USB/Bluetooth handoff; refresh the bundled manager.
- Include app/full firmware, Windows/MacOS companions, the separate TrafficMonitor bridge, manager web, source, update manifest and unified checksums.
- Every confirmed save plays a visible blue completion sweep, including fast saves; failures do not show success.


## v28 · 2026-10-18

- v28 asset refresh: optional wallpaper navigation, consistent previews, a 15-second idle option, and top progress for all saves.
- Request immediate device page refresh; visible scanning still requires physical acceptance.
- Add virtual pet and fullscreen wallpaper pages. New defaults: usage, analytics, virtual pet, system monitor, postcard, wallpaper; saved custom order is retained.
- Crop and compress wallpapers, use a theme background, and toggle time/date together independently for postcard and wallpaper.
- Support full-screen boot-logo framing and fix duplicate custom logo playback, boot flashing and power-button screen-off behavior.
- Unify web previews, crop dialogs, mobile layouts and navigation; add RTOS branding and version badges.
- Pet buttons are configurable. Sending, undo and stop/resume depend on Codex accessibility support; screenshot and hold-to-dictate still require hardware acceptance and are not advertised as verified.
- Include app/full firmware, Windows/MacOS sync packages, separate TrafficMonitor bridge, source and unified checksums.


## v23 · 2026-10-17

- Link the saved device name to Bluetooth without intentionally disconnecting the current peer; reload it after reboot.
- Shorten long advertised UTF-8 names at complete character boundaries while retaining the full stored name.
- Gate website hints by firmware capability and align all downloads with v23.
- Keep a single Windows start.cmd with automatic USB/BLE selection and optional login startup; download the plugin separately.
- Provide a minimal Python-based macOS preview with start.command; Python 3.10+ is required, Mac system metrics and real-Mac verification are unavailable.
- Include application/full firmware, update manifest, installation guide, source and unified SHA256 checksums.
- Reuse the unchanged TrafficMonitor 1.86 x64 bridge; existing users need no replacement or TrafficMonitor restart.
- Build and host tests pass; v23 device renaming and reboot behavior remain unverified. No community submission was made.


## v22 · 2026-10-17

- Add four device palettes and advanced colors across postcard, page thumbnails and simulator; website day/night mode remains independent.
- Restore screen color swatches; add color bars and 16-step grayscale preview/device tests.
- Remove webpage transition effects, publish complete simulator frames and reduce firmware refresh strip count; retain volume slide-in.
- Add day/night TK branding and the interactive real-photo simulator, improve notices/version checks and USB management, and remove Wi-Fi.
- Preserve postcard, page/card management, supplied recordings, display, volume and startup settings.
- For browser update, stop the companion, use desktop Chrome/Edge → About → Check updates → Install update. Choose this release's app.bin if automatic download is blocked.
- The app image is for data-preserving updates; the merged full image installs at 0x0. Build/host/browser/render tests passed; v22 physical behavior is not yet verified.
