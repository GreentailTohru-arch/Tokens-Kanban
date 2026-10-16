"""Completion notifications: explicit lifecycle events, no prompts or message text."""
import secrets
import time


class CompletionTracker:
    def __init__(self):
        self.seen = None
        self.session = secrets.randbits(32) or 1
        self.sequence = 0
        self.pending = None

    def observe(self, completions, now=None):
        now = time.time() if now is None else now
        keys = set(completions)
        if self.seen is None:
            self.seen = keys  # Never announce history when the program starts.
        else:
            fresh = keys - self.seen
            self.seen |= keys
            if any(0 <= now - stamp <= 60 for _, stamp in fresh):
                self.sequence = self.sequence % 0xffffffff + 1
                self.pending = (self.sequence, now)
        if self.pending and now-self.pending[1] > 60:
            self.pending = None

    def send(self, connection):
        if not self.pending:
            return
        sequence = self.pending[0]
        reply = rpc(connection, f'PC_EVT {self.session} {sequence} 1')
        if reply != f'PC_EVT_ACK {self.session} {sequence}':
            raise OSError('Completion notification not acknowledged: '+reply)
        self.pending = None


def rpc(connection, command):
    connection.write((command+'\n').encode())
    deadline = time.monotonic()+4
    while time.monotonic() < deadline:
        line = connection.readline(256)
        if line.startswith(b'PC_'):
            return line.decode('ascii').strip()
    raise OSError('Device settings response timed out')


def supports_events(connection):
    return rpc(connection, 'PC_CFG_GET').startswith('PC_CFG 1 ')
