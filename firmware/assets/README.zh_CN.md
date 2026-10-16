<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 资源目录（Assets）

Codex Token Display 使用 `fonts/NotoSansCJKsc-Regular.otf`，来源为
[Noto CJK](https://github.com/notofonts/noto-cjk)，授权见 `fonts/OFL.txt`。
`tools/token_font.py` 生成 14px、4bpp PLAIN 的 `fonts/token_font_14.c` 与码点／哈希清单。
固件只编译生成的子集，完整 OTF 用于复现。验证方式见[应用说明](../docs/codex-token-display.zh_CN.md)。

`images/codex-token-reference.png` 为用户提供的 410 × 327 PNG 界面参考，用于布局与配色，
不作为固件背景直接显示，未声明公开再分发许可。

本目录集中存放可复用的资源（字库、图片、音乐等），按资源类型分子目录管理。每个资源放在其类型对应的子目录，并记录放置路径、命名方式、集成方式与来源/许可。二进制资源（字体、图片、音频）不属于纯 markdown 文档，请勿与文档混放。涉及版权/授权的资源需注明来源与许可。

## 字库（fonts）

可复用的字库文件与生成的字库源码放在 `fonts/`。

- 命名要能反映字族、字重、字级与格式。
- 记录来源、许可、字符范围、转换命令与目标放置路径。
- 添加字库前评估 Flash 与内部 RAM 影响；ESP32-C3 无 PSRAM。
- 不提交许可不允许分发的字库。

## 图片（images）

可复用的源图与生成的显示资产放在 `images/`。

| 文件 | 尺寸与格式 | 用途与来源 |
| --- | --- | --- |
| [`images/home.jpg`](images/home.jpg) | 3840 × 2160，JPEG | 嵌入中英文项目 README 的产品主图，突出 AI Passport 产品形象与开放、人人可创作的理念。 |
| [`images/readme-hardware-specs.png`](images/readme-hardware-specs.png) | 2172 × 724，PNG RGBA | 保留为可选技术参考图，不再用于首页主视觉。于 2026-09-17 使用内置图像生成工具为本仓库生成；已根据文档中的硬件能力契约核对图中的六项标签与参数。 |
| [`images/logo-wordmark.png`](images/logo-wordmark.png) | 1648 × 336，PNG RGBA | 从仓库原始 `images/logo.png` 中精确裁切并去除背景的黑色字标；用于中英文项目 README 的浅色主题。 |
| [`images/logo-wordmark-dark.png`](images/logo-wordmark-dark.png) | 1648 × 336，PNG RGBA | 提取字标的白色版本；README 使用 `<picture>` 在 GitHub 深色主题下显示。 |

- 使用描述性命名，并记录尺寸、像素格式、转换步骤与目标路径。
- 优先采用适合 240 × 320 RGB565 显示的格式，并纳入 Flash 与内部 RAM 考量。
- 许可允许时保留可编辑源文件，并记录来源与许可。
- 图片中不得包含设备二维码秘密、凭证或个人数据。

## 音乐与音效（music）

用户提供的 `images/codex-details-reference.png` 和 `images/codex-analytics-reference.png`
原样保留为 Codex 应用的本地设计参考，不嵌入固件。来源：2026-10-16 用户附件；再分发授权未指定。
明信片图片／二维码属于运行时导入内容，不会嵌入或保存在源码仓库。

可复用的音乐与音效源码放在 `music/`。

- 记录来源、许可、采样率、位深、声道、转换命令与目标路径。
- 与当前 BSP 音频路径匹配时优先采用 16 kHz、16 位单声道 PCM。
- 嵌入音频前评估 Flash 与内部 RAM 成本；长录音应流式或分块。
- 无再分发许可不提交媒体文件。

- `images/postcard-date-reference.png`：用户提供的日期／星期栏参考图（2026-10-16），仅作设计参考、不嵌入固件；授权信息未提供。

- `images/postcard-default-avatar.svg`／`.png`：原创左右对称默认用户头像，MIT，使用 `tools/create_default_avatar.py` 生成并随网页分发。原用户参考图另存为 `images/postcard-avatar-reference.png`，不作为默认素材。
- `images/postcard-editor-header-reference.png`：用户提供的原网页顶部截图，仅保留作为改版参考。
- `images/postcard_default_rgb565.c`：由 `tools/postcard_defaults.py` 使用实际网页生成，作为只读 Flash 默认卡片，包含指向公开编辑器的二维码。

## 用户提供的 Codex 提示音（v8）

`music/codex-notification.wav` 用于思考结束（48kHz、16 位、立体声、0.570875 秒）；
`music/codex-error.wav` 用于额度耗尽（48kHz、24 位、立体声、0.586646 秒）。
来源：用户于 2026-10-16 提供的桌面附件；不另行声明所有权或再分发许可，
原文件保持不变，用于本次指定实现。

运行 `python3 tools/convert_codex_sounds.py`（或 `--check`）生成
`music/codex_sounds.c` 和哈希／格式清单 `music/codex_sounds.json`。
转换器仅依赖 Python 标准库，平均声道、低通滤波后降采样至 16kHz／16 位单声道，
不增益、不裁剪、不改变速度。固件只读 Flash 保存 37042 字节采样，
通过 640 字节栈缓冲分块交给 BSP，不在 RAM 中分配完整音效，也不在设备解析 WAV 头。
清单记录原始与转换后内容哈希。

## 三套用户提供音效（v9）

只链接用户提供的录音：Codex 完成／错误 WAV，OpenCode 的
`opencode-complete.aac`／`opencode-error.aac`，以及 Antigravity 的
`antigravityCascadeDone.mp3`／`antigravityActionRequired.mp3`。来源为用户
2026-10-16 附件，原文件保持不变，不另行声明再分发许可。Antigravity 的
ActionRequired 用于额度耗尽事件。

运行 `python3 tools/convert_codex_sounds.py --decode --ffmpeg <FFmpeg路径>`，
先解码压缩源为 16kHz 单声道 WAV 缓存，再生成现有 `music/codex_sounds.c`
与 JSON 清单，内含三套音效。`--decode --check` 可复核解码；普通 `--check`
仅依赖标准库，通过 `recording_decode.json` 的源／缓存 SHA 绑定校验。
Codex 既有滤波后 PCM 保持不变。不手工裁剪、不归一化、不改变速度，
遵循解码器的标准填充元数据。

六段采样共 181320 字节保存在只读 Flash，播放缓冲仍为 640 字节，
设备不需要 AAC／MP3 解码器。原合成柔和双音和本应用的正弦合成播放代码均已移除。

## 提供的开机素材（v10）

`images/startup-logo.png`、`music/macos-startup.mp3` 为 2026-10-16 用户附件原件，
保持不变，所有权／转载授权未注明。`tools/convert_startup_assets.py` 在黑底合成图片，
将可见图案按比例适配到 88 × 110（v12；v10 为 180 × 224，v11 为 96 × 119），居中放入 240 × 320 画布，输出实际量化预览
`images/startup-preview.png` 及只读 Flash 的 `startup_assets.c`。FFmpeg 在电脑端
解码出 `music/startup.wav`（16kHz／16-bit 单声道）；源文件／缓存绑定为
`music/startup_decode.json`，几何、时长和哈希写入 `images/startup_assets.json`。
不手工裁剪音频、不加增益或变速。图片占 153600 Flash 字节，PCM 占 92082 字节，
仍以 640 字节块播放，不在运行时申请完整素材大小的 RAM。

素材转换依赖 `python3 -m pip install -r tools/requirements-assets.txt`（Pillow 11.3.0）；只有重新生成 MP3 解码缓存时才需要 FFmpeg。

### 系统监测预览

`images/system-monitor-reference.png` 为用户提供的六项指标参考。
`companion/postcard/page-0.png` 至 `page-3.png` 为原生 LVGL 测试导出的原创布局预览，
依次为首页、统计、系统监测、明信片。数据是示例，不冒充实机照片或实时电脑读数，
中文沿用已获许可的 Noto Sans CJK 字体子集。
