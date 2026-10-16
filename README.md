[简体中文](README.zh_CN.md)

# Tokens Kanban

[Device manager](https://greentailtohru-arch.github.io/Tokens-Kanban/) ·
[Tokens Kanban v17 downloads](https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/tag/v17)

Keep Codex tokens, quota and activity on AI Passport, view configurable computer
monitoring cards, or display a personal postcard. API cost is an estimate, not a
subscription charge. Usage covers readable local Codex history only.

## Start

1. Install/sign in to Codex on Windows and extract the v17 Windows ZIP. For computer
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

v17 full build/archive, host models, native LVGL layout/fonts and diagnostic color
overlays, browser USB/BLE simulations, packaged editor resources, supplied-recording
decoding and 320/390-pixel mobile layouts passed. Real TrafficMonitor validation is
the earlier v15/v16 result for the identical DLL; it was not restarted this time.
v17 has NOT been flashed: new device telemetry, microphone/speaker/screen tests,
name persistence after power loss and long-run/range tests remain unverified.
The user accepted v16 date, cards, volume controls and rebooted volume retention;
those results do not validate v17's new diagnostic interfaces.

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

## Experience simulator and device information

The first sidebar item is Experience Simulator, followed by Page Management,
System Monitor, Postcard and Settings. The virtual device's left power key holds
for about two seconds to turn off or half a second to start. The three right keys
support mouse, touch and focused keyboard input. They simulate the current browser
draft's page order, card layout, postcard, sounds, idle wake and volume without
writing to the device. Illustrative data is explicitly labeled.

Connect matching v17 firmware to use the separate live Passport dashboard. The
default management name is `You's AI Passport`, editable up to 32 UTF-8 bytes and
saved independently; Bluetooth advertising is unchanged. CPU use/frequency,
allocatable internal heap, Flash/partition sizes, battery SOC/voltage and uptime
come from the device. 500 mAh is the product rating, not measured remaining charge;
heap is not total chip SRAM, and partition allocation is not used file storage.
Unsupported, disconnected or stale readings show `--` rather than sample values.

Explicit test buttons read the battery, sample microphone amplitude for about
0.4 seconds without retaining recordings, play the selected supplied sound,
observe physical key events and show five display colors. Screen testing ends
after 15 seconds or a physical key, consuming that exit action. Confirm speaker
quality by listening; volume zero remains silent. Older firmware disables these
tests and name saving while retaining its existing editor features.
