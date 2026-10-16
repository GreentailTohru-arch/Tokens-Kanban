[简体中文](README.zh_CN.md)

# Tokens Kanban

[Open the device manager](https://greentailtohru-arch.github.io/Tokens-Kanban/)

Keep Codex usage, remaining quota and live status in sight. Browse model rankings,
activity and trends, or flip to a personal postcard with a portrait and contact QR.
Amounts are estimates at API prices, not subscription charges. Statistics cover
readable local Codex history, not lifetime usage across devices.

## Start

1. Install Codex and sign in on Windows. Download and extract the
   [v9 Windows companion](https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/tag/v9).
   The device needs no Wi-Fi setup; live account quotas require the computer online.
2. Connect by USB and run `start-auto.cmd`, or run `start-bluetooth.cmd` for Bluetooth.
   Wait for `Connected`. Double-press OK for Home, Analytics and Postcard;
   Up/Down changes periods on the first two pages. Single OK opens Home details
   and estimated cost, or the postcard contact face.
3. Stop the companion before connecting the manager in Chrome/Edge. Use the left
   sidebar for Postcard or Settings; connect by USB or Bluetooth, edit and choose
   **Save to device**. Disconnect the manager before restarting sync.

Avatar and QR framing uses a modal. Images are processed locally and sent directly
to the device; the website never uploads personal cards to GitHub. Existing cards
remain until a successful save. The default avatar is original symmetric vector
artwork, available as `default-avatar.svg`.

## Settings and sounds

Choose automatic screen-off, volume, and independent completion/quota sounds:
Off, Codex, OpenCode or Antigravity. Only the supplied recordings are used;
there are no generated presets. These are sound choices for Codex events, not
OpenCode or Antigravity usage integrations. Both firmware and companion should
be v9. Preview plays on the device without saving; **Save to device** retains
settings across normal restart. Defaults are Never, Codex/Codex and 35% volume.
Any key wakes a dark screen without also navigating; sync and sounds continue.
Fresh completed Codex tasks trigger completion alerts; cancelled tasks and
startup history do not. Exhausted quota alerts once per observed exhausted period.

## Source and verification

The complete v9 source, original recordings, tests, companion and reused BSP are
in [firmware/](firmware/). [Application and test details](firmware/docs/codex-token-display.md).
Build and host/browser checks passed. Device acceptance covered the supplied
sounds and bounded BLE synchronization. Long-run/range/stress tests remain
unverified; USB was busy after flashing, so no v9 startup console was captured.
The [v9 release](https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/tag/v9)
contains the merged image for 0x0 flashing and Windows program. Back up personal
settings before a complete merged-image refresh; do not erase the whole device.

The BSP comes from [FoloToy AI Passport](https://gitee.com/FoloToy/ai-passport).
See the accompanying license and retained notices. The default avatar is original
MIT-licensed artwork.
