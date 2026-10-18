import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'companion'))
from pet_remote import PetRemote,DesktopKeys,open_accessible_codex,dictation_keys,screenshot_keys,parse_custom_shortcut,custom_shortcut
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
        shortcut=patch('pet_remote.dictation_keys',return_value=(0xA2,0xA4,0x78));shortcut.start()
        self.addCleanup(shortcut.stop)
        b=Keys()
        try:
            b.hold();self.assertEqual(len(b.held),3)
            b.last_poll=time.monotonic()-2
            deadline=time.monotonic()+1
            while b.held and time.monotonic()<deadline:time.sleep(.02)
            self.assertFalse(b.held)
            self.assertEqual(b.events[-3:],[(0x78,False),(0xA4,False),(0xA2,False)])
            b.last_poll=time.monotonic();b.hold();b.focus=2;b.tick();self.assertTrue(b.held)
            b.focus=0;b.tick();self.assertFalse(b.held)
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
    def test_native_shortcuts_have_physical_scan_codes(self):
        import ctypes
        from ctypes import wintypes
        class Keyboard(ctypes.Structure):
            _fields_=[('vk',wintypes.WORD),('scan',wintypes.WORD),('flags',wintypes.DWORD),('time',wintypes.DWORD),('extra',ctypes.c_size_t)]
        class Data(ctypes.Union):
            _fields_=[('keyboard',Keyboard),('padding',ctypes.c_byte*32)]
        class Input(ctypes.Structure):
            _fields_=[('type',wintypes.DWORD),('data',Data)]
        events=[]
        def send(n,item,size):
            self.assertEqual(size,ctypes.sizeof(Input))
            k=ctypes.cast(item,ctypes.POINTER(Input)).contents.data.keyboard
            events.append((k.vk,k.scan,k.flags));return 1
        b=DesktopKeys.__new__(DesktopKeys)
        b.user=SimpleNamespace(MapVirtualKeyW=lambda vk,mode:{0xA4:0x38,0xA5:0x38,0xA2:0x1d,0x78:0x43}[vk],SendInput=send)
        for vk in [0xA4,0xA5,0xA2,0x78]:b.key(vk,True);b.key(vk,False)
        self.assertEqual(events,[(0,0x38,8),(0,0x38,10),(0,0x38,9),(0,0x38,11),(0,0x1d,8),(0,0x1d,10),(0,0x43,8),(0,0x43,10)])
    def test_screenshot_releases_both_alt_keys(self):
        b=DesktopKeys.__new__(DesktopKeys);b.supported=True;events=[]
        b.key=lambda k,down:events.append((k,down))
        with patch('pet_remote.screenshot_keys',return_value=(0xA4,0xA5)):b.action(4)
        self.assertEqual(events,[(0xA4,True),(0xA5,True),(0xA5,False),(0xA4,False)])

    def test_configurable_dictation_binding(self):
        import tempfile,json
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'pet-controls.json'
            self.assertEqual(dictation_keys(p),(0xA2,0xA4,0x78))
            p.write_text(json.dumps({'dictation_hotkey':'Alt+H'}),encoding='utf-8')
            self.assertEqual(dictation_keys(p),(0xA4,0x48))
            for invalid in ['H','Ctrl+Alt+Delete','Ctrl+Ctrl+H','Alt+F25',None]:
                p.write_text(json.dumps({'dictation_hotkey':invalid}),encoding='utf-8')
                with self.assertRaises(OSError):dictation_keys(p)

    def test_custom_shortcut_and_preserved_dictation(self):
        import tempfile,json
        self.assertEqual(parse_custom_shortcut('Space'),(0x20,))
        self.assertEqual(parse_custom_shortcut('Ctrl+NumpadAdd'),(0xA2,0x6B))
        self.assertEqual(parse_custom_shortcut('Shift+Slash'),(0xA0,0xBF))
        self.assertEqual(parse_custom_shortcut('LeftAlt+RightAlt'),(0xA4,0xA5))
        self.assertEqual(parse_custom_shortcut('F8'),(0x77,))
        self.assertEqual(parse_custom_shortcut('Ctrl+Shift+A'),(0xA2,0xA0,0x41))
        self.assertEqual(parse_custom_shortcut(''),())
        for value in ['A+B','Ctrl+Ctrl+A','F25','Ctrl+','unknown',None]:
            with self.assertRaises(ValueError):parse_custom_shortcut(value)
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'controls.json';p.write_text(json.dumps({'dictation_hotkey':'Alt+H'}))
            with patch('pet_remote.custom_shortcut_path',return_value=p):
                self.assertEqual(screenshot_keys(),())
                custom_shortcut('Ctrl+Shift+A')
                self.assertEqual(screenshot_keys(),(0xA2,0xA0,0x41))
                self.assertEqual(dictation_keys(p),(0xA4,0x48))
                custom_shortcut('');self.assertEqual(screenshot_keys(),())

    def test_independent_custom_bindings(self):
        from pet_remote import custom_shortcuts
        import tempfile,json
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'controls.json'
            path.write_text(json.dumps({'custom_hotkey':'Alt+T','dictation_hotkey':'Alt+H'}))
            with patch('pet_remote.custom_shortcut_path',return_value=path):
                values=custom_shortcuts();self.assertEqual(values,['Alt+T']*8)
                values[4]='Ctrl+Shift+S';custom_shortcuts(values)
                self.assertEqual(custom_shortcuts()[0],'Alt+T')
                self.assertEqual(custom_shortcuts()[4],'Ctrl+Shift+S')
                self.assertEqual(dictation_keys(path),(0xA4,0x48))
                before=path.read_bytes()
                with self.assertRaises(ValueError):custom_shortcuts(['Bad']*8)
                self.assertEqual(path.read_bytes(),before)
                b=DesktopKeys.__new__(DesktopKeys);b.supported=True;events=[]
                b.key=lambda k,down:events.append((k,down))
                with patch('pet_remote.time.sleep'):
                    b.action(16)
                    self.assertEqual(events,[(0xA4,True),(0x54,True),(0x54,False),(0xA4,False)])
                    events.clear();b.action(20)
                    self.assertEqual(events,[(0xA2,True),(0xA0,True),(0x53,True),(0x53,False),(0xA0,False),(0xA2,False)])
                backend=Backend();remote=PetRemote(backend)
                remote.poll(None,lambda c,q:'PC_CTRL 1 1 20 0')
                remote.poll(None,lambda c,q:'PC_CTRL 1 1 20 0')
                values[0]='';custom_shortcuts(values)
                self.assertEqual(custom_shortcuts()[4],'Ctrl+Shift+S')

    def test_invalid(self):
        for reply in ['PC_CTRL 2 0 0 0','PC_CTRL 1 0 9 0','PC_CTRL 1 0 0 2']:
            with self.assertRaises(OSError):PetRemote(Backend()).poll(None,lambda c,q:reply)

if __name__=='__main__':unittest.main()
