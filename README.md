[简体中文](README.zh_CN.md)

# Tokens Kanban

[Open the postcard editor](https://greentailtohru-arch.github.io/Tokens-Kanban/)

A static AI Passport postcard editor. Connect over Web Bluetooth or Web Serial in Chrome/Edge,
frame the avatar and QR in a modal, edit the text, and save both faces directly to the device.
The browser processes images locally; this repository only hosts the editor and public default assets.
Stop the desktop usage-sync connection before connecting the editor. Requires compatible Passport firmware.

The original symmetric avatar and editor-link QR are defaults. Existing personal cards remain on the device
until a successful save. Uploaded images are never committed or sent to GitHub by the editor.

The hardware application uses the BSP from [FoloToy AI Passport](https://gitee.com/FoloToy/ai-passport).
Code is distributed under the accompanying MIT license. The default generic avatar is original MIT-licensed vector artwork; `default-avatar.svg` is its source.

## Firmware and desktop companion

The current v6 complete source is in [firmware/](firmware/), including the BSP, application, tests and
computer sync program. [Download the Windows companion](https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/tag/v6).
This turns Passport into a live Codex usage dashboard with token/cost views, trends and a personal postcard.
Use Up/Down to switch periods, single OK for details/contact, and double OK to cycle the three pages.
The device needs no Wi-Fi setup; current account quotas require the signed-in computer to be online.
