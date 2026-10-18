[简体中文](codex-pet.zh_CN.md) · English

# Codex pet page (v26)

The fifth device page shows Codex status at the upper left, battery at the upper
right and the creator's six original transparent GIF animations in the center.
Double-click OK to reach it; enable or reorder it in the manager's Page Management.
The web simulator offers all six sample states. It does not write sample status
to the device. Device palettes change the background, not the pet artwork.

Use the v26 companion for enhanced status. A fresh statistics connection means
online; an active turn means thinking; a completed turn returns directly to online. Recent in-flight terminal or supported computer/browser
tool metadata shows computer use. Missing or expired quota data never means an
exhausted quota. A fresh exhausted quota overrides other connected states.
After 15 seconds without statistics the pet is disconnected.

The listening/typing animation is included and can be previewed, but current
Codex logs do not provide a reliable live microphone or pre-submit typing signal.
It is not automatically inferred. Tool detection is limited to explicit tool
names in local session metadata; nested or unreported operations stay thinking.
No prompts, tool arguments or microphone recordings are retained by this feature.

Firmware accepts optional `PC_PET_HELLO` / `PC_PET <sequence> <state>` (states
0–5 in the supplied order). Without this protocol it derives basic states from
existing token snapshots. New page configuration uses `PC_PAGES_GET2/SET2`;
legacy four-page NVS order and enabled bits migrate with the pet appended.
Old clients retain their four-page protocol and cannot remove the pet's setting.

The 38 frames retain the GIF timing, transparency and 192×208 resolution.
RGB565 conversion and row RLE use about 850 KiB of flash and a single decoder
row, not a full-frame GIF buffer. The pet widget exists only on its page; animation
updates stop off-page or with the screen off. Existing boot logo and volume slide
behavior are unchanged. Physical acceptance remains a separate authorized test.

Status labels are unified across the usage and pet headers: idle, running, operating, quota low and dictating. Offline behavior is unchanged.
