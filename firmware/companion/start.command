#!/bin/bash
set -e
cd -- "$(dirname -- "$0")"
export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"
export PYTHONUTF8=1
export PYTHONIOENCODING=utf-8
fail() { printf '\n%s\n' "$1"; read -r -p '按回车关闭 / Press Enter to close... ' ignored; exit 1; }
trap 'fail "启动未完成，请查看上方错误后重试。Startup failed; check the error above."' ERR
[[ "$(uname -s)" == Darwin ]] || fail '此程序包仅适用于 macOS。This package is for macOS.'
command -v python3 >/dev/null || fail '请先从 https://www.python.org/downloads/macos/ 安装 Python 3.10 或更新版本。'
python3 -c 'import sys; assert sys.version_info >= (3,10)' || fail '需要 Python 3.10 或更新版本。Python 3.10+ is required.'
if [[ ! -x .runtime/bin/python3 ]]; then
    printf '首次准备运行环境 / Preparing runtime...\n'
    python3 -m venv .runtime
fi
if ! .runtime/bin/python3 -c 'from importlib.metadata import version; assert version("pyserial")=="3.5" and version("bleak")=="3.0.2"; import serial, bleak' 2>/dev/null; then
    printf '首次需联网下载依赖 / Downloading dependencies...\n'
    .runtime/bin/python3 -m pip install --disable-pip-version-check -r requirements.txt
fi
.runtime/bin/python3 codex_tokens.py --launcher
read -r -p '按回车关闭 / Press Enter to close... ' ignored
