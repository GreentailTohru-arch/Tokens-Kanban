"""Verify real source identity, conversion timing and anti-alias filtering."""
from pathlib import Path
import hashlib,math,sys,unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from convert_codex_sounds import ASSETS,read_mono,decimate,COMPRESSED

class SoundTests(unittest.TestCase):
    def test_supplied_sources_and_duration(self):
        hashes={'notification':'d3ec08b617dedc0d46de8da082e83fcd7526e129bbf6ce14354ee7f182a4e2ba','error':'fb993fdfef45312eaaae208019bc90c637b0b85bbe4f0a1741adda6978cecabc'}
        for name,digest in hashes.items():
            path=ASSETS/f'codex-{name}.wav';self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(),digest)
            source,info=read_mono(path);pcm=decimate(source)
            self.assertLess(abs(len(pcm)/16000-info['seconds']),1/16000)
            self.assertGreater(max(map(abs,pcm)),9000);self.assertLess(max(map(abs,pcm)),32768)
    def test_additional_supplied_banks(self):
        hashes={'opencode-complete.aac':'01ad688048bca68a31ae6d1134f0977de6d35eb51242c983c441aec672b91c5a','opencode-error.aac':'748f50a244b17a1a2f5020bc253adc1319f29c8b563b390e09627daa91b989c6','antigravityCascadeDone.mp3':'965965913e685e5dd7b484c39c78b2f2cb7b7cdb71a670f4bcbed8eba408e613','antigravityActionRequired.mp3':'7d8ae66c7ca962634860dcf901a69494dd6c7a425a8f11fd6f2775d3cafd3ed7'}
        for (bank,event),name in COMPRESSED.items():
            self.assertEqual(hashlib.sha256((ASSETS/name).read_bytes()).hexdigest(),hashes[name])
            source,info=read_mono(ASSETS/f'{bank}-{event}.wav');pcm=decimate(source,info['rate'])
            self.assertEqual(pcm,source);self.assertGreater(max(map(abs,pcm)),5000);self.assertLess(max(map(abs,pcm)),32768)
    def test_filter_preserves_signal_and_rejects_alias(self):
        dc=decimate([12000]*3000)[30:-30];self.assertTrue(all(abs(v-12000)<=1 for v in dc))
        def rms(hz):
            data=decimate([10000*math.sin(2*math.pi*hz*i/48000) for i in range(6000)])[30:-30]
            return math.sqrt(sum(v*v for v in data)/len(data))
        self.assertGreater(rms(1000),7000);self.assertLess(rms(12000),15)

if __name__=='__main__':unittest.main()
