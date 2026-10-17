"""Opt-in LaunchAgent for the source-based macOS preview; active jobs are untouched."""
import os
from pathlib import Path
import plistlib
import sys
import tempfile

LABEL = 'io.github.greentailtohru-arch.tokens-kanban.sync'
MARKER = 'Tokens Kanban managed login startup v1'


def configure(enabled, python, script, home=None):
    if sys.platform != 'darwin':
        raise OSError('此启动项仅适用于 macOS。')
    home = Path(home) if home else Path.home()
    folder = home / 'Library/LaunchAgents'
    target = folder / (LABEL + '.plist')
    # Keep the venv Python path: resolving its symlink would lose installed packages.
    python, script = Path(os.path.abspath(python)), Path(script).resolve()
    if target.is_symlink():
        raise OSError('启动项是符号链接，未修改。')
    if target.exists():
        try:
            current = plistlib.loads(target.read_bytes())
        except (ValueError, plistlib.InvalidFileException) as error:
            raise OSError('已有启动项无法识别，未修改。') from error
        if current.get('Label') != LABEL or current.get('Comment') != MARKER:
            raise OSError('已有同名启动项不属于本程序，未修改。')
    if not enabled:
        target.unlink(missing_ok=True)
        return {'enabled': False}
    if not python.is_file() or not script.is_file():
        raise OSError('请保留完整解压目录，并使用 start.command 设置登录启动。')
    folder.mkdir(parents=True, exist_ok=True)
    logs = home / 'Library/Logs/TokensKanban'
    logs.mkdir(parents=True, exist_ok=True)
    data = {
        'Label': LABEL, 'Comment': MARKER,
        'ProgramArguments': [str(python), str(script), '--transport', 'auto',
                             '--background', '--no-browser'],
        'WorkingDirectory': str(script.parent), 'RunAtLoad': True,
        'ProcessType': 'Background',
        'EnvironmentVariables': {
            'PATH': os.environ.get('PATH', '/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin'),
            'PYTHONUTF8': '1',
            **({'CODEX_HOME': os.environ['CODEX_HOME']} if os.environ.get('CODEX_HOME') else {}),
        },
        'StandardOutPath': str(logs / 'launch.log'),
        'StandardErrorPath': str(logs / 'launch-error.log'),
    }
    # Write atomically without touching other jobs or starting/stopping the sender.
    with tempfile.NamedTemporaryFile(dir=folder, prefix=LABEL, suffix='.tmp', delete=False) as f:
        temporary = Path(f.name)
        plistlib.dump(data, f)
    try:
        temporary.chmod(0o600)
        temporary.replace(target)
    finally:
        temporary.unlink(missing_ok=True)
    if plistlib.loads(target.read_bytes()) != data:
        raise OSError('登录启动设置校验失败。')
    return {'enabled': True}
