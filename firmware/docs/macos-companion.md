<p align="right"><a href="macos-companion.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# macOS companion preview

This source-based preview reuses the v23 device protocol, local Codex usage
statistics, USB/BLE automatic selection, and shared local management website.
It is not a signed standalone app. No Mac runtime or hardware validation has
been performed; portable tests are not Mac acceptance tests.

## Start

1. Install Python 3.10+ from <https://www.python.org/downloads/macos/> if needed.
   Extract the entire package to a permanent, writable folder. Keep all contents.
2. Open `start.command`. The first run downloads pinned dependencies into the
   package's `app/.runtime` virtual environment and requires Internet access. Later
   runs reuse it. If execution permission was lost during extraction, open Terminal,
   type `bash `, drag `start.command` into the window, and press Enter.
3. Choose **1** (or Enter) to sync. Connect one Passport by USB, or grant Terminal
   Bluetooth permission when prompted. USB is preferred; unplugging retries BLE.
   Use the automatically opened local page to manage the shared connection.
4. Choose **2** for next-login background startup, **3** to cancel future startup,
   or **4** to stop the running sender. Normal foreground sync stops with Ctrl+C.
   Keep the folder in place; after moving it, set up the environment again and
   re-enable startup. Cancelling startup does not terminate the current sender.

Local usage is read from `CODEX_HOME` or `~/.codex`. Quota needs a logged-in Codex
CLI discoverable on PATH; when unavailable it displays `--`. The launcher adds
the conventional Homebrew binary directories to PATH without editing shell files.
No passwords or keys are bundled. Apple Silicon and Intel use the Python and
dependencies installed on that Mac; neither architecture is yet verified.

## Limits and login startup

TrafficMonitor and the existing bridge use Windows IPC. Mac system metrics,
including temperatures, remain unavailable (`--`); the preview does not invent
values or install a separate sensor collector. Existing firmware may label this
as “TrafficMonitor disconnected”. Tokens and management work independently.

Login startup writes only the current user's owned LaunchAgent at
`~/Library/LaunchAgents/io.github.greentailtohru-arch.tokens-kanban.sync.plist`.
It takes effect at the next login, does not open a browser, and has no KeepAlive
restart loop. It does not load, unload, or change unrelated jobs. Enable it only
after a successful foreground connection. Background Bluetooth permission may
differ from Terminal permission and still needs real-Mac testing; if refused,
use foreground sync and cancel login startup. Do not bypass macOS protections.
Startup errors are in `~/Library/Logs/TokensKanban`; regular background sync logs
and connection state are in `~/.local/share/TokensKanban`.

Before website firmware installation, stop sync to release the serial port.
Browser-managed USB/BLE and flashing depend on browser support and are not yet
verified on Mac. Do not run the sender on two computers targeting the same device.

## Validation

Portable tests cover LaunchAgent serialization, paths containing spaces/Unicode,
owned-job protection, idempotent enable/disable, Mac USB classification, and
unavailable Windows sensors. Real Mac installation, Bluetooth permissions,
USB/BLE handoff, login startup, and device rendering remain unverified.

References: [Apple LaunchAgents](https://developer.apple.com/library/archive/documentation/MacOSX/Conceptual/BPSystemStartup/Chapters/CreatingLaunchdJobs.html),
[Bleak macOS permissions](https://bleak.readthedocs.io/en/latest/backends/macos.html).
