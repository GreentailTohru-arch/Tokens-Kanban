"""Startup registration tests use a temporary folder, never the actual Startup folder."""
from pathlib import Path
import os,sys,tempfile,unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'companion'))
from windows_startup import configure,startup_arguments,ps_literal

class Tests(unittest.TestCase):
    def test_literal_quoting(self):
        self.assertEqual(ps_literal("a'b$()"),"'a''b$()'")
        value=startup_arguments(Path('space and 中文')/"user's"/'program.exe')
        self.assertIn("user''s",value)
        self.assertIn('--background',value)
        self.assertIn('--no-browser',value)
        self.assertNotIn('--enable-startup',value)

    @unittest.skipUnless(os.name=='nt','Windows shortcut integration')
    def test_enable_repeat_move_disable_and_foreign_file(self):
        with tempfile.TemporaryDirectory(prefix='tokens-startup-test-') as temp:
            root=Path(temp);folder=root/'startup';exe=root/"中文 user's app.exe"
            exe.write_bytes(b'test placeholder - never executed')
            self.assertTrue(configure(True,exe,folder)['enabled'])
            self.assertTrue((folder/'Tokens Kanban.lnk').is_file())
            self.assertTrue(configure(True,exe,folder)['enabled'])
            moved=root/'another folder'/'app.exe';moved.parent.mkdir();moved.write_bytes(b'test')
            self.assertTrue(configure(True,moved,folder)['enabled'])
            self.assertFalse(configure(False,moved,folder)['enabled'])
            self.assertFalse(configure(False,moved,folder)['enabled'])
            foreign=folder/'Tokens Kanban.lnk';foreign.write_bytes(b'not-owned')
            with self.assertRaises(OSError):configure(True,moved,folder)
            self.assertEqual(foreign.read_bytes(),b'not-owned')
            with self.assertRaises(OSError):configure(False,moved,folder)
            self.assertEqual(foreign.read_bytes(),b'not-owned')

    def test_missing_exe(self):
        with self.assertRaises(OSError):configure(True,'missing-startup-executable.exe')

if __name__=='__main__':unittest.main()
