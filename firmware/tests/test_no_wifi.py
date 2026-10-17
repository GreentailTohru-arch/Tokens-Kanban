"""Guard against accidentally restoring the removed device networking feature."""
from pathlib import Path
import sys,unittest
from unittest.mock import patch
R=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(R/'companion'))
import codex_tokens
class RemovedWiFiTests(unittest.TestCase):
 def test_firmware_has_no_radio_or_server_dependency(self):
  config=(R/'sdkconfig.defaults').read_text(encoding='utf-8')
  self.assertIn('CONFIG_ESP_COEX_SW_COEXIST_ENABLE=n',config)
  cmake=(R/'main/CMakeLists.txt').read_text()
  for token in ['esp_wifi','esp_netif','esp_http_server','management_assets','network_http','token_wifi']:self.assertNotIn(token,cmake)
  main=(R/'main/main.c').read_text();self.assertNotIn('token_wifi',main)
 def test_removed_transport_cannot_be_selected(self):
  with patch.object(sys,'argv',['codex_tokens','--transport','wifi']):
   with self.assertRaises(SystemExit) as e:codex_tokens.main()
   self.assertEqual(e.exception.code,2)
 def test_no_device_wifi_settings_remain(self):
  html=(R/'companion/postcard/index.html').read_text(encoding='utf-8')
  for token in ['id="wifi"','wifiSSID','wifiPassword','wifiForm']:self.assertNotIn(token,html)
  self.assertFalse((R/'companion/token_wifi.py').exists())
if __name__=='__main__':unittest.main()
