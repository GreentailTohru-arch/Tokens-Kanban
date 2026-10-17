[简体中文](README.zh_CN.md)

# Tokens Kanban

[Device manager](https://greentailtohru-arch.github.io/Tokens-Kanban/?v=wifi-19) · [Published v18 downloads](https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/tag/v18) · [Wi-Fi guide](firmware/docs/tokens-network.md)

Keep Codex usage, quota and activity on AI Passport, arrange up to six computer-monitoring cards, and carry a personal postcard. Cost is an API-price estimate, not a subscription charge. Monitoring uses TrafficMonitor's existing data and selected adapters; unavailable values show --.

## Current development: v19

The latest source and management website add saved Wi-Fi auto-reconnect, a ten-minute setup hotspot and phone QR entry. The new computer program shares a physical connection between synchronization and its automatically opened management page, avoiding competing BLE connections. Reopening the program reuses its management page. **The published firmware/Windows release is still v18. v19 has been built locally and awaits Wi-Fi hardware testing; this website explicitly disables Wi-Fi settings on older firmware.**

1. Extract the matching Windows package. Run `start-auto.cmd`, keep it running for Codex/TrafficMonitor data, and wait for Connected. v19 automatically opens the shared local manager; v18 uses separate synchronization and direct browser sessions, so stop its sync program before directly opening its port from the public website.
2. For v19 Wi-Fi, first connect USB. Settings → Wi-Fi → enter the 2.4 GHz network and password → Save and connect. Keep USB/BLE through another synchronization cycle to pair the computer, then unplug USB for automatic LAN discovery. `start-wifi.cmd` selects paired Wi-Fi only. Details and failure recovery are in the guide.
3. On v19, hold device OK for its management QR; Up/Down selects setup and starts a protected hotspot. Phone: scan to join, use the captive window or `http://192.168.4.1/`, then fill network settings. After changing networks, join the same LAN and rescan the device. QR codes contain private management access; do not publish them. NFC can hold the public website address after writing the passive tag with a phone; firmware cannot write that tag.
4. Double OK cycles enabled pages. Short Up/Down changes the period on Home/Analytics; single OK toggles Home details/cost or postcard contact. Hold Up/Down for volume; OK closes the bar. The management QR is a temporary overlay, not an extra page. Existing settings/postcard/recordings/logo partitions remain unchanged.

## Manage and customize

Six consistent website sections: Experience → Pages → System Monitor → Postcard → Settings → About. At least one device page stays enabled; up to six monitoring cards may be selected/reordered, or all cards may be off. Preview data is explicitly illustrative. Phone navigation uses a drawer; the toolbar remains one row. Errors are red and other notices green. Closing prompts only for unsaved drafts.

Use small crop dialogs for postcard images, QR images and boot logos. GIF logos include a bundled local decoder; restore default retains the original logo. Brightness, screen timeout, shared volume, supplied Codex/OpenCode/Antigravity recordings and startup-sound switch remain available. Unsupported custom glyphs block saving. The simulator's power button supports short-click screen-off/wake and hold-to-power-off; the physical power switch's short event is not exposed by the BSP.

Only TrafficMonitor **1.86 x64** has been adapted and validated. The unchanged bridge DLL goes in its `plugins` folder on the computer; the website cannot install it. Already installed users need no TrafficMonitor restart for v19. RAM exposes real percent only; today's traffic is not lifetime traffic. [Bridge installation](firmware/companion/trafficmonitor/README.md).

About checks GitHub's public release and offers matching downloads/USB installation with chip, partition and SHA256 checks. Desktop Web Serial installation requires a secure/loopback browser context and USB; phone Wi-Fi management does not provide wireless firmware installation. A 0x0 merged flash can reset stored data; use a checked compatible application update to preserve data, without full-chip erase.

## Verification

v19: ESP-IDF full build, merged-image/archive, C/Python host tests, actual LVGL rendering/font checks and QR decoding, GIF import, browser management/regression and 320/390-pixel layouts passed. Final Windows EXE resources were checked byte-for-byte. Its shared USB manager was tested against the real v18 device while synchronization continued; settings/page/card/name/logo readbacks stayed identical and uptime continued without reset.

**v19 device tests: NOT RUN.** Physical Wi-Fi joining/boot reconnect, captive pop-ups, BLE/Wi-Fi coexistence, sustained memory, phone scanning and NFC writing remain unverified. Earlier user acceptance of v18 display/sound/controls and unchanged-DLL TrafficMonitor results do not validate these new radio features. TrafficMonitor was not restarted, and no new community review was submitted.

[Source](firmware/) · [application history](firmware/docs/codex-token-display.md) · BSP: [FoloToy AI Passport](https://gitee.com/FoloToy/ai-passport). Existing licenses and user-provided recordings are retained; sound names do not imply usage tracking for those additional tools.
