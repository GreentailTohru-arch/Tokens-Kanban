[简体中文](README.zh_CN.md)

# TrafficMonitor bridge for Tokens Kanban (component v15)

The x64 bridge reuses **TrafficMonitor 1.86** data without polling sensors or
network adapters. It adds no display item, edits no TrafficMonitor configuration,
opens no network port, and reads no other process memory. No additional hardware
monitor or driver is installed. Later TrafficMonitor versions are deliberately
unsupported until their independent sensor-plugin interfaces are verified.

## Install and use

1. Keep the existing TrafficMonitor 1.86 x64 installation and its selected network
   adapter. Copy `TokensKanbanBridge.dll` into its `plugins` subdirectory.
2. Exit and reopen TrafficMonitor once to load the DLL. Ask the owner first when
   restarting a running copy. Preserve its existing elevation level and settings.
3. Run the matching `CodexTokenDisplay.exe` (v15 or later); USB is preferred automatically, or use
   `--transport ble`. Only one sync process may listen to the bridge at a time.
4. On the Passport, double OK through the enabled pages to System Monitor.
   Up/Down and single OK keep their existing behavior on the other pages. The
   browser's first sidebar entry manages all four pages with previews, switches,
   and move-up/down controls; save applies the order and enforces at least one page.

`CodexTokenDisplay.exe --system-only` prints one diagnostic sample without
connecting to the device or reading Codex logs. It includes the eleven raw metrics,
TrafficMonitor's formatted taskbar strings and a eleven-bit callback/getter
agreement mask (`2047` means all agree within the host's whole-degree precision).
It cannot run concurrently with another bridge receiver. The normal sync process
handles token/quota data independently if TrafficMonitor is absent.

Unknown metrics use `-1`, displayed as `--`; zero load and zero traffic remain
valid. Temperatures at or below zero are unavailable under the V1.86 host's
temperature convention. A dead source process or a sample older than five seconds
invalidates all metrics. The device reflects that on its next five-second sync;
if the computer stops syncing, the device expires its system sample after fifteen
seconds. Resuming the receiver or TrafficMonitor recovers without device reset.
Network byte rates come from the host's selected adapter and use its 1024-based
unit convention; compact display rounds to one decimal. No Wi-Fi setup is needed.

## Verified interface and build

The vendored, unmodified `vendor/PluginInterface.h` is API **7** from official
[V1.86](https://github.com/zhongyang219/TrafficMonitor/tree/02a817a069bac6bf4d263b5209d9c1b07fe2f950),
under the adjacent original license. The host calls `OnMonitorInfo` after collecting
data and supplies its built-in temperatures and adapter byte rates; the bridge
also checks `GetMonitorValue` and records `GetMonitorValueString` from that same
callback. Relevant sources are
[TrafficMonitorDlg.cpp](https://github.com/zhongyang219/TrafficMonitor/blob/V1.86/TrafficMonitor/TrafficMonitorDlg.cpp#L1513)
and [TrafficMonitor.cpp](https://github.com/zhongyang219/TrafficMonitor/blob/V1.86/TrafficMonitor/TrafficMonitor.cpp#L1401).
An installed V1.86 may ship LibreHardwareMonitor internally; the bridge only
consumes the host's public plugin interface and never accesses that library itself.

The local Windows mailslot `\\.\mailslot\TokensKanban-TrafficMonitor-v1` carries a
bounded 1,180-byte message per host update. The reader checks version, length,
timestamp, bounds, process image identity and liveness; no `PROCESS_VM_READ` is
requested. Duplicate/old messages cannot replace a newer sample. The DLL owns no
thread, retained handle, heap allocation or CRT dependency. Windows x64 MSVC ABI
is required; a MinGW C++ DLL is not a compatible substitute.

Build with an installed x64 MSVC compiler (C++20):

```powershell
./build.ps1 -OutputDirectory ../../build/trafficmonitor
```

The build uses documented minimal Win32 declarations and a generated Kernel32
import library so a Windows SDK/CRT installation is not required. ASLR and DEP are
enabled. The isolated real-DLL test is
`python tests/trafficmonitor_bridge_probe.py <DLL>` from the firmware repository.
It uses a mock host, not the user's running TrafficMonitor; real source agreement
and physical display acceptance must be reported separately.

## v15 configurable cards and verified data scope

The sidebar is Page Management, System Monitor, Postcard, Settings. Select up to
six cards, drag or use buttons to reorder, and toggle or remove cards independently.
All-off shows an empty state. Page switches/order are stored separately and at
least one page remains enabled. Reconnect reads actual saved device configuration.

CPU frequency uses the public GHz getter converted to MHz, rather than the
callback's truncated integer GHz. Disk utilization follows the host's selected disk.
RAM exposes only used percent, never inferred capacity. V1.86 temperatures are
built in; separate sensor plugins are not assumed to forward readings.
Today's upload/download are restored daily history plus traffic collected on the
host-selected adapters during that day. Changing adapters does not rewrite earlier
history. The host resets at its local day. This is today's traffic, not lifetime
traffic or strictly the current adapter's daily total. Combined values require both
inputs. Byte/bit speed uses x8 and 1024-based units; zero remains a valid reading.

Titles accept six characters and direction labels two. The shared glyph inventory
covers GB2312, ASCII and arrows; unsupported text explicitly blocks saving. Choose
auto/fixed units, zero to two decimals, optional suffixes/percent/spaces and compact
display. Narrow cards may reduce precision and tighten text to avoid clipping.
Previews are illustrative, never real-time readings. v15 accepts old 764-byte bridge
packets with new fields unavailable. SM2 carries eleven metrics; negotiated SM1
sends the original seven to older monitor-capable firmware.

The actual DLL was loaded into local TrafficMonitor 1.86 x64 with restart permission.
Eleven readings matched host getters and formatted strings. Receiver reconnect,
real cached-data expiry and a retired source PID were checked. Missing sensors and
unsupported versions used an isolated actual-DLL mock host. v15 has not been flashed
to the Passport; host tests do not imply physical device acceptance.

The v16 release retains this verified v15 bridge DLL byte for byte; only the
System Monitor clock, web preview and companion package version changed.
