<p align="right"><a href="bluetooth-device-name.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Linked device and Bluetooth names (v23)

Saving the device name in the management website also changes the GAP Device
Name and advertised Bluetooth name in v23. The existing persisted `name1`
setting is reused, including names saved on older firmware. The default remains
`You's AI Passport`. A failed NVS save keeps both previous names.

The refresh runs on the NimBLE host event queue after the save succeeds. It
updates scan-response data without disconnecting the current peer, restarting
the device, or changing the service UUID/address. Both the browser and desktop
sender discover the service UUID, so their selection does not depend on the name.
The name is reloaded on startup, stack resynchronization and advertising restart.
If the controller rejects an immediate refresh, it is retried at the next
advertising restart; the saved setting remains authoritative.

The full name limit remains 32 UTF-8 bytes. Legacy scan responses can carry
29 name bytes; longer names are shortened on a complete UTF-8 character boundary
and marked as a shortened local name. The full name remains stored and available
through the GAP characteristic and `PC_INFO_NAME`. Some clients cache names;
disconnect and scan again if their list still shows an old one.

`PC_NAME_BLE` replies `PC_NAME_BLE 1 29` on supported firmware. The website checks
this capability and shows an upgrade hint for older firmware instead of claiming
that its Bluetooth name changed. No change to the existing save protocol is needed.

Host tests cover UTF-8 boundaries, failed commits, repeated requests, persisted
names, and website behavior with new and old firmware. Firmware builds validate
the NimBLE integration. Real radio scans, cached client names, uninterrupted BLE
sync while renaming, and reboot persistence still require device testing.

This change does not authorize flashing or publication. Keep the existing
settings and postcard partition when installing a validated application update.
