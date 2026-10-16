"""Read only the TrafficMonitor bridge's explicit local Windows mailslot IPC."""
import ctypes
from ctypes import wintypes
from pathlib import Path
import struct
import threading
import time

SLOT = r"\\.\mailslot\TokensKanban-TrafficMonitor-v1"
HEADER = struct.Struct('<8sIIIIQ7qI')
HEADER2 = struct.Struct('<8sIIIIQ11qI')
PACKET_SIZE = 764
UNKNOWN = [-1] * 11
TTL_MS = 5000


def api():
    k = ctypes.WinDLL('kernel32', use_last_error=True)
    k.CreateMailslotW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD, ctypes.c_void_p]
    k.CreateMailslotW.restype = wintypes.HANDLE
    k.GetMailslotInfo.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.POINTER(wintypes.DWORD), ctypes.POINTER(wintypes.DWORD), ctypes.c_void_p]
    k.ReadFile.argtypes = [wintypes.HANDLE, ctypes.c_void_p, wintypes.DWORD, ctypes.POINTER(wintypes.DWORD), ctypes.c_void_p]
    k.CloseHandle.argtypes = [wintypes.HANDLE]
    k.GetTickCount64.restype = ctypes.c_uint64
    k.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    k.OpenProcess.restype = wintypes.HANDLE
    k.QueryFullProcessImageNameW.argtypes = [wintypes.HANDLE, wintypes.DWORD, wintypes.LPWSTR, ctypes.POINTER(wintypes.DWORD)]
    k.GetExitCodeProcess.argtypes = [wintypes.HANDLE, ctypes.POINTER(wintypes.DWORD)]
    return k


def process_is_trafficmonitor(pid):
    # Query image identity/liveness only, never PROCESS_VM_READ / ReadProcessMemory.
    k = api()
    handle = k.OpenProcess(0x1000, False, pid)
    if not handle:
        return False
    try:
        name = ctypes.create_unicode_buffer(32768)
        length, exit_code = wintypes.DWORD(len(name)), wintypes.DWORD()
        return (bool(k.QueryFullProcessImageNameW(handle, 0, name, ctypes.byref(length)))
                and Path(name.value).name.lower() == 'trafficmonitor.exe'
                and bool(k.GetExitCodeProcess(handle, ctypes.byref(exit_code))) and exit_code.value == 259)
    finally:
        k.CloseHandle(handle)


def decode_packet(raw, now_ms, valid_process):
    header=HEADER if len(raw)==764 else HEADER2 if len(raw)==1180 else None
    if header is None:return None
    count=7 if header is HEADER else 11
    magic, version, size, pid, sequence, tick, *tail = header.unpack_from(raw)
    values, matches = tail[:count], tail[count]
    if ((magic,version,size) != ((b'TMK13\0\0\0',1,764) if count==7 else (b'TMK15\0\0\0',2,1180)) or not pid or not sequence
            or tick > now_ms or now_ms - tick > TTL_MS or not valid_process(pid)):
        return None
    limits = [100, 100, 100, 150, 150, 10**12, 10**12,100000,100,2**53-1,2**53-1][:count]
    if any(v < -1 or v > limit or (i in (3,4) and v == 0) for i, (v,limit) in enumerate(zip(values,limits))):
        return None
    displays = []
    try:
        for i in range(count):
            text = raw[header.size + i*96:header.size + (i+1)*96].decode('utf-16-le').split('\0',1)[0]
            displays.append(text)
    except UnicodeDecodeError:
        return None
    # Host temperatures are whole Celsius; the device wire uses tenths Celsius.
    values = list(values)
    for i in (3,4):
        if values[i] >= 0:
            values[i] *= 10
    values.extend([-1]*(11-len(values)))
    return {'values':values, 'pid':pid, 'sequence':sequence, 'tick':tick,
            'hostGetterMatches':matches, 'trafficMonitorDisplay':displays}


class MailslotReader:
    def __init__(self, name=SLOT):
        self.k = api()
        self.handle = self.k.CreateMailslotW(name, 2048, 0, None)
        if self.handle == ctypes.c_void_p(-1).value:
            raise OSError(ctypes.get_last_error(), 'TrafficMonitor bridge listener unavailable; close any other v13 companion')
    def read(self):
        size, count = wintypes.DWORD(), wintypes.DWORD()
        if not self.k.GetMailslotInfo(self.handle, None, ctypes.byref(size), ctypes.byref(count), None):
            raise ctypes.WinError(ctypes.get_last_error())
        if not count.value:
            return None
        data = ctypes.create_string_buffer(2048)
        received = wintypes.DWORD()
        if not self.k.ReadFile(self.handle, data, len(data), ctypes.byref(received), None):
            raise ctypes.WinError(ctypes.get_last_error())
        return data.raw[:received.value]
    def close(self):
        if self.handle:
            self.k.CloseHandle(self.handle)
            self.handle = None


class SystemPoller:
    def __init__(self):
        self.lock = threading.Lock()
        self.stop = threading.Event()
        self.ready = threading.Event()
        self.data = None
        self.error = ''
        self.thread = threading.Thread(target=self._run, daemon=True)
        self.thread.start()
    def _run(self):
        reader = None
        try:
            reader = MailslotReader()
            self.ready.set()
            while not self.stop.is_set():
                for _ in range(32):
                    raw = reader.read()
                    if raw is None:
                        break
                    sample = decode_packet(raw, reader.k.GetTickCount64(), process_is_trafficmonitor)
                    if sample:
                        with self.lock:
                            if self.data is None or sample['pid'] != self.data['pid'] or sample['tick'] > self.data['tick']:
                                self.data = sample
                self.stop.wait(.1)
        except (OSError, AttributeError) as error:
            self.error = str(error)
            self.ready.set()
        finally:
            if reader:
                reader.close()
    def diagnostics(self):
        with self.lock:
            sample = dict(self.data) if self.data else None
        if sample and (api().GetTickCount64()-sample['tick']>TTL_MS or not process_is_trafficmonitor(sample['pid'])):
            sample = None
        return sample
    def values(self):
        sample = self.diagnostics()
        return sample['values'].copy() if sample else UNKNOWN.copy()
    def close(self):
        self.stop.set()
        self.thread.join(timeout=2)


def encode_system(sequence, values):
    if len(values) not in (7,11) or not 1 <= sequence <= 0xffffffff:
        raise ValueError('Invalid system snapshot')
    limits = [100, 100, 100, 1500, 1500, 10**12, 10**12,100000,100,2**53-1,2**53-1][:len(values)]
    if any(type(v) is not int or v < -1 or v > limit for v, limit in zip(values, limits)):
        raise ValueError('System metric outside limits')
    body = (('SM1 ' if len(values)==7 else 'SM2 ') + ' '.join(map(str, [sequence, *values]))).encode('ascii')
    checksum = 2166136261
    for byte in body:
        checksum = ((checksum ^ byte) * 16777619) & 0xffffffff
    return (body + f'*{checksum:08x}'.encode()).decode()
