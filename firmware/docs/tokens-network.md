<p align="right"><a href="tokens-network.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Tokens Kanban v19 connections

## Start on a computer

1. Extract the entire Windows package and run `start-auto.cmd`. Keep this program running for Codex and TrafficMonitor data. It prefers USB, then a previously paired Wi-Fi device, then BLE. A phone management page does not collect computer data.
2. On a successful connection the program opens its local management page. Management and synchronization share the same physical connection, including the single BLE connection. Do not additionally connect the public website directly to the port already in use. Repeated reconnects open at most one browser tab per minute. `--no-browser` disables automatic opening.
3. Open Settings → Wi-Fi. Through USB, enter the **2.4 GHz** network name/password and select Save and connect. An open personal network uses an empty password; enterprise authentication is not implemented. Credentials stay on the device and are never uploaded to GitHub. Saved networks reconnect after boot and after temporary failures.
4. Keep USB or BLE connected for at least one more synchronization cycle after Wi-Fi connects. The computer records the device's pairing token and address in `%LOCALAPPDATA%/TokensKanban/network-devices.json`, outside the repository. After unplugging USB, automatic discovery finds that paired device on the same LAN. `start-wifi.cmd` selects Wi-Fi only; `--wifi-host IPv4` selects a specific paired device. If the address changed, discovery updates it. An unpaired device requires one USB/BLE connection first. Guest-network isolation/firewall restrictions may block discovery or HTTP.

## Phone setup and management

Long-press the device's **OK** key to show its management QR. Up/Down switches to setup, starting a password-protected hotspot for ten minutes. Scan the Wi-Fi QR to join it. The phone may show a captive-portal window; otherwise open `http://192.168.4.1/` in its browser. Enter the home network under Settings → Wi-Fi. A network switch can interrupt the current page: join the same home Wi-Fi and scan the device's management QR again. Short OK closes the device overlay without changing pages.

Once connected to a LAN, the management QR points to the device's own website and includes its private management token. Do not publish this QR. Over an ordinary BLE connection, network passwords are deliberately rejected: start the hotspot, or use USB to provision. No additional phone app, cloud relay, or LibreHardwareMonitor is required for Wi-Fi management. Phone and computer browser limitations may require keeping the full browser open instead of the small captive window.

The passive NTAG213 supports an NDEF website record. A phone with NFC writing support can write the public management URL `https://greentailtohru-arch.github.io/Tokens-Kanban/` once. Tapping opens a website prompt; it does not automatically connect Wi-Fi/BLE. The MCU has no tag-writing API, so firmware cannot update that tag's address. A changing private LAN address continues to use the screen QR.

## Existing controls and data

Double OK still cycles enabled pages in their saved order. Short Up/Down keeps its normal period navigation; holding either key adjusts volume. The QR overlay is temporary and does not change page settings. Existing settings, postcard, custom boot logo and supplied audio use the same storage partitions. New Wi-Fi data uses an independent NVS namespace; no full-chip erase is needed.

The website simulator's left power key now toggles screen-off/wake on a short click, switches off on a two-second hold and starts on a half-second hold. A wake click consumes its action. The physical independent power switch is controlled by hardware; its short-click event is not exposed by the current BSP, so this update does not claim that new physical behavior. The three existing function keys still wake an automatically blanked real screen.

The device serves the management site locally over HTTP. GIF logo import includes a bundled decoder and works without `ImageDecoder`; it retains crop/size/time limits. Firmware installation still requires desktop Chrome/Edge, Web Serial, USB and the HTTPS public or loopback management page. The phone/device HTTP page can manage settings and download firmware but cannot bypass those browser requirements.

## Compatibility and verification

The new computer proxy also supports v18 devices: it reports Wi-Fi unsupported rather than disconnecting token/system synchronization. The direct public website's USB connection preserves RTS-before-DTR handling. Management never starts a second physical connection when attached through the computer proxy.

Host tests cover bounded credentials/commands, retry delays, captive DNS malformed packets, pairing cache, authenticated UDP discovery, HTTP authentication, browser reconnect, QR decoding, preserved settings and mobile layout. A successful host build is not Wi-Fi hardware validation. LAN reconnect, boot reconnect, hotspot pop-up behavior, radio coexistence, sustained memory use and physical NFC writing remain device checks until executed with explicit flashing permission.
