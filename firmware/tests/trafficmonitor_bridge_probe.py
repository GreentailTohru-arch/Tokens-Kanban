"""Windows-only real DLL ABI/IPC probe with an isolated mock host."""
import ctypes as c
import json
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'companion'))
from system_monitor import MailslotReader,decode_packet
reader=MailslotReader()
dll=c.WinDLL(str(Path(sys.argv[1]).resolve()))
dll.TMPluginGetInstance.restype=c.c_void_p
obj=dll.TMPluginGetInstance()
vtable=c.cast(obj,c.POINTER(c.POINTER(c.c_void_p))).contents
def method(index,result,*args):return c.WINFUNCTYPE(result,c.c_void_p,*args)(vtable[index])
assert method(0,c.c_int)(obj)==7
for index in (0,-1,999):assert not method(1,c.c_void_p,c.c_int)(obj,index)
assert method(4,c.c_wchar_p,c.c_int)(obj,0)=='Tokens Kanban Bridge'
version=c.create_unicode_buffer('1.86')
texts=[c.create_unicode_buffer(t) for t in ['8%','9%','64%','57 C','51 C','9.68 KB/s','1000 B/s','3.38GHz','12%','1.00GB','2.00GB']]
raw=[8,9,64,57,51,9910,1000,3380,12,1073741824,2147483648]
items={2:0,4:1,3:2,5:3,6:4,1:5,0:6,10:7,9:8,11:9,12:10}
callbacks=[c.WINFUNCTYPE(c.c_int,c.c_void_p)(lambda _:0),c.WINFUNCTYPE(c.c_void_p,c.c_void_p)(lambda _:c.addressof(version)),c.WINFUNCTYPE(c.c_double,c.c_void_p,c.c_int)(lambda _,item:float(raw[items[item]])/(1000 if item==10 else 1)),c.WINFUNCTYPE(c.c_void_p,c.c_void_p,c.c_int,c.c_int)(lambda _,item,main:c.addressof(texts[items[item]]))]
host_table=(c.c_void_p*len(callbacks))(*(c.cast(cb,c.c_void_p).value for cb in callbacks))
host=(c.c_void_p*1)(c.addressof(host_table))
method(14,None,c.c_void_p)(obj,c.cast(host,c.c_void_p))
class Monitor(c.Structure):
    _fields_=[('up',c.c_uint64),('down',c.c_uint64)]+[(name,c.c_int) for name in ('cpu','ram','gpu','hdd','cpu_temp','gpu_temp','hdd_temp','board_temp','cpu_freq')]
assert c.sizeof(Monitor)==56
update=method(5,None,c.POINTER(Monitor))
m=Monitor(1000,9910,8,64,9,12,57,51,-1,-1,3)
update(obj,c.byref(m))
sample=decode_packet(reader.read(),reader.k.GetTickCount64(),lambda _:True)
assert sample['values']==[8,9,64,570,510,9910,1000,3380,12,1073741824,2147483648] and sample['hostGetterMatches']==2047
reader.close();update(obj,c.byref(m));reader=MailslotReader()
m.cpu_temp=0;m.gpu_temp=-1;m.gpu=-1;m.up=m.down=0
raw[:7]=[8,-1,64,0,-1,0,0];update(obj,c.byref(m))
sample=decode_packet(reader.read(),reader.k.GetTickCount64(),lambda _:True)
assert sample['values'][:7]==[8,-1,64,-1,-1,0,0]
version.value='1.87';method(14,None,c.c_void_p)(obj,c.cast(host,c.c_void_p));update(obj,c.byref(m));assert reader.read() is None
reader.close()
print(json.dumps({'DLL':'PASS','API':'V1.86 / 7','x64VirtualABI':'PASS','actualLocalIPC':'PASS','receiverReconnect':'PASS','missingSensors':'PASS','unsupportedHostGating':'PASS','realTrafficMonitor':'NOT RUN'}))
