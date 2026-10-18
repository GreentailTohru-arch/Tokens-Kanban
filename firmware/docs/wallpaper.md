[简体中文](wallpaper.zh_CN.md)

# Full-screen wallpaper

The sixth device page fills the 240 × 320 screen. Its default is the current
device theme background, with the combined time/date switch off. Double-click OK to change pages;
the page participates in the existing ordering and enable switches. The postcard
time/date header remains on by default and has its own combined switch in the postcard editor.

In the management website, open Wallpaper, select an image, drag or use arrow
keys to frame it, adjust zoom, then apply and save. Images are resized to 240 ×
320 RGB565 (150 KiB); the transfer uses lossless run compression. Animated input
uses the first frame. Transparent areas are flattened onto the selected theme
background. Cancel preserves the previous draft. Restore theme background and
save to remove the stored image. Each page has one switch for both time and date. Legacy wallpaper flags with
only one component enabled now show both components. Time and date use the existing computer clock
sync; without a dated clock, the date stays empty.

The website reads display options on connection. Reading the stored image is
optional because it can take time over Bluetooth. Changing only the switches
copies the committed image on the device, without uploading or replacing it.

The optional bottom navigation indicator is off by default. Enable it in the wallpaper editor and save; it follows enabled pages and their order, without cropping the image. The new firmware reports wallpaper protocol version 2 (flag bit 3); the website still accepts version 1 and disables this option on older firmware. Page changes request an immediate LVGL refresh and wait for queued pixels, with no transition animation. SPI scanning may remain visible and requires physical acceptance.

## Storage and upgrade

This version adds a 512 KiB wallpaper partition at `0x6C0000` and shortens factory
to `0x6B0000` bytes. Boot logo (`0x740000`), postcard (`0x780000`) and NVS offsets
are unchanged. The first upgrade requires the matching partition table: an
app-only update on the previous layout cannot enable wallpaper storage. Use a
compatible segmented flash of bootloader, partition table and application to
preserve NVS; do not erase the whole chip. A merged image at zero includes gaps
that can overwrite NVS. The package retains the verified matching segments.

Two alternating slots publish only after complete length/hash verification.
Interrupted writes retain the previous slot. The renderer reads Flash one row
at a time, with no full-screen RAM image. `PC_WALL_GET/READ`, transactional
`BEGIN/DATA/DONE`, and `OPTIONS` are separate from postcard and boot protocols.
`PC_PAGES_GET3/SET3` supports six pages; old four/five-page configurations migrate
and their protocol views stay compatible.

## Validation

Host tests cover storage failures, reboot recovery, flags-only preservation,
page migration and navigation. Real LVGL host rendering covers blank/image
wallpapers, clock/date visibility and repeated pet/wallpaper transitions within
the 48 KiB pool. Browser tests use simulated USB only. Physical flashing,
power-cycle acceptance and real transfer timings are still required.
