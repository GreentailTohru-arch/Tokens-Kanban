[简体中文](README.zh_CN.md)

# Tokens Kanban

[Device manager](https://greentailtohru-arch.github.io/Tokens-Kanban/) ·
[Tokens Kanban v18 downloads](https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/tag/v18)

Keep Codex tokens, quota and activity on AI Passport, view configurable computer
monitoring cards, or display a personal postcard. API cost is an estimate, not a
subscription charge. Usage covers readable local Codex history only.

## Start

1. Install/sign in to Codex on Windows and extract the v18 Windows ZIP. For computer
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

v18 full build/archive, host models, native LVGL layout/fonts and diagnostic color
overlays, browser USB/BLE simulations, packaged editor resources, supplied-recording
decoding and 320/390-pixel mobile layouts passed. Real TrafficMonitor validation is
the earlier v15/v16 result for the identical DLL; it was not restarted this time.
v18 has NOT been flashed: new device telemetry, microphone/speaker/screen tests,
name persistence after power loss and long-run/range tests remain unverified.
The user accepted v16 date, cards, volume controls and rebooted volume retention;
those results do not validate v18's new diagnostic interfaces.

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

Connect matching v18 firmware to use the separate live Passport dashboard. The
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

## Display, startup images and updates

Settings includes 5–100% screen brightness and Never screen-off. Brightness persists
independently and is restored on wake. The separate Startup Logo editor accepts
PNG/JPEG/WebP or GIF, with modal cropping, animated draft preview, Save and Restore
default. The supplied default remains unchanged. GIF needs ImageDecoder support;
conversion limits source size/frame count and adapts up to six seconds to the
device's bounded storage. Reconnect reads saved metadata, not the custom pixels.

About shows the actual device firmware version separately from the website version.
Check Update reads this project's GitHub Releases and tagged manifest. Install opens
a USB-only workflow with chip, known-partition and SHA256 checks, followed by readback.
If browser CORS blocks automatic downloading, download the linked application BIN
from GitHub and select it in the same dialog. Only Select USB and Install starts
installation. The complete BIN button downloads the separate 0x0 image.

v18 reserves an independent double-slot logo partition in previously unused app
capacity, keeping NVS/PHY/postcard offsets and sizes. Upgrades require the matching
application and, when needed, partition table; application-only v16/v17 upgrades
cannot provide logo storage until migration. The web installer writes these two
verified regions without full-chip erase. Unknown layouts stop before writes.
The bridge card links TrafficMonitor and a short instruction dialog; companion
downloads remain in Releases. Real brightness, GIF playback, physical USB migration
and power-loss preservation remain unverified because v18 has not been flashed.

Web notifications now appear at the top, green for normal/success and red for errors. A Save Changes summary tracks configuration drafts; closing/reloading warns only while unsaved. Canceling departure keeps Bluetooth connected. Native browser exit dialogs cannot save to the device: stay, then use Save Changes. Keep screen on (not recommended) is the last screen-off option. Firmware and the v18 release binaries are unchanged.
