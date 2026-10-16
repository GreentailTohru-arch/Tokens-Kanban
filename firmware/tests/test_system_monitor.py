from pathlib import Path
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'companion'))
from system_monitor import HEADER,PACKET_SIZE,HEADER2,decode_packet,encode_system

def packet(values=None,tick=1000,pid=42,version=1):
    values=[8,9,64,57,51,9910,1000] if values is None else values
    texts=['8%','9%','64%','57 C','51 C','9.68 KB/s','1000 B/s']
    return HEADER.pack(b'TMK13\0\0\0',version,PACKET_SIZE,pid,1,tick,*values,127)+b''.join(t.encode('utf-16-le').ljust(96,b'\0') for t in texts)

class MonitorTests(unittest.TestCase):
    def test_actual_units_and_display_text(self):
        s=decode_packet(packet(),1001,lambda pid:pid==42)
        self.assertEqual(s['values'],[8,9,64,570,510,9910,1000,-1,-1,-1,-1])
        self.assertEqual(s['hostGetterMatches'],127)
        self.assertEqual(s['trafficMonitorDisplay'][5],'9.68 KB/s')
    def test_stale_exit_future_version_and_size(self):
        for raw,now,valid in [(packet(),6001,True),(packet(),999,True),(packet(),1001,False),(packet(version=2),1001,True),(packet()[:-1],1001,True)]:
            self.assertIsNone(decode_packet(raw,now,lambda p:valid))
    def test_missing_sensors_are_unknown_and_real_zero_load_is_valid(self):
        s=decode_packet(packet([0,-1,0,-1,-1,0,0]),1001,lambda p:True)
        self.assertEqual(s['values'],[0,-1,0,-1,-1,0,0,-1,-1,-1,-1])
        self.assertIsNone(decode_packet(packet([8,8,64,0,51,0,0]),1001,lambda p:True))
        self.assertIsNone(decode_packet(packet([101,8,64,57,51,0,0]),1001,lambda p:True))
    def test_v2_expanded_packet_and_wire(self):
        values=[8,9,64,57,51,9910,1000,3380,12,1073741824,2147483648]
        raw=HEADER2.pack(b'TMK15\0\0\0',2,1180,42,1,1000,*values,2047)+b'\0'*(11*96)
        result=decode_packet(raw,1001,lambda p:True)
        self.assertEqual(result['values'][7:],[3380,12,1073741824,2147483648])
        self.assertEqual(result['hostGetterMatches'],2047)
        self.assertTrue(encode_system(1,result['values']).startswith('SM2 '))
        self.assertIsNone(decode_packet(raw,6001,lambda p:True))
    def test_wire_and_corruption_bounds(self):
        frame=encode_system(1,[8,9,64,570,510,9910,1000]);body,checksum=frame.split('*');actual=2166136261
        for b in body.encode():actual=((actual^b)*16777619)&0xffffffff
        self.assertEqual(checksum,f'{actual:08x}')
        for values in ([101]*7,[float('nan')]*7,[0]*6,[-2]*7):
            with self.assertRaises(ValueError):encode_system(1,values)

if __name__=='__main__':unittest.main()
