[简体中文](README.zh_CN.md)

# Codex Tokens Kanban v9

Complete firmware, companion, tests, original recordings and reused BSP source.
The verified merged-image SHA256 is `220dd600137cc9622754746a82492687c7006ebb2d4b0b619821147fb1bd25f3`.

- [Application guide and acceptance](docs/codex-token-display.md)
- [Board setup](docs/README.md)
- Use ESP-IDF 5.5.3 and `./tools/validate.sh` to build and verify a merged 0x0 image.
- Install `companion/requirements.txt`, then run `python companion/codex_tokens.py --transport auto`.
- The Windows program and launchers are in this repository's v9 release.

Build/host checks and bounded device acceptance passed; long-run/range/stress tests remain unverified.
Rebuilding may produce a different identity from the checked release.
