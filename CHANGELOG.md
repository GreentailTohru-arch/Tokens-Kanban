[简体中文](CHANGELOG.zh_CN.md)

# Changelog

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
