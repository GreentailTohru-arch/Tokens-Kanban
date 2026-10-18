"""Original GIFs and delivered sprite frames must stay identical, including alpha."""
from pathlib import Path
import hashlib,json,unittest
from PIL import Image
R=Path(__file__).resolve().parents[1]
class AssetTest(unittest.TestCase):
    def test_original_frames_and_routes(self):
        import sys
        sys.path.insert(0,str(R/'companion'))
        from postcard_server import ASSETS
        meta=json.loads((R/'assets/images/codex-pet/manifest.json').read_text(encoding='utf-8'))
        self.assertEqual(len(meta),6)
        for a in meta:
            original=R/'assets/images/codex-pet'/(a['key']+'.gif')
            self.assertEqual(hashlib.sha256(original.read_bytes()).hexdigest(),a['sha256'])
            gif=Image.open(original);sheet=Image.open(R/'companion/postcard/pet'/(a['key']+'.png'))
            self.assertEqual(gif.n_frames,len(a['durations']))
            self.assertIn('/pet/'+a['key']+'.png',ASSETS)
            for i in range(gif.n_frames):
                gif.seek(i);frame=gif.convert('RGBA')
                self.assertEqual(frame.tobytes(),sheet.crop((192*i,0,192*(i+1),208)).tobytes())
                self.assertEqual(gif.info['duration'],a['durations'][i])
                self.assertEqual(frame.getpixel((0,0))[3],0)
            gif.close();sheet.close()
if __name__=='__main__':unittest.main()
