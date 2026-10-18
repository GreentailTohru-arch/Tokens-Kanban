import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'companion'))
from pet_remote import PetRemote,DesktopKeys,open_accessible_codex
import time
from unittest.mock import patch
from types import SimpleNamespace

class Backend:
    def __init__(self):self.calls=[];self.held=[];self.error=''
    def hold(self):self.calls.append('hold');self.held=[1]
    def release(self):self.calls.append('release');self.held=[]
    def action(self,a):self.calls.append(a)
    def tick(self):pass

class TestRemote(unittest.TestCase):
    def test_compatibility_launch_never_closes_running_app(self):
        with patch('sys.platform','win32'),patch('subprocess.run',return_value=SimpleNamespace(returncode=2)) as run:
            with self.assertRaisesRegex(OSError,'完全退出'):open_accessible_codex()
            script=run.call_args.args[0][-1]
            self.assertNotIn('Stop-Process',script);self.assertIn('--force-renderer-accessibility',script)
        with patch('sys.platform','win32'),patch('subprocess.run',return_value=SimpleNamespace(returncode=0)):
            self.assertIn('已请求',open_accessible_codex())
    def test_recording_watchdog_without_real_input(self):
        class Keys(DesktopKeys):
            focus=1
            events=[]
            def foreground(self):return self.focus
            def key(self,code,down):self.events.append((code,down))
        b=Keys()
        try:
            b.hold();self.assertEqual(len(b.held),3)
            b.last_poll=time.monotonic()-2
            deadline=time.monotonic()+1
            while b.held and time.monotonic()<deadline:time.sleep(.02)
            self.assertFalse(b.held)
            self.assertEqual(b.events[-3:],[(0x78,False),(0x12,False),(0x11,False)])
            b.last_poll=time.monotonic();b.hold();b.focus=2;b.tick();self.assertFalse(b.held)
        finally:b.close()
    def test_retry_dedup_and_release(self):
        b=Backend();r=PetRemote(b)
        for reply in ['PC_CTRL 1 1 1 0','PC_CTRL 1 1 1 0','PC_CTRL 1 1 0 1','PC_CTRL 1 1 0 1','PC_CTRL 1 1 0 0']:
            r.poll(None,lambda c,q:reply)
        self.assertEqual(b.calls,[1,'hold','release'])
        old=r.session;r.reset();self.assertNotEqual(old,r.session);self.assertEqual(r.ack,0)
    def test_failed_send_not_replayed(self):
        b=Backend();r=PetRemote(b)
        def fail(a):b.calls.append(a);raise OSError('not focused')
        b.action=fail
        for _ in range(2):r.poll(None,lambda c,q:'PC_CTRL 1 7 1 0')
        self.assertEqual(b.calls,[1]);self.assertEqual(b.error,'not focused')
    def test_no_send_while_recording(self):
        b=Backend();r=PetRemote(b);r.poll(None,lambda c,q:'PC_CTRL 1 1 1 1')
        self.assertEqual(b.calls,['hold']);r.reset();self.assertEqual(b.calls[-1],'release')
    def test_invalid(self):
        for reply in ['PC_CTRL 2 0 0 0','PC_CTRL 1 0 9 0','PC_CTRL 1 0 0 2']:
            with self.assertRaises(OSError):PetRemote(Backend()).poll(None,lambda c,q:reply)

if __name__=='__main__':unittest.main()
