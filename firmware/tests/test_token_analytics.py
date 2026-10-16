import datetime as dt
from pathlib import Path
import sys,unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'companion'))
from token_analytics import summarize
from codex_tokens import encode_frame

class AnalyticsTests(unittest.TestCase):
    def test_calendar_forks_models_and_future(self):
        zone=dt.timezone(dt.timedelta(hours=8));now=dt.datetime(2026,10,16,12,tzinfo=zone)
        a=('a',dt.datetime(2026,10,15,15,59,tzinfo=dt.timezone.utc),(100,50,10),None,'gpt-6-astra')
        b=('b',dt.datetime(2026,10,15,16,1,tzinfo=dt.timezone.utc),(200,100,20),None,'gpt-6.1-sol')
        c=('c',now+dt.timedelta(days=1),(999,0,1),None,'gpt-6-astra')
        r=summarize([a,b,a,b,c],now)
        self.assertEqual(r['fields'][:12],[2,220,0,-1,2,220,1,110,2,220,1,110])
        self.assertEqual(r['fields'][12],2)
        self.assertEqual(r['fields'][16:19],[818,916,1016])
        self.assertEqual(r['fields'][19:],[220,1])
        self.assertTrue(r['heat'].endswith('4455'))
        self.assertEqual(r['trend'][-6:],'1f43e8')
    def test_missing_empty_day_and_unknown_model(self):
        now=dt.datetime(2026,10,16,tzinfo=dt.timezone.utc)
        r=summarize([],now);self.assertEqual(r['fields'][-1],0)
        self.assertEqual(r['trend'],'000'*60)
        a=('a',now-dt.timedelta(days=1),(10,0,5),None,'unlisted-model')
        r=summarize([a],now)
        self.assertEqual(r['fields'][:4],[0,-1,0,-1])
        self.assertEqual(r['fields'][4:8],[0,15,0,-1])
        self.assertEqual(r['fields'][-1],1)
    def test_model_metadata_copy_and_monday(self):
        now=dt.datetime(2026,10,19,tzinfo=dt.timezone.utc)
        a=('a',now-dt.timedelta(days=1),(10,0,5),None,None)
        b=(*a[:-1],'gpt-6-sol')
        r=summarize([a,b],now)
        self.assertEqual(r['fields'][4:8],[0,-1,0,-1])
        self.assertEqual(r['fields'][8:12],[3,15,0,-1])
        self.assertTrue(r['heat'].endswith('0555555'))
    def test_ct4_bounded_packet_and_bad_graphs(self):
        now=dt.datetime(2026,10,16,tzinfo=dt.timezone.utc)
        r=summarize([('a',now,(10**15,0,10**15),None,'gpt-6-astra')],now)
        # Per-field protocol range; total analytics model count must fit the same bound.
        r['fields'][1]=r['fields'][5]=r['fields'][9]=10**15;r['fields'][19]=10**15
        args=(10**15,[[10**15,10**15,10**15]]*3,[100,525600,31536000]*2+[10**15,5,10**15],2,[[10**15,1]]*3)
        packet=encode_frame(*args,r)
        self.assertTrue(packet.startswith(b'CT4 '));self.assertLessEqual(len(packet),1024)
        bad=dict(r,trend='fff'*60)
        with self.assertRaises(ValueError):encode_frame(*args,bad)

if __name__=='__main__':unittest.main()
