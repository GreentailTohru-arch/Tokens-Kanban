[简体中文](README.zh_CN.md)

# Tokens Kanban

[Open the postcard editor](https://greentailtohru-arch.github.io/Tokens-Kanban/)

A static AI Passport postcard editor. Connect over Web Bluetooth or Web Serial in Chrome/Edge,
frame the avatar and QR in a modal, edit the text, and save both faces directly to the device.
The browser processes images locally; this repository only hosts the editor and public default assets.
Stop the desktop usage-sync connection before connecting the editor. Requires compatible Passport firmware.

The supplied generic avatar and editor-link QR are defaults. Existing personal cards remain on the device
until a successful save. Uploaded images are never committed or sent to GitHub by the editor.

The hardware application uses the BSP from [FoloToy AI Passport](https://gitee.com/FoloToy/ai-passport).
Code is distributed under the accompanying MIT license. The default generic avatar was supplied by the
project owner for this application; its original source/license was not specified.
