<p align="right"><a href="pet-controls.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Pet page controls

The v26 pet-control firmware and matching Windows companion add controls to the
Codex pet page. Other pages retain their original behavior. This is a companion
feature over the existing USB/BLE connection, not a Bluetooth keyboard.

| Key | Single | Double | Triple | Hold |
| --- | --- | --- | --- | --- |
| Up | Send message | Undo input | Stop / continue | Volume up |
| Down | None | Codex Appshot | None | Volume down |
| OK | None | Next enabled page | None | Dictation |

Open device management → Settings → Pet buttons to change the nine configurable
bindings, restore defaults, and save to the device. Double OK and volume holds
remain reserved. Single/double taps wait 350 ms after release to distinguish
triple taps. Preview controls only simulate actions; they never control the PC.

Run the matching Windows sync companion. Keep the intended Codex composer in
the foreground for send, undo, stop and continue. A second stop/continue action
after a successful stop inserts and sends the Chinese word for “continue” only when the input is empty;
it does not overwrite a draft. Send/undo/stop use Windows accessibility controls
and fail without executing when those controls cannot be identified. The
current Codex installation did not expose its composer during read-only
inspection; these three actions require desktop acceptance before release.
For compatibility testing, save your work and fully exit Codex, then choose
launcher option 5. It opens the installed Windows Codex with Chromium renderer
accessibility enabled. It never closes an existing session; support must still
be verified after reopening. This is not an automatic startup preference.

In Codex Settings → Keyboard Shortcuts, set the **hold dictation shortcut** to
Ctrl+Alt+F9 and allow microphone access. Hold OK to use the computer microphone;
release to finish dictation without sending. The pet shows the listening
animation while the companion holds the shortcut. This indicates the shortcut
state, not a measurement of actual microphone capture. Codex permissions and
transcription availability still apply. Focus loss, link silence for 1.5 seconds,
exit, or the 20-second hold limit releases all injected keys.

Appshot presses left and right Alt simultaneously. Codex captures the foreground
window using its own permissions and configured destination; no extra Enter is
sent. Set the Appshot destination to the desired chat in Codex.

The macOS companion continues data synchronization; desktop remote actions in
this revision are Windows-only. No Wi-Fi, audio uploads, API keys, or additional
firmware animation buffers are introduced.

## Protocol and acceptance

`PC_KEYS_GET` returns `PC_KEYS 1` followed by nine action IDs. `PC_KEYS_SET id ...`
uses the existing `PC_CFG_STATUS` worker/persistence protocol. Actions 0–4 are
none/send/undo/stop-continue/custom shortcut; the hold slot accepts 0 or 5 (dictation).

`PC_CTRL_HELLO` negotiates support. `PC_CTRL_POLL session ack` returns
`PC_CTRL 1 sequence action held`. The companion polls at 100 ms between ordinary
five-second snapshots. New sessions/transports drop pending actions. Pending
actions expire after 1.5 seconds and are deduplicated before dispatch, including
failed dispatch. Offline input is not stored for later replay.

Host checks cover gestures, reserved controls, persistence failures, stale
sessions, deduplication, voice release, and browser preview. Physical button
timing, the actual Codex composer, Appshot capture, microphone permission and
transcription remain device/desktop acceptance items. No device flash or public
release is implied by a successful build.

## Windows remote compatibility fix

Continue locates the current visible Codex composer after Stop, preserves existing drafts, and waits for Send to update after inserting the continuation text. Custom shortcuts and dictation use physical scan-code events with left/right key identity. Moving into a Codex dictation popup no longer immediately releases the held keys; switching to another app, disconnecting or timing out still releases them. If Ctrl+Alt+F9 also fails manually, first configure and verify hold-to-dictate and microphone permission in Codex. This companion-only fix does not require reflashing; actual capture and transcription still require user acceptance.

Set `dictation_hotkey` in `%LOCALAPPDATA%/TokensKanban/pet-controls.json` (for example `Alt+H`) to match Codex hold-to-dictate. Without a local preference it defaults to Ctrl+Alt+F9; package upgrades retain this preference.

## Custom keyboard shortcut

The unreliable Appshot action is replaced by Custom shortcut. Open the management page from the Windows sync program, select a key (F8, Space) or a combination (Ctrl+Shift+A) in the keyboard dialog, and save the local shortcut. Assign Custom shortcut to a single/double/triple device press and save the device bindings. Each gesture has its own shortcut. Blank disables it; the foreground application receives it. The setting survives package upgrades and preserves the dictation binding. This does not guarantee that applications accept synthetic input.


## Visual shortcut picker

Select Custom shortcut to open a full desktop-keyboard dialog. Click a key or modifier combination, click again to deselect, and confirm to display the chosen shortcut. The dialog has no physical-key capture field. On narrow screens, scroll the full keyboard horizontally. Save the binding on this PC and save the device gesture mapping separately. Each custom gesture has its own binding (up to four keys, with at most one ordinary key); clear and save to disable it.

## Independent gesture shortcuts (v31)

Each of the eight single/double/triple gesture slots has its own custom binding; OK double press remains page navigation and OK hold remains dictation. Choose Custom shortcut on a gesture, select keys in the dialog, confirm, save shortcuts on this PC, and save device bindings. Existing shared shortcuts are copied to all eight local slots on migration; changing or clearing one does not affect the others or dictation. This requires v31 firmware and the updated Windows companion. The public site can preview selections but cannot save computer preferences.

The device reports `PC_KEYS 2`; custom actions 16–23 identify the eight slots. The poll reply retains the `PC_CTRL 1` frame format, extending the action range. Old ordinary actions and the legacy shared action 4 remain supported.

For an existing custom binding, choose Adjust shortcut in the same gesture dropdown to reopen the keyboard; there is no separate edit button.
