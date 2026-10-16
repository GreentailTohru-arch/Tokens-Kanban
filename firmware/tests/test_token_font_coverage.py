"""Check every UI literal against generated font coverage without optional packages."""
import json
from pathlib import Path
import re
import unittest
ROOT=Path(__file__).resolve().parents[1]
class FontTests(unittest.TestCase):
    def test_literal_coverage_and_negative_control(self):
        inventory=json.loads((ROOT/'assets/fonts/token_font_inventory.json').read_text())
        points=set(inventory['codepoints'])
        source=(ROOT/'main/token_ui.c').read_text(encoding='utf-8')
        for literal in re.findall(r'"([^"\n]*)"',source):
            for char in literal:
                self.assertIn(ord(char),points,repr(char))
        self.assertNotIn(0x9F98,points)
        self.assertIn('lv_obj_set_style_text_font(o, &token_font_14',source)
        self.assertEqual(inventory['lineHeight'],18)
    def test_startup_has_no_demo_navigation(self):
        source=(ROOT/'main/main.c').read_text(encoding='utf-8')
        self.assertNotIn('enter_menu',source)
        self.assertIn('token_ui_create()',source)
if __name__=='__main__':unittest.main()
