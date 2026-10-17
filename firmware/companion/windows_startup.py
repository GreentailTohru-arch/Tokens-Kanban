"""Opt-in current-user startup shortcut; no admin privileges or extra package files."""
import json
import os
from pathlib import Path
import subprocess

MARKER = 'Tokens Kanban automatic sync (managed startup v1)'


def ps_literal(value):
    return "'" + str(value).replace("'", "''") + "'"


def startup_arguments(executable):
    exe = Path(executable).resolve()
    command = ('Start-Process -FilePath ' + ps_literal(exe) +
               " -ArgumentList '--transport','auto','--background','--no-browser'" +
               ' -WorkingDirectory ' + ps_literal(exe.parent) + ' -WindowStyle Hidden')
    return subprocess.list2cmdline(['-NoProfile', '-NonInteractive', '-WindowStyle',
                                    'Hidden', '-Command', command])


def configure(enabled, executable, startup_dir=None):
    if os.name != 'nt':
        raise OSError('开机启动目前仅支持 Windows。')
    exe = Path(executable).resolve()
    if enabled and (not exe.is_file() or exe.suffix.lower() != '.exe'):
        raise OSError('请从已解压的 Windows 程序包启用开机启动。')
    shell = Path(os.environ['SystemRoot']) / 'System32/WindowsPowerShell/v1.0/powershell.exe'
    # Values arrive over stdin as JSON, never interpolated into executable code.
    script = r'''
$ErrorActionPreference='Stop'
$settings=[Console]::In.ReadToEnd() | ConvertFrom-Json
$folder=if($settings.folder){$settings.folder}else{[Environment]::GetFolderPath('Startup')}
if(-not $folder){throw 'Windows Startup folder unavailable'}
$folder=[IO.Path]::GetFullPath($folder)
if(-not (Test-Path -LiteralPath $folder)){New-Item -ItemType Directory -Path $folder | Out-Null}
$path=Join-Path $folder 'Tokens Kanban.lnk'
$ws=New-Object -ComObject WScript.Shell
if(Test-Path -LiteralPath $path){
  $current=$ws.CreateShortcut($path)
  if($current.Description -ne $settings.marker){throw 'Startup shortcut already exists and is not owned by this program'}
}
if($settings.enabled){
  $link=$ws.CreateShortcut($path)
  $link.TargetPath=$settings.shell
  $link.Arguments=$settings.arguments
  $link.WorkingDirectory=$settings.working
  $link.Description=$settings.marker
  $link.IconLocation=$settings.executable+',0'
  $link.WindowStyle=7
  $link.Save()
  $saved=$ws.CreateShortcut($path)
  if($saved.TargetPath -ne $settings.shell -or $saved.Arguments -ne $settings.arguments){throw 'Startup shortcut verification failed'}
}elseif(Test-Path -LiteralPath $path){Remove-Item -LiteralPath $path}
@{enabled=[bool](Test-Path -LiteralPath $path)} | ConvertTo-Json -Compress
'''
    data = dict(enabled=bool(enabled), executable=str(exe), shell=str(shell),
                arguments=startup_arguments(exe), working=str(exe.parent),
                folder=str(Path(startup_dir).resolve()) if startup_dir else None, marker=MARKER)
    # ASCII JSON works regardless of the Windows PowerShell console code page.
    result = subprocess.run([str(shell), '-NoProfile', '-NonInteractive', '-Command', script],
        input=json.dumps(data), capture_output=True, text=True, errors='replace', timeout=15,
        creationflags=subprocess.CREATE_NO_WINDOW)
    if result.returncode:
        raise OSError('无法修改开机启动；请检查启动文件夹权限或是否已有同名快捷方式。')
    answer = json.loads(result.stdout)
    if answer.get('enabled') != bool(enabled):
        raise OSError('开机启动设置校验失败。')
    return answer


def background_output():
    """Capture background errors without keeping a visible console or unbounded log."""
    import sys
    root = Path(os.environ.get('LOCALAPPDATA', Path.home() / '.local/share')) / 'TokensKanban'
    root.mkdir(parents=True, exist_ok=True)
    path = root / 'startup-sync.log'
    # Rotate on each login once the previous file exceeds 1 MiB.
    if path.exists() and path.stat().st_size > 1024 * 1024:
        path.replace(root / 'startup-sync.previous.log')
    sys.stdout = sys.stderr = path.open('a', encoding='utf-8', buffering=1)
