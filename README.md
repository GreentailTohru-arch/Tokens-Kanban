[简体中文](README.zh_CN.md)

# Tokens Kanban

[Open the device manager](https://greentailtohru-arch.github.io/Tokens-Kanban/)

A static AI Passport postcard editor and device manager. Connect over Web Bluetooth or Web Serial in Chrome/Edge,
frame the avatar and QR in a modal, edit the text, and save both faces directly to the device.
The browser processes images locally; this repository only hosts the editor and public default assets.
Stop the desktop usage-sync connection before connecting the editor. Requires compatible Passport firmware.

The original symmetric avatar and editor-link QR are defaults. Existing personal cards remain on the device
until a successful save. Uploaded images are never committed or sent to GitHub by the editor.

The hardware application uses the BSP from [FoloToy AI Passport](https://gitee.com/FoloToy/ai-passport).
Code is distributed under the accompanying MIT license. The default generic avatar is original MIT-licensed vector artwork; `default-avatar.svg` is its source.

## Firmware and desktop companion

The current v7 complete source is in [firmware/](firmware/), including the BSP, application, tests and
computer sync program. [Download the Windows companion](https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/tag/v7).
This turns Passport into a live Codex usage dashboard with token/cost views, trends and a personal postcard.
Use Up/Down to switch periods, single OK for details/contact, and double OK to cycle the three pages.
The device needs no Wi-Fi setup; current account quotas require the signed-in computer to be online.

## Device settings (v7)

The left sidebar switches between Postcard and Settings. Stop the desktop sync program,
connect over USB or Bluetooth, then open Settings. Choose an automatic screen-off timeout,
completion tone, quota-exhausted tone and volume. Preview plays on the device; Save to device
persists settings across reboot. Disconnect the website before restarting the v7 companion.

Screen-off turns off only the backlight; any key wakes it without triggering navigation.
The default is Never, Light completion tone, Low quota tone and 35% volume. Sounds can be
disabled separately. Actual fresh Codex task completions trigger the completion tone;
startup history and cancelled tasks do not. Quota audio sounds once per observed exhausted
period. Both firmware and companion must be v7 for completion alerts. Older firmware still
works with Postcard; its Settings controls remain disabled. No Wi-Fi setup is needed.

The v7 firmware build and host/browser checks passed; new sound, idle timing, persistence
and concurrent operation still require device acceptance. v7 has not been flashed.
[Implementation and test details](firmware/docs/codex-token-display.md).
