[简体中文](README.zh_CN.md)

# Tokens Kanban

[Device manager](https://greentailtohru-arch.github.io/Tokens-Kanban/) ·
[Tokens Kanban v16 downloads](https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/tag/v16)

Keep Codex tokens, quota and activity on AI Passport, view configurable computer
monitoring cards, or display a personal postcard. API cost is an estimate, not a
subscription charge. Usage covers readable local Codex history only.

## Start

1. Install/sign in to Codex on Windows and extract the v16 Windows ZIP. For computer
   monitoring, put TokensKanbanBridge.dll in TrafficMonitor 1.86 x64's plugins folder
   and restart TrafficMonitor once. The plugin is installed on the computer; the
   website cannot install it. It reads existing data without changing host settings.
2. Connect Passport by USB and run start-auto.cmd, or run start-bluetooth.cmd for BLE.
   Wait for Connected. No device Wi-Fi setup is required; live account quotas need
   the computer online. Double OK cycles enabled pages in their saved order. Up/Down
   changes periods on Home/Analytics. Single OK opens Home details/cost or postcard
   contact. Hold Up/Down for the minimal volume bar; OK dismisses it. A wake key is
   consumed without changing volume or navigating.
3. Stop sync before opening the manager in a desktop Chrome/Edge browser. Connect
   USB/Bluetooth, edit and Save to device. Disconnect the manager before restarting
   sync. Page Management previews/toggles/reorders pages, keeping at least one on.
   System Monitor configures up to six cards, titles and units. All cards may be off.
   Reconnect reads actual saved configuration. Previews use illustrative data.

## Data and customization

Monitoring supports CPU/GPU/RAM utilization, CPU/GPU temperatures, CPU frequency,
disk utilization, upload/download/combined speed and today's traffic. Network data
follows TrafficMonitor's selected adapters; today's historical total is not lifetime
traffic or necessarily only the current adapter. RAM exposes percent only. Missing
values show --; stale/dead sources become unavailable and reconnect recovers.
Only TrafficMonitor 1.86 x64 is verified. See the [bridge guide](firmware/companion/trafficmonitor/README.md).

Postcard avatar/QR crops stay local and are sent directly to the device, never
uploaded to GitHub. Existing cards remain until a successful save. Settings retain
screen-off, shared volume, supplied Codex/OpenCode/Antigravity recordings and the
startup-sound switch. Sound-bank names do not add usage tracking for those tools.
Custom card text is checked against the device font; unsupported glyphs block saving.

## Verification and source

v16 build, firmware archive, actual TrafficMonitor getter/formatted-value agreement,
receiver reconnection/staleness, isolated-DLL missing-sensor tests, simulated browser
USB/BLE saves and native LVGL layout/font checks passed. v16 has NOT been flashed;
physical display/buttons/volume, power-cycle persistence and long-run/range tests
remain unverified. Prior versions' hardware acceptance does not validate v16.

[Complete source](firmware/) and [application notes](firmware/docs/codex-token-display.md)
retain the original BSP and supplied recordings. The release includes the full 0x0
image, bridge, Windows companion, source ZIP, installation guide and SHA256 hashes.
A merged flash may reset settings; preserving data needs a checked application-only
upgrade without full-chip erase. No new community review was submitted.

BSP source: [FoloToy AI Passport](https://gitee.com/FoloToy/ai-passport). Retained
licenses/notices apply; the original symmetric default avatar is MIT licensed.

System Monitor shows the synchronized local HH:MM clock at top left, or 00:00 before
time sync. Battery remains at top right. The bridge DLL is unchanged from the
verified v15 component and needs no further TrafficMonitor restart.
