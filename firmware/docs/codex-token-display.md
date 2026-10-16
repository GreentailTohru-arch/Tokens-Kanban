[简体中文](codex-token-display.zh_CN.md)

# Codex Token Display

An independent 240 × 320 dashboard for AI Passport. The dark slate background,
mint tabs and cyan quota bars follow the supplied visual reference. The firmware
uses the existing display, button and battery BSP; it has no demo-menu route,
Wi-Fi credentials, cloud key, or automatic flashing operation. Optional notification tones use the audio BSP.

## Controls and data

- Up / Down: previous / next period (today, this week, all retained history).
- Single OK on Home: toggle quota + exact tokens versus input/cache/output details + estimated USD.
  The TOTAL TOKENS heading and large count stay visible. Press again to return.
- Double OK: cycle **three** pages, Home → Analytics → Postcard → Home. This does not also
  trigger the single-click action. Home remembers its overview/detail mode.
- The bottom has three thin page indicators; the active page is highlighted. No action-help text.
- Up/Down changes the selected period on Home/Analytics; the model ranking follows it.
  Single OK on Analytics does nothing; the single-click display modes belong to Home.
- The top-right value is the hardware battery reading; unavailable is `--%`.
  Postcard shows time at the upper left, date/weekday at the upper right, imported content and page indicators.
- Single OK on Postcard toggles the picture/title/subtitle and contact QR/name.
  It remembers its side; Up/Down does nothing there.
- USB or Bluetooth LE updates every five seconds. After 15 seconds without a valid snapshot the
  retained numbers are explicitly marked disconnected/stale. Quota values expire
  after three minutes without a successful account fetch.

Token totals come from this computer's readable Codex `sessions` and
`archived_sessions` JSONL files. Input already includes cached input; output
already includes reasoning output. Total = input + output, never input + cache
+ output. Day/week use the computer's local calendar; week begins Monday.
Repeated counter events and copied fork-history events are deduplicated. A first
record with inherited/truncated cumulative history contributes only its reported
last request. Removed or unavailable logs reduce the retained-history count. This
is not an account billing report, nor a guaranteed all-device lifetime counter.

Account quota is fetched every 60 seconds using the installed Codex executable's
read-only `account/rateLimits/read` RPC. The bridge prefers the `codex` multi-bucket
entry and uses each window's actual duration, rather than assuming the primary
window is five hours. A missing second window is shown as unavailable. The reset
voucher is informational only; no button redeems it. Dollar amounts are Standard API equivalents, not subscription charges or invoices. The bridge never reads
or exports authentication files, sends prompts, or prints raw profile responses.
Only counters, quota percentages/durations, plan category, voucher count an activity enum, and estimate cents/coverage flags cross USB/BLE. No chat contents or account credentials are transmitted.

Protocol and API source: [official Codex App Server documentation](https://learn.chatgpt.com/docs/app-server).

## Estimated API value

The fixed price snapshot is **2026-10-16 Standard**, USD per million tokens:

| Model (official source) | Ordinary input | Cache read | Cache write | Output |
| --- | ---: | ---: | ---: | ---: |
| [gpt-6.1-sol](https://developers.openai.com/api/docs/models/gpt-6.1-sol) | 2 | 0.10 | 2.50 | 10 |
| [gpt-6-sol](https://developers.openai.com/api/docs/models/gpt-6-sol) | 2 | 0.20 | 2.50 | 10 |
| [gpt-6-astra](https://developers.openai.com/api/docs/models/gpt-6-astra) | 10 | 1 | 12.50 | 50 |
| [gpt-5.6-sol](https://developers.openai.com/api/docs/models/gpt-5.6-sol) | 4 | 0.40 | 5 | 20 |
| [gpt-5.6-luna](https://developers.openai.com/api/docs/models/gpt-5.6-luna) | 0.20 | 0.02 | 0.25 | 1.20 |

For a request above 272,000 input tokens, input/read/write rates double and output
is multiplied by 1.5 for the entire request. Cost is `(input - cache_read - cache_write)
* input_rate + cache_read * read_rate + cache_write * write_rate + output * output_rate`,
divided by one million. Reasoning output is already included in output. Integer nano-USD
is summed before rounding half up to cents, once per selected calendar period.

The bridge uses the request model from `turn_context` and `last_token_usage` only
when it matches the counted delta. Unknown models, absent cache-write counters,
invalid subsets or ambiguous multi-request deltas remain unpriced. No price alias
is inferred. Duplicated fork history is counted once; a copy with complete metadata
may supply the estimate for that same event. The Chinese partial-estimate label means some counted usage is
unpriced; the Chinese unavailable-estimate label means no priced usage is available. An empty measured
period is $0.00. Values from $1 million use an M suffix to fit the display.

This values retained usage at today's snapshot, not historical transaction prices.
The local metadata does not establish a billable service tier: Standard is the explicit
comparison basis. Fast/Ultrafast premiums, regional premiums, tool charges, taxes and
subscription fees are excluded. GPT-5.6 Sol's current rate is promotional (officially
available at least through November 21, 2026); future changes require updating
`companion/token_pricing.py` and rebuilding the companion. Runtime does not fetch prices
or make paid inference calls. New model IDs show unavailable/partial estimates.

## Status light

The top-left circle is red when either fresh quota window reports 100% used and
its reset deadline has not passed (or is unavailable); red overrides activity.
It is green for an active local Codex turn and blue for idle. Grey means disconnected
or unknown. Quota is the Codex account bucket, not separate ChatGPT website usage.
Percentages below 100 are floored, so 99.9% does not produce a false red light.

Activity uses local `task_started`, `task_complete` and `turn_aborted` lifecycle
events, matching turn IDs across copied histories. Any recent unfinished turn
makes the light green; completion/abort clears it. Archived histories cannot make
it green. An unfinished turn with no events for 15 minutes becomes unknown;
this covers abandoned logs and can also occur during a very long silent tool call.
The latest known completed lifecycle gives idle; absent/unreadable lifecycle data
gives unknown. This is an inference from local logs, not a server-wide activity API.
Separate app-server processes do not expose the desktop's loaded runtime state.
No threads are resumed or modified to obtain activity.

## Run the Windows companion

Use the delivered `CodexTokenDisplay.exe`, or Python 3.10+:

```text
python -m pip install -r companion/requirements.txt
python companion/codex_tokens.py
python companion/codex_tokens.py --dry-run
python companion/codex_tokens.py --list-ports
python companion/codex_tokens.py --transport usb --port COM7
python companion/codex_tokens.py --transport ble
python companion/codex_tokens.py --list-ble
python companion/codex_tokens.py --transport ble --ble-address AA:BB:CC:DD:EE:FF
```

Codex must already be installed and signed in for quota retrieval. Local tokens
still work when account/network retrieval fails. The executable discovers the
Codex binary on PATH or under the normal Windows Codex installation directory.
The companion stays in a console window; close it or press Ctrl+C to stop. It does
not register a startup task or service. When the device is disconnected, it retries.
Default `--transport auto` chooses USB when present, otherwise scans BLE. An
established connection stays on its transport until lost. Use `--transport ble`
to force wireless while charging by USB. Run only one companion for one device.
Automatic USB selection requires one Espressif VID:PID 303A:1001 device; multiple
candidates require `--port`. BLE selection requires one matching service, or an
explicit `--ble-address` from `--list-ble`. Multiple devices are never guessed.
Before sending statistics v4 requires a `CT_HELLO 4` / `CT_READY 4` handshake;
upgrade the firmware before using the v4 companion.
It does not toggle DTR/RTS intentionally and never invokes a flash command.

## Bluetooth service

The ESP32-C3 runs a single-connection NimBLE GATT peripheral named `Codex Passport`.
The Windows companion uses Bleak 3.0.2; Windows Bluetooth must be enabled. It
connects directly without requiring a classic Bluetooth serial/COM pairing.

- Service: `9e7a0001-4b51-4a4c-9b50-2f53f077c301`.
- RX write-with-response: `9e7a0002-4b51-4a4c-9b50-2f53f077c301`.
- TX read/notify: `9e7a0003-4b51-4a4c-9b50-2f53f077c301`.

The client subscribes before the handshake, sends 20-byte chunks (minimum ATT
MTU compatible), and waits for an application ACK for each snapshot. The board
clears partial frames on disconnect or a two-second inter-write gap, and resumes
advertising after disconnect. The client retries discovery after connection failure. On Windows it requests fresh
GATT services and explicitly disables notifications before disposing the connection,
then drains session events before closing the event loop to avoid cancelled re-subscriptions.
No OS pairing/bond or application authentication is used in this version; this is a
nearby-device statistics display, not a protected credential/data-transfer channel.
The service accepts numeric snapshots only and cannot execute commands or flash.

After a merged firmware refresh, a stale Windows pairing may prevent GATT service
discovery even while the device advertises. This was observed during the v3 upgrade:
restarting the board did not resolve it; removing only the old Codex Passport
pairing restored the direct BLE connection. If Windows lists this device as paired,
remove that specific old pairing and reconnect with the companion. Do not reset
other Bluetooth devices or erase Flash as a remedy.

References: [Bleak client API](https://bleak.readthedocs.io/en/latest/api/client.html),
[ESP-IDF NimBLE peripheral example](https://github.com/espressif/esp-idf/tree/v5.5.3/examples/bluetooth/nimble/bleprph).

## Analytics page

The second page follows the supplied model/activity/trend reference, adapted to the
240 x 320 display. It keeps the status light and battery header. The first page's
input/cache/output panel is a single-OK mode, not a third page.

The model section shows the two largest model groups for today/week/all history,
counting input + output and rounding each share to the nearest whole percent.
All groups remain in the denominator. Unrecognized or missing model metadata is
grouped as Other models. A missing second model is a dash, not a fabricated entry.

The activity grid has 26 week columns (Monday to Sunday, top to bottom), ending
in the current week. It counts dates with positive retained token usage and shows
four logarithmic intensity levels; future dates in the last column are blank.
Month labels describe the first, middle and last columns. The line is daily tokens
for the last 60 local calendar days, scaled to that window's peak (shown above it).
The three date labels describe the first, middle and final samples. Missing log dates
are empty, not interpolated; these are retained local records, not account-wide activity.
Period buttons change model ranking, not the fixed 26-week/60-day chart windows.
No statistics or UI reference numbers are invented at runtime.

## USB / BLE protocol

CT1 (19 integers), CT2 (20), and CT3 (26) remain supported. Their absent analytics
are unavailable, never shown as a measured zero. Legacy HELLO versions 1-3 still work.
The companion uses `CT4 <47 space-separated integers> <heat> <trend>*<checksum>\n`.
The checksum is eight lowercase hexadecimal FNV1a digits over everything before `*`.

The first 26 fields preserve CT3: sequence; day input/cache/output; week
input/cache/output; retained-history input/cache/output; first quota used-percent/
window-minutes/reset-seconds; second quota triple; voucher-count; plan-enum;
quota-age-seconds; activity; day cents/status; week cents/status; all cents/status.
Activity: 0 unknown, 1 idle, 2 active. Plan: 0 unknown, 1 Free, 2 Plus, 3 Pro,
4 Team/Business, 5 Enterprise. Estimate status: 0 unavailable (cents=-1),
1 complete, 2 partial (cents nonnegative). Unknown counts are -1.

Then: two model ID/count pairs per period (12 fields), active days, three heatmap
month numbers, three trend dates encoded MMDD, peak daily tokens, analytics-valid
(0/1). Model IDs: 0 other/unknown, 1 gpt-6-astra, 2 gpt-6.1-sol, 3 gpt-6-sol,
4 gpt-5.6-sol, 5 gpt-5.6-luna, 6 gpt-6-luna, 7 gpt-5.6-terra. An absent row uses 0/-1.
`heat` is 182 digits 0-4 (intensity), or 5 (future/blank); `trend` is 60 normalized
0-1000 values, each encoded as exactly three lowercase hex digits. It is a bounded
numeric protocol; model names and chat contents are never accepted as remote UI strings.

Valid snapshots receive `CT_ACK <sequence>`. Frames exceeding 1023 bytes are
discarded through newline. Checksums, field bounds, model sums, dates and graph
samples are checked before replacing a snapshot. Counts and charts arrive in one
atomic snapshot, avoiding mixed periods or sequences. BLE still uses 20-byte writes
with response and snapshot ACKs. USB has a 2048-byte RX buffer; its 1024-byte stream
buffer is static so it does not consume the USB worker stack.

## Build and verify

Activate ESP-IDF 5.5.3, then run `./tools/validate.sh`. The gate retains the merged
image at `build/FoloToy-AI-Passport-full.bin` and matching debug archive. Flash
the **merged** image at **0x0**, only after explicit user confirmation. Merged
flashing replaces the installed firmware and can reset saved NVS data. No full-chip
erase is part of this procedure. The original minimal 8 MB partition layout remains.

Application tests cover token aggregation, calendar boundaries, partial lines,
fork copies, quota mapping, large counters, frame validation, missing glyphs,
startup routing, countdowns, status priorities, lifecycle deduplication/staleness,
BLE fragmentation, rejected discovery candidates and cleanup. Existing BSP/demo regression tests
remain in the gate. On Windows/MSYS, fault-injection tests disable host unwind
tables so PE section collection can discard unused demo UI functions.

Fonts: `assets/fonts/NotoSansCJKsc-Regular.otf`, SIL OFL, generated 14px PLAIN 4bpp
subset with explicit widget binding. Generate/check using Pillow 11.3.0 and
fontTools 4.60.1: `python tools/token_font.py` / `python tools/token_font.py --check`.
The generator verifies each serialized pixel against rasterization and records
font hash and code points. Firmware enables glyph placeholders. A native LVGL
9.5.0 preview compiles the same `main/token_ui.c`, checks real glyph lookup (with
a deliberately missing negative control), and renders both pages, Home overview/details, full/partial/unavailable estimates and missing analytics:

```text
cmake -S tests/ui_preview -B build/ui-preview -DLVGL_SOURCE_DIR=/path/to/lvgl-9.5.0
cmake --build build/ui-preview
```

Run the resulting `token_ui_preview` executable in a scratch directory. Its data
is a test fixture, not a device screenshot. Host rendering does not verify panel
orientation, physical button timing, USB enumeration or battery accuracy.

## Device acceptance after approved flashing

Check boot without reset loops, clear Chinese glyphs and rounded-corner visibility;
all three periods, both pages and Home single/double-click behavior; mixed-digit widths; real USB and BLE handshake/ACK; red/green/blue/grey state transitions;
changing live counters; disconnect for 15 seconds and reconnect; unavailable
network/quota; physical battery display; repeated fast button presses; sustained
use and free heap with BLE enabled. The accepted v3 subset is recorded below; remaining physical scenarios still need testing. These checks require actual hardware and remain separate from
successful compilation and host tests.

## Previous v3 device acceptance (2026-10-16)

With explicit user approval, v3 merged image
`b6a4a13299cf002779fb7d0ad20825993d35f68691efc9713272f3465204c3da`
was flashed at 0x0 on COM9; write verification passed. A controlled restart produced
a clean startup with matching ELF prefix `e85072406`. USB CT3 handshake and estimate
snapshot ACK passed. After removing the stale Windows Passport pairing described
above, BLE CT3 handshake, 12 real estimate snapshots over 60 seconds and reconnect
after a 17-second software disconnect passed; no panic or unexpected reset appeared
in the observation window. The user confirmed the title and bidirectional token/USD
button toggle while the large total and quota panel stayed unchanged. The packaged
v3 companion ran in the background over BLE; v4 below supersedes it, with no startup registration.

Build: PASS. Host tests: PASS. Device tests: PASS for that subset.
Unverified: manual amount comparison for every period, physical USB unplug,
RF range, long-run stability, runtime heap and battery accuracy. Estimated API
value is not validated as subscription billing. No firmware changes or second
flash were needed for the Windows pairing recovery.

## Initial v4 handoff (before flashing)

v4 restores Home details, adds Analytics, Postcard and three page indicators. The BSP and
button driver are reused; the driver emits mutually exclusive single/double-click
events after its existing timing window. App navigation dispatches those events
separately. The v3 acceptance above does not certify v4. New device tests require
approval of the new merged image. Current build and host results accompany the
v4 delivery; physical double-click timing, model/chart display and CT4 radio
transport remain unverified until that approved flash.

## Browser postcard editor

Run `CodexTokenDisplay.exe --postcard-editor` (or the packaged `open-postcard-editor.cmd`).
The executable serves only four bundled static assets at a random localhost port
and opens the editor in the default browser. Use Windows Chrome/Edge for Web Bluetooth
or Web Serial. Browser device selection requires the user's button click. No website,
cloud account, CDN, or image upload server is used. Implementation references:
[Chrome Bluetooth](https://developer.chrome.com/docs/capabilities/bluetooth) and
[ESP-IDF partitions](https://docs.espressif.com/projects/esp-idf/en/v5.5.3/esp32c3/api-reference/storage/partition.html).

Stop the companion first to release its connection; choose the Passport through
“Search Bluetooth” or “Connect USB”. Select a photo, title, optional subtitle, QR
image and contact name. The browser preview shows the exact imported layout.
Long text is fitted or clipped to the preview area. Photo: adjustable square framing,
174 × 174, with centered title and subtitle. QR: adjustable square framing within 164 × 164 plus an outer white margin; initially the entire image is contained.
Both 240 × 264 faces are rasterized with the computer's Chinese font stack, converted
to RGB565, then losslessly run-length compressed. Custom Chinese text does not rely
on the firmware's fixed font subset. The photo is downsampled and color-quantized;
the contact image uses nearest-neighbor scaling. Scan the actual display after import
to validate the particular QR's density and contrast.

Save waits for a checksum-confirmed device commit. Do not close the browser during
transfer. Disconnect the editor and restart the companion afterward. Data is not
automatically recovered from the device into the editor; keep the original files.
An interrupted transfer retains the previous card. A new attempt may need to wait
60 seconds for the previous unfinished session to expire. Empty storage displays
only 00:00 and the page indicators, with no demo postcard or extra instructions.

The clock is synchronized from local computer time when connecting/saving and
every five seconds by the companion. It continues from the monotonic device clock
when disconnected; after power loss it shows 00:00 until another synchronization.
There is no RTC accuracy or cross-reboot wall-clock guarantee.

### Storage and transfer contract

The app's deliberate partition layout reduces factory to `0x770000` at `0x10000`
and reserves `postcard`, data subtype `0x40`, `0x780000..0x7FFFFF` (512 KiB).
NVS and PHY offsets are unchanged. Two 256 KiB slots each reserve the first 4096
bytes for a header and hold 253440 bytes of raw two-face RGB565 data. Raw images
are flash-mapped, not copied into scarce internal RAM. A worker erases/writes the
inactive slot and checks its FNV-1a hash before writing the validity marker last.
The image pointers switch under the BSP LVGL lock before the old slot is reusable.
Startup checks both slots and chooses the newest valid one. Neither the merged
image nor an ordinary app flash contains a preloaded card. A full-chip erase or
incompatible future partition layout destroys this data.

USB and BLE share newline-framed commands alongside CT1–CT4: `PC_HELLO` →
`PC_READY 1`, `PC_TIME seconds_since_local_midnight` → `PC_TIME_OK`,
`PC_BEGIN id fnv32`, `PC_DATA id raw_byte_offset hex_rle`, `PC_DONE id`, and
`PC_STATUS`. Upload commands reply `PC_STATE id state raw_bytes_received`.
State 1 is queued/busy, 2 receiving, 3 committed, 4 failed. The browser polls until
2/3, verifies every offset and only reports success after state 3. Wrong-order or
duplicate chunks do not advance the offset. Frames remain below 1024 bytes. Each
chunk expands to at most 256 bytes; tags 0..127 encode tag+1 literal RGB565 pixels,
128..255 encode (tag&127)+1 repeats of the next two bytes. BLE writes use 20-byte
fragments, compatible with the minimum ATT MTU. Bulk writes are done in the worker,
not in button or Bluetooth callbacks.

Additional host checks: `tests/test_postcard.c` fault-injects interrupted uploads,
CRC failures, erase/write failures, ordering, reboot recovery and clock rollover.
`tests/test_postcard_browser.py` uses headless Chromium with simulated USB/BLE,
imports files through the real editor, and round-trips every compressed block
through `tests/postcard_codec_probe.c`. These are not physical radio/QR tests.

## v4 device acceptance (2026-10-16)

With the user's explicit upload approval, merged image `29cb512c9b7aa68e20c9d623e581e517ddec323c07bde2ba8f1bf688c3011537` was written
at 0x0 to COM9 (ESP32-C3, MAC [device address omitted]); flash hash verification passed.
No full-chip erase or firmware rebuild was performed. Targeted application-header
readback matched embedded ELF `8782c66ff3...`; early boot ELF logs were not captured.
Captured application startup had no panic or storage-start failure. USB and BLE CT4
real statistics/time acknowledgements passed. Each transport uploaded and committed
all 990 blocks / 253440 raw postcard bytes with checksum confirmation:
USB 42.27 s, BLE 582.42 s for the test image.
BLE live updates for 30 seconds and reconnection after a 17-second software
disconnect passed. Targeted postcard-partition readback after reset matched both
valid slots byte for byte; the application and postcard worker restarted cleanly.
Real Edge Web Serial browser import: NOT RUN.
The user confirmed all three pages, single/double-click controls, Home details/USD,
postcard front/contact, Chinese text, image, time and phone QR recognition.
The sample QR points to example.com/contact; replace it with personal content
in the editor. v4 BLE background synchronization replaces the previous v3 process.

Build: PASS. Host tests: PASS. Device tests: PASS for the checks above.
Unverified: actual browser USB/Bluetooth chooser workflow, physical power removal,
radio range, long-run stability, runtime heap/stack high-water marks, battery
accuracy and other imported QR densities. Software resets are not power-loss tests.

## v5 postcard layout and framing (2026-10-16)

The centered avatar is a 174 x 174 square. The browser crop viewport supports dragging, arrow keys,
1–4x zoom and reset; replacing the photo resets framing. The crop is clamped within the source and
shared between preview and upload. Titles and both subtitle lines are centered; the QR side is unchanged.
HH:MM stays at the upper left, with a right-aligned Chinese month/day and weekday (for example October 16, Friday).
The date advances across midnight, months and years; the fixed font includes all weekday/date glyphs.
After power loss the clock awaits synchronization and the date is blank.

`PC_CLOCK seconds` accepts 2000–2099 local civil time encoded as UTC epoch seconds and replies
`PC_CLOCK_OK`. For browsers use `Date.UTC(year, monthIndex, day, hour, minute, second)/1000`;
the board does not perform another timezone conversion. The v5 companion sends this every five seconds.
Legacy `PC_TIME secondsAfterMidnight` remains supported and hides the date. The editor falls back to
time-only on v4 and explicitly reports that date display requires v5. The editor remains local;
run the v5 `open-postcard-editor.cmd` to open its loopback URL.

The v4 partition layout is retained and the merged 0x0 image does not include the final postcard
partition. Existing cards are raster images and are not automatically reformatted: reimport the source
photo and text with the new editor to apply the square/centered layout. User content is not overwritten
automatically. Final Build/Host results accompany the v5 delivery. Device tests: NOT RUN for v5;
earlier v4 user acceptance does not validate the new date, framing or layout on hardware.

## v5.1 crop dialogs (2026-10-16)

Both photo and QR selection open a compact modal crop window. Photo zoom is 1–4x; QR zoom is 1–8x,
with dragging, arrow keys and reset. QR starts fully contained with white padding for nonsquare images.
Confirm applies the draft and closes the dialog; close/Escape cancels without replacing the existing
bitmap or framing. Each image has an edit-framing button to reopen it. Preserve the entire QR pattern
and quiet zone and scan the device after saving. The photo, title and subtitle move down together
by 6 pixels while remaining centered; the date and page indicators stay in place.

This iteration changes only the web editor/image composition. Firmware protocol, UI and partitions
are unchanged from v5. Resaving the postcard applies the framing and spacing, including on v4; the
date display still requires the previously prepared v5 firmware and separate flash approval.
The Windows editor is packaged as v5.1; its loopback URL is printed on startup.
Host checks now include confirm/cancel/reopen, cancelling a replacement without losing the old bitmap,
QR poster framing followed by decoding the actual uploaded RGB565 face, and desktop/390px dialogs.
Simulated uploads are not hardware acceptance. No firmware or postcard was written to hardware in
this iteration; final validation and artifact identities accompany the v5.1 package.

## v6 hosted editor and default postcard (2026-10-16)

The editor is hosted at https://greentailtohru-arch.github.io/Tokens-Kanban/ with source in the user's
Tokens-Kanban repository. The top-left toolbar contains Bluetooth search, USB connection, disconnect
and save in that order. The right side shows connection status. Introductory copy and persistent
help are removed; action feedback uses a temporary toast and uploads show a thin progress bar.
The 174 x 174 avatar has 16px rounded corners, retaining modal framing and the 6px content offset.
Defaults use an original, horizontally symmetric generic user avatar, Chinese title/subtitle placeholders, a scan-QR contact
caption, and a QR linking to the hosted editor. All can be replaced. Images are processed locally in
the browser and sent directly to the authorized USB/BLE device; Pages only serves static files.

When no valid personal postcard is stored, firmware shows the same default faces from 253440 bytes
of read-only flash. No full-image RAM allocation or data-partition write is added. A valid saved
postcard takes priority on startup. Flashing requires separate approval; older firmware can also
receive the default faces from the website, while date display requires v5 or newer.
`tools/postcard_defaults.py --browser <Chromium-path>` generates the data using the actual browser
renderer; `--check` verifies identity. Generation uses Microsoft YaHei UI on Windows. The default QR
uses M error correction and a four-module quiet zone; physical scan acceptance remains separate.
Build/host/deployment results and exact firmware identities accompany v6. No unapproved flashing.

## v6 device acceptance (2026-10-16)

After the user's explicit flash approval, verified merged image `75c08d1a7d2332eeec46ba86d4857445d055ba0bf722d5aecf8aa31ed21731e2` was written
at 0x0 to COM9, ESP32-C3, MAC [device address omitted]. Write hash verification passed, with
no rebuild or full-chip erase. Targeted application-header readback matched ELF
`05e9c232e98b98978a0d1fa8663af09416e1410f11b2fd38f63334054f0ec385`; early startup ELF log lines were not captured.
USB CT4 handshake, real usage ACK, PC_CLOCK date/time ACK and postcard worker response passed.
The postcard partition initially matched the prior validated v4 readback byte for byte, with both
slots valid, confirming that firmware flashing retained the previously generated demo.

The user then reported the old third-page appearance. The retained raster demo explained this;
after verifying it was the known generated sample, the v6 default card was uploaded over USB:
990 blocks, 253440 bytes, checksum commit passed in 45.56 seconds.
The user confirmed the centered rounded avatar, title/subtitle and contact display, and that
phone QR scanning opened the published editor. No personal postcard was replaced; the inactive
slot retains the preceding generated demo.

Initial Windows BLE service discovery timed out. Removing only this Passport's old pairing
restored connection. BLE CT4, real statistics/date/time updates over 30 seconds and reconnection
after a 17-second software disconnect passed. A concurrent Windows USB observation handle was
interrupted, so this is not a claim of uninterrupted reset-free serial observation. Captured logs
showed no panic or storage error; the subsequent full USB upload passed. The packaged v6 companion
now runs BLE synchronization every five seconds in the background, without startup registration.

Build: PASS. Host tests: PASS. Device tests: PASS for the executed checks above.
Unverified: real browser USB/BLE permission and full upload, physical power loss, persistence
of the new card after reboot, long-run stability/clock drift, radio range, runtime heap/stack,
battery accuracy and uninterrupted serial observation. This card upload used desktop USB,
not the browser workflow.

## v7 device manager (2026-10-16)

The same hosted website now has a left sidebar with Postcard and Settings. Connect over
USB or BLE after stopping the desktop sync program. Select Settings to read the device's
current values, choose an idle timeout and separate completion/quota tones, adjust volume,
and click Save to device. Preview sends a short tone to the device without changing saved
settings. Disconnect the web manager before resuming the v7 desktop companion.

The default timeout is Never; choices run from 30 seconds to one hour. Only backlight turns
off: statistics, BLE/USB and audio continue. Any key wakes the display and consumes that
first key action. Saving settings or previewing a tone also wakes it. Incoming statistics do
not keep the screen awake. Settings live in NVS; commit failure retains the old values.
The default completion tone is Light, quota tone is Low, and volume is 35%. Each tone can
be disabled and volume zero mutes both. Saved settings survive normal reboot.

The v7 companion emits an anonymous completion sequence only for a fresh explicit
`task_complete` log event. Startup history, cancelled turns, expired events and duplicate
fork history do not sound; reconnect retries keep the same ID for firmware deduplication.
Completions in the same five-second poll are coalesced. Quota audio fires once on a fresh
transition to exhausted (or first observed exhaustion), and re-arms after known recovery.
Unknown or stale quota does not clear the latch or create a new alarm. Quota reset times
already at zero are not treated as exhaustion. No message text or credentials cross the link.

Firmware commands: PC_CFG_GET, PC_CFG_SET id idle done quota volume, PC_CFG_STATUS,
PC_SOUND tone volume, PC_EVT host sequence 1. Tone IDs: 0 Off, 1 Light, 2 Soft double,
3 Low alert; timeout 0 or 15..3600 seconds; volume 0..100. Saves and audio execute on a
bounded worker queue outside LVGL/button callbacks. A queued preview is not proof of
physical playback. Older firmware retains postcard support and shows Settings unavailable;
completion audio requires both v7 firmware and companion. The three device pages stay intact.

Host tests cover timers, wake consumption, stale quota, retry deduplication, numeric overflow,
queue pressure, NVS failure preservation, audio sample bounds, and simulated USB/BLE browser
save/readback. Build and hardware results are reported with the v7 delivery. No v7 firmware
is flashed without new user approval; physical sound quality, idle timing and concurrent
audio/radio/flash behavior remain hardware checks.
