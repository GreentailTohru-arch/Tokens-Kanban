#!/usr/bin/env python3
"""Create a source-based macOS preview, excluding Windows binaries and user data."""
from pathlib import Path
import argparse
import hashlib
import json
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'companion'))
from postcard_server import ASSETS


def package(destination):
    destination = Path(destination)
    destination.parent.mkdir(parents=True, exist_ok=True)
    files = {}
    for source in (ROOT / 'companion').glob('*.py'):
        files['app/' + source.name] = source.read_bytes()
    files['app/requirements.txt'] = (ROOT / 'companion/requirements.txt').read_bytes()
    for relative, _mime in ASSETS.values():
        files['app/postcard/' + relative] = (ROOT / 'companion/postcard' / relative).read_bytes()
    launcher = (ROOT / 'companion/start.command').read_text(encoding='utf-8')
    launcher = launcher.replace('cd -- "$(dirname -- "$0")"', 'cd -- "$(dirname -- "$0")/app"')
    files['start.command'] = launcher.encode('utf-8')
    guide = (ROOT / 'docs/macos-companion.zh_CN.md').read_text(encoding='utf-8').split('\n', 1)[1].lstrip()
    files['使用说明.txt'] = ('保留 start.command 和完整 app 文件夹；ZIP 可以删除。\n\n' + guide).encode('utf-8-sig')
    license_file = ROOT / 'LICENSE'
    if license_file.exists(): files['app/LICENSE.txt'] = license_file.read_bytes()
    prefix = 'Tokens Kanban Mac/'
    with zipfile.ZipFile(destination, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
        for name, data in sorted(files.items()):
            info = zipfile.ZipInfo(prefix + name)
            info.create_system = 3
            info.external_attr = (0o100755 if name == 'start.command' else 0o100644) << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, data)
    with zipfile.ZipFile(destination) as archive:
        assert archive.testzip() is None
        assert set(archive.namelist()) == {prefix + name for name in files}
        assert not any(name.endswith(('.dll', '.exe', '.cmd', '.pyc')) for name in archive.namelist())
        assert (archive.getinfo(prefix + 'start.command').external_attr >> 16) & 0o111
        for name, data in files.items(): assert archive.read(prefix + name) == data
    result = {'package': str(destination), 'files': len(files), 'bytes': destination.stat().st_size,
        'sha256': hashlib.sha256(destination.read_bytes()).hexdigest(),
        'zipIntegrity': 'PASS', 'executableCommandMode': 'PASS', 'macRuntimeTest': 'NOT RUN'}
    print(json.dumps(result, ensure_ascii=False))
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('destination', type=Path)
    package(parser.parse_args().destination)
