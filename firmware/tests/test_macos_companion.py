"""Portable contract tests; these do not replace execution on a real Mac."""
from pathlib import Path
import plistlib
import sys
import tempfile
import unittest
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'companion'))
from macos_startup import configure, LABEL
from management_server import LinkManager
from system_monitor import SystemPoller, UNKNOWN
from codex_tokens import candidates


class MacContracts(unittest.TestCase):
    def test_launch_agent_roundtrip_and_ownership(self):
        with tempfile.TemporaryDirectory() as tmp, patch('macos_startup.sys.platform', 'darwin'):
            home = Path(tmp) / "中文 User's Folder"
            home.mkdir()
            python = home / 'python3'; python.touch()
            script = home / 'codex_tokens.py'; script.touch()
            self.assertTrue(configure(True, python, script, home)['enabled'])
            target = home / 'Library/LaunchAgents' / (LABEL + '.plist')
            data = plistlib.loads(target.read_bytes())
            self.assertEqual(data['ProgramArguments'], [str(python), str(script.resolve()),
                '--transport', 'auto', '--background', '--no-browser'])
            self.assertTrue(data['RunAtLoad'])
            self.assertNotIn('KeepAlive', data)  # stop-sync must stay stopped
            self.assertTrue(configure(True, python, script, home)['enabled'])
            self.assertFalse(configure(False, python, script, home)['enabled'])
            self.assertFalse(configure(False, python, script, home)['enabled'])
            target.write_bytes(plistlib.dumps({'Label': LABEL, 'Comment': 'foreign'}))
            for enabled in (True, False):
                with self.assertRaises(OSError): configure(enabled, python, script, home)
            self.assertEqual(plistlib.loads(target.read_bytes())['Comment'], 'foreign')

    def test_missing_runtime_does_not_create_job(self):
        with tempfile.TemporaryDirectory() as tmp, patch('macos_startup.sys.platform', 'darwin'):
            with self.assertRaises(OSError):configure(True, Path(tmp)/'missing', __file__, tmp)
            self.assertFalse((Path(tmp)/'Library/LaunchAgents').exists())

    def test_macos_serial_classification_and_selection(self):
        from types import SimpleNamespace
        port = SimpleNamespace(device='/dev/cu.usbmodem101', vid=0x303A, pid=0x1001)
        unrelated = SimpleNamespace(device='/dev/cu.other', vid=1, pid=2)
        with patch('serial.tools.list_ports.comports', return_value=[port, unrelated]):
            self.assertEqual(candidates(), [port.device])
        manager = LinkManager()
        manager.attach(SimpleNamespace(port=port.device))
        self.assertEqual(manager.kind, 'USB')
        manager.attach(SimpleNamespace(port='BLE macOS-UUID'))
        self.assertEqual(manager.kind, '蓝牙')

    def test_no_windows_sensor_access_or_fake_zero_on_mac(self):
        with patch('system_monitor.sys.platform', 'darwin'), patch('system_monitor.MailslotReader') as reader:
            monitor = SystemPoller()
            try:
                self.assertTrue(monitor.ready.wait(2))
                reader.assert_not_called()
                self.assertEqual(monitor.values(), UNKNOWN)
                self.assertIn('Windows-only', monitor.error)
            finally: monitor.close()


if __name__ == '__main__': unittest.main()
