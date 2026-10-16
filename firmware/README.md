[简体中文](README.zh_CN.md)

# Codex Tokens Kanban v6

This directory contains the complete current firmware, companion, tests and upstream BSP source.
The checked v6 merged firmware SHA-256 is `75c08d1a7d2332eeec46ba86d4857445d055ba0bf722d5aecf8aa31ed21731e2`.

- [Application guide](docs/codex-token-display.md)
- [Board setup and development](docs/README.md)
- Use ESP-IDF 5.5.3 and run `./tools/validate.sh` to build and verify the merged 0x0 image.
- For Python companion use `python -m pip install -r companion/requirements.txt` then
  `python companion/codex_tokens.py --transport auto`, on a computer with Codex installed and signed in.
- The Windows companion and launchers are available in the repository's v6 release.

Build and host tests passed. Authorized device checks passed within the scope documented in the app guide.
A later local rebuild can have a different identity; the released artifact is bound to the hash above.
