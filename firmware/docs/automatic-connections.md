[简体中文](automatic-connections.zh_CN.md)

# Automatic desktop connections

Extract the complete desktop package and run `start.cmd`, then choose Start sync.
The same launcher can enable or disable current-user Windows login startup, or
stop background synchronization. Startup is opt-in: it creates one owned shortcut
in the user's Startup folder, needs no administrator rights, and runs silently
without opening a browser. Keep the extracted folder in place. After moving it,
enable startup again from the new location. Disabling startup does not stop the
current process; use Stop background sync for that. The old sender must be stopped
before replacing the program; old versions may require Ctrl+C or Task Manager.

One process sends both
Token and TrafficMonitor data. Auto mode prefers USB, uses BLE when USB is absent
or unavailable, and attempts to promote an existing BLE link when USB appears.
The USB handshake succeeds before the working BLE link is closed. An unavailable
USB port is retried after 30 seconds; unplugging it clears that retry delay.

Handoff is not instantaneous: the sender checks at its five-second sync interval,
and Bluetooth scanning, handshakes and OS retries can take longer. Management
writes and upload status requests defer promotion until five seconds of inactivity.
Read-only device telemetry does not postpone promotion. Do not unplug during a save.

Only one sender may run per user. An OS lock prevents racing startup processes and
is released on exit or process failure. Repeated launch reuses the management page;
it does not replace an existing process or change its mode. Stop the old sender
before upgrading. `start-auto.cmd` remains an equivalent legacy entry and
`start-bluetooth.cmd` remains BLE-only; stop and restart to change modes.

Automatic selection assumes one Passport. Multiple USB devices require `--port`;
multiple BLE devices require `--ble-address`. A pinned BLE connection is not
promoted to an unidentified USB device. This update does not change TrafficMonitor,
the firmware, device settings, image storage or sound assets. It does not enable
Windows startup automatically. Keep the sender stopped while flashing firmware.
