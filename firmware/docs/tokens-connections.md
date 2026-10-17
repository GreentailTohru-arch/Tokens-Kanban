<p align="right"><a href="tokens-connections.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Tokens Kanban v22 connections

v20 removes device Wi-Fi, setup hotspots, LAN discovery and the device-hosted
website. It keeps USB and Bluetooth LE, all four pages, page/card management,
postcards, the user's sound banks, volume, brightness and startup settings.
The main-task stack fix is retained. No saved user partition is erased.

1. Extract the complete Windows package and run `start-auto.cmd`. Keep it running
   for local Codex and TrafficMonitor data. Auto mode prefers USB, then BLE;
   `start-bluetooth.cmd` selects BLE only. Do not restart an existing TrafficMonitor
   installation for this update; its bridge DLL is unchanged.
2. A successful connection opens the computer's loopback management website.
   It shares the one physical USB/BLE connection with synchronization. Do not
   connect another website directly to the same port while the program owns it.
   Direct browser USB/BLE management remains available when the program is stopped.
3. Single OK shows details/contact; double OK cycles the enabled pages. Up/Down
   keeps its normal page behavior; hold either to adjust volume. OK closes the
   volume overlay without turning pages. Long OK no longer starts a Wi-Fi panel.
4. Read/save settings, page/card order, images and startup logos from the website.
   The contact QR remains a normal user-provided image; its default public website
   link does not pair a phone or provide local computer data.

The program binds its manager only to `127.0.0.1`. This local HTTP proxy is not
Wi-Fi support on the device. It retains authentication, host/origin checks,
manual disconnect and reconnect, automatic page opening and duplicate-instance
reuse. `--no-browser` suppresses automatic opening.

Device pages change immediately; only the volume overlay slides in and out at the right edge. The management website uses brief sidebar and dialog transitions and respects reduced-motion preferences. Success notices last about 2.2 seconds and errors about 4.2 seconds; swipe, the close button or Escape dismisses them, while hovering or keyboard focus pauses the timer. Content, device settings, user-selected boot logos/GIFs and sounds are preserved. Battery-life differences have not been measured.

Opening the website automatically checks the latest stable GitHub release. Connecting a device reads its firmware version and compares it with that release; the top-bar version indicator opens About, where installation remains an explicit action. A device newer than the public release is identified without offering a downgrade. Failed checks remain marked unavailable and can be retried manually. A release without a valid installation manifest can still be identified, but cannot be installed through the website.

The experience simulator uses the user-provided transparent front photograph, with the interactive canvas aligned to its screen and four controls at the physical button positions. Screen content remains explicitly labeled as simulation data.

The dedicated Theme page controls device colors: Default (original blue-gray), Obsidian, Porcelain and green ASCII. Collapsible advanced controls customize accent, chart, background, text and secondary text, with individual/full resets. The real-photo preview reuses the simulator with power/up/down/OK, single/double clicks and holds; all readings are labeled examples. Connect v21, read the device palette, then Save to device. Completion and readback are checked before acknowledging success; colors persist across reboots. Older firmware remains preview-only with an upgrade hint. Unsaved palettes participate in leave protection. In v22, postcard backgrounds and text follow the palette without rewriting stored pixels; photographs, QR codes, boot images and semantic status lamps retain their colors. Page thumbnails and postcard previews use the selected device palette.

The management website has separate WinUI-style light/dark modes at the bottom of its sidebar, remembered per browser origin. Device palettes never recolor the website; website mode never writes device configuration.


v22 removes the management-page fade/slide and publishes completed simulator frames in one canvas copy, keeping the right-side volume overlay. Device navigation has no tween animation. The single DMA strip grows from 4 to 20 lines (+7680 bytes), reducing full-screen submissions from 80 to 16 to reduce progressive-refresh perception; appearance and runtime headroom still need board verification. This does not claim scan-free LCD transfers. Favicon, sidebar and About use the supplied black/white TK images according to website light/dark mode; the boot logo is unchanged.

Screen diagnostics show accurate black/red/green/blue/white swatches, color bars and a 16-step grayscale preview. v22 displays matching patterns on the device, exiting on any key or after about 15 seconds. Older firmware retains solid-color tests and requests an upgrade for the new patterns. Diagnostic colors are independent of all themes.
