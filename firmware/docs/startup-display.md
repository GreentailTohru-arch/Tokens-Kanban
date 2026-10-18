<p align="right"><a href="startup-display.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Startup display repair (v24)

## Full-screen framing (v25)

The web editor uses the entire 240×320 display as its 3:4 viewport. Images fit
inside it initially; 10–800% scaling and dragging allow black margins or cropped
edges. All GIF frames use the same placement. The original default logo remains
available. Save the logo separately after confirming the framing.

TLG2 stores bounded 3:4 frames (up to 120×160); the display decoder expands one
row at a time to 240×320 without a full-screen transform buffer. Complex GIFs
may be reduced in resolution/frame count to fit the existing storage budget.
Legacy TLG1 square logos retain their original centered size. `PC_LOGO_CAPS`
advertises TLG2 support; the editor allows preview but blocks saving to older
firmware. Existing settings and image partitions are unchanged.

Host/browser checks include portrait parsing, old-format compatibility, fit,
shrinking, zooming, position retention, GIF decoding and firmware parsing of the
actual browser output. Real LVGL rendering covers full-screen edges and repeated
UI updates in the 48 KB graphics pool. Device acceptance remains pending.

The backlight stays off while persisted settings and the boot image are loaded.
The selected first frame is rendered and queued SPI transfers are drained before
the saved brightness is applied. A valid custom image replaces the built-in logo
before the screen becomes visible; missing or invalid custom data falls back to
the original logo. Animated images play once and hold their final frame while
startup audio finishes. Stored images, sounds and settings are not rewritten.

The dedicated left power button controls hardware power. The documented BSP
exposes only the three right-side ADC buttons; there is no documented firmware
event for a short press of the power button. Software cannot promise short-press
screen sleep on this hardware. Automatic screen timeout remains available.

Host regression tests cover frame boundaries and holding the last frame beyond
the animation duration. Cold boot, restart, custom static/GIF replacement and
visible flicker still require device testing. Firmware installation and public
release require separate authorization.
