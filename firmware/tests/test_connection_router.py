"""Host-only transport handoff and cross-process sender ownership tests."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'companion'))
from connection_router import ConnectionRouter
from sync_instance import SyncInstance


class Link:
    def __init__(self, port):
        self.port = port
        self.closed = False

    def close(self):
        self.closed = True


class RouterTests(unittest.TestCase):
    def setUp(self):
        self.ports = []
        self.calls = []
        self.time = 100
        self.busy = False
        self.router = ConnectionRouter('auto', lambda explicit: self.ports,
            self.usb, self.ble, clock=lambda: self.time)

    def usb(self, port):
        self.calls.append(('usb', port))
        if self.busy:
            raise OSError('Port busy')
        return Link(port)

    def ble(self, address):
        self.calls.append(('ble', address))
        return Link('BLE TEST')

    def test_usb_first(self):
        self.ports = ['COM9']
        self.assertEqual(self.router.ensure(None).port, 'COM9')
        self.assertEqual(self.calls, [('usb', 'COM9')])

    def test_bluetooth_to_usb_and_back(self):
        old = self.router.ensure(None)
        self.ports = ['COM9']
        current = self.router.ensure(old)
        self.assertTrue(old.closed)
        self.assertEqual(current.port, 'COM9')
        self.ports = []
        self.router.failed(current)
        self.assertTrue(current.closed)
        self.assertEqual(self.router.ensure(None).port, 'BLE TEST')

    def test_busy_usb_keeps_ble_and_backs_off(self):
        old = self.router.ensure(None)
        self.ports = ['COM9']; self.busy = True
        self.assertIs(self.router.ensure(old), old)
        self.assertFalse(old.closed)
        self.router.ensure(old)
        self.assertEqual(self.calls.count(('usb', 'COM9')), 1)
        self.time += 31; self.busy = False
        self.assertEqual(self.router.ensure(old).port, 'COM9')

    def test_busy_usb_at_start_falls_back(self):
        self.ports = ['COM9']; self.busy = True
        self.assertEqual(self.router.ensure(None).port, 'BLE TEST')

    def test_recent_save_defers_upgrade(self):
        old = self.router.ensure(None); self.ports = ['COM9']
        self.assertIs(self.router.ensure(old, allow_upgrade=False), old)
        self.assertEqual(self.calls, [('ble', None)])
        self.assertEqual(self.router.ensure(old).port, 'COM9')

    def test_multiple_usb_does_not_steal_working_link(self):
        self.ports = ['COM9', 'COM10']; old = Link('BLE TEST')
        self.assertIs(self.router.ensure(old), old)
        with self.assertRaisesRegex(OSError, 'Multiple USB'):
            self.router.ensure(None)

    def test_usb_only_never_opens_ble(self):
        self.router.mode = 'usb'
        with self.assertRaises(OSError):self.router.ensure(None)
        self.assertEqual(self.calls, [])

    def test_ble_only_never_opens_usb(self):
        self.router.mode = 'ble'; self.ports = ['COM9']
        old = self.router.ensure(None)
        self.assertIs(self.router.ensure(old), old)
        self.assertEqual(self.calls, [('ble', None)])

    def test_failed_usb_waits_before_retry(self):
        self.ports = ['COM9']; old = self.router.ensure(None)
        self.router.failed(old)
        new = self.router.ensure(None)
        self.assertEqual(new.port, 'BLE TEST')
        self.assertEqual(self.calls.count(('usb', 'COM9')), 1)

    def test_replug_clears_backoff(self):
        self.ports = ['COM9']; self.busy = True
        old = self.router.ensure(None)
        self.ports = []; self.router.ensure(old)
        self.ports = ['COM9']; self.busy = False
        self.assertEqual(self.router.ensure(old).port, 'COM9')

    def test_explicit_ble_address_not_promoted_to_unknown_usb(self):
        self.router.ble_address = 'AA'
        old = Link('BLE AA'); self.ports = ['COM9']
        self.assertIs(self.router.ensure(old), old)
        self.assertFalse(self.calls)


class InstanceTests(unittest.TestCase):
    def test_lock_across_processes_and_crash_recovery(self):
        script = ('import sys;from sync_instance import SyncInstance;'
                  'lock=SyncInstance(sys.argv[1]).__enter__();'
                  'print(int(lock.acquired),flush=True);'
                  'sys.stdin.readline()')
        env = {**os.environ, 'PYTHONPATH': str(Path(__file__).resolve().parents[1] / 'companion')}
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'sync.lock'
            child = subprocess.Popen([getattr(sys, '_base_executable', sys.executable), '-c', script, str(path)],
                stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, env=env)
            try:
                self.assertEqual(child.stdout.readline().strip(), '1')
                with SyncInstance(path) as other:self.assertFalse(other.acquired)
                child.kill(); child.wait(timeout=5)
                with SyncInstance(path) as recovered:self.assertTrue(recovered.acquired)
            finally:
                if child.poll() is None:child.kill();child.wait()
                child.stdin.close();child.stdout.close()


if __name__ == '__main__':unittest.main()
