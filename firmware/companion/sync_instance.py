"""Per-user OS file lock, released automatically even after a process crash."""
import os
from pathlib import Path


class SyncInstance:
    def __init__(self, path=None):
        self.path = Path(path) if path else Path(os.environ.get(
            'LOCALAPPDATA', str(Path.home() / '.local/share'))) / 'TokensKanban/sync.lock'
        self.handle = None
        self.acquired = False

    def __enter__(self):
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.handle = self.path.open('a+b')
        if self.handle.seek(0, 2) == 0:
            self.handle.write(b'0')
            self.handle.flush()
        self.handle.seek(0)
        try:
            if os.name == 'nt':
                import msvcrt
                msvcrt.locking(self.handle.fileno(), msvcrt.LK_NBLCK, 1)
            else:
                import fcntl
                fcntl.flock(self.handle, fcntl.LOCK_EX | fcntl.LOCK_NB)
            self.acquired = True
        except OSError:
            self.handle.close()
            self.handle = None
        return self

    def __exit__(self, *args):
        if self.handle:
            self.handle.close()
        self.handle = None
        self.acquired = False
