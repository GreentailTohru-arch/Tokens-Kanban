<p align="right"><a href="pet-controls.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Virtual pet controls

Open Device management → Virtual pet from the Windows companion, edit bindings, then use Save pet buttons or the top save action. Controls travel over the existing USB/BLE sync connection and require the companion to run; the device is not a Bluetooth keyboard.

| Key | Single | Double | Triple | Hold |
| --- | --- | --- | --- | --- |
| Up | Send | Undo | Stop / continue | Volume up |
| Down | None | Custom shortcut (inactive until configured) | None | Volume down |
| OK | None | Next enabled page | Previous enabled page | Dictation |

Starting with v32, OK double/triple press cycles forward/backward through enabled pages on every page and cannot be customized. The pet offers seven customizable click gestures and a dictation toggle. Volume holds remain reserved. Single and double presses wait 350 ms after release to distinguish triple presses. The web simulator never controls the PC. Single OK on other pages retains its existing function.

## Custom shortcuts and saving

Choose Custom shortcut on a gesture, click keys in the full keyboard dialog and confirm. Click selected keys again to deselect; combinations allow up to four keys with at most one ordinary key. Narrow screens scroll the keyboard horizontally. Adjust an existing combination through Adjust shortcut in the same dropdown. There is no physical-key capture field or separate Save shortcut on this PC button.

Save pet buttons or the top save action saves modified computer shortcuts first, then device mappings. A computer-save failure prevents device writes; a device-save failure reports the saved computer portion and allows retry. Each gesture is independent. Clearing one disables only that gesture, preserving other gestures and dictation. The public site can preview selections but cannot write computer preferences. Shortcuts target the foreground app; synthetic-input support requires actual testing.

Run the companion and focus the intended Codex composer for send, undo, stop and continue. Continue sends the Chinese word for continue only into an empty composer, preserving drafts. The unreliable left/right Alt Appshot action has been replaced by Custom shortcut.

## Dictation

Match Codex's hold-to-dictate shortcut to `dictation_hotkey` in `%LOCALAPPDATA%/TokensKanban/pet-controls.json`, for example Alt+H. Without a preference the companion defaults to Ctrl+Alt+F9; upgrades preserve it. Allow the computer microphone, hold OK on the pet page, and release to keep text without sending. The animation indicates a held shortcut, not measured microphone activity. Switching apps, 1.5 seconds of link silence, exit or a 20-second hold releases keys.

The MacOS package provides data synchronization; desktop remote control is Windows-only.

## Protocol and compatibility

`PC_KEYS_GET` returns `PC_KEYS 2` with nine IDs, retaining the storage layout. Index 7, formerly OK triple, is reserved as zero and is neither editable nor dispatched. Other bindings are preserved. Custom actions 16–22 identify the seven click slots. Legacy action 4 and computer preferences remain compatible. `PC_KEYS_SET id ...` persists through the `PC_CFG_STATUS` worker.

`PC_CTRL_POLL session ack` returns `PC_CTRL 1 sequence action held`. New connections discard pending actions, which expire after 1.5 seconds and are deduplicated before dispatch. Offline input is never replayed. Host checks cover gesture timing, page order, save failures, deduplication and dictation release. This revision is not automatically flashed; physical button acceptance is performed by the user.
