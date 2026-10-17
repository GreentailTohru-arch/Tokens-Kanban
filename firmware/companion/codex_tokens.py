"""Read-only Codex statistics to AI Passport over USB or BLE. No inference or flashing."""
from __future__ import annotations
import argparse
import datetime as dt
import hashlib
import json
import math
import os
from pathlib import Path
import queue
import shutil
import subprocess
import threading
import time
from token_pricing import PRICE_DATE, request_nano_usd
from token_analytics import summarize
from token_events import CompletionTracker,supports_events
from system_monitor import SystemPoller,encode_system

MAX_VALUE = 10**15
CREATE_FLAGS = getattr(subprocess, 'CREATE_NO_WINDOW', 0)

def integer(value, default=-1):
    return value if isinstance(value, int) and not isinstance(value, bool) and 0 <= value <= MAX_VALUE else default

class UsageIndex:
    """Tail local/archived rollouts; keep only counters, dates and dedup hashes."""
    def __init__(self, home: Path):
        self.home = home
        self.files = {}

    def refresh(self):
        candidates = set()
        for root in ('sessions', 'archived_sessions'):
            candidates.update((self.home / root).rglob('*.jsonl'))
        self.files = {p: s for p, s in self.files.items() if p in candidates}
        for path in sorted(candidates):
            try:
                stat = path.stat()
                state = self.files.get(path)
                if state is None or stat.st_size < state['offset'] or (stat.st_size == state['size'] and stat.st_mtime_ns != state['mtime']):
                    state = dict(offset=0, previous=None, records=[], size=-1, mtime=-1,
                                 active=None, turns={}, completions={}, readable=True, model=None)
                    self.files[path] = state
                state['readable'] = True
                if stat.st_size == state['size'] and stat.st_mtime_ns == state['mtime']:
                    continue
                with path.open('rb') as stream:
                    stream.seek(state['offset'])
                    while True:
                        start = stream.tell()
                        line = stream.readline()
                        if not line or not line.endswith(b'\n'):
                            state['offset'] = start
                            break
                        state['offset'] = stream.tell()
                        if len(line) > 8*1024*1024 or (b'"event_msg"' not in line and b'"turn_context"' not in line):
                            continue
                        try:
                            record = json.loads(line)
                            payload = record.get('payload') or {}
                            if record.get('type') == 'turn_context':
                                state['model'] = payload.get('model')
                                continue
                            if record.get('type') != 'event_msg':
                                continue
                            self._activity_event(state,record,payload)
                            if payload.get('type') != 'token_count':
                                continue
                            info = payload.get('info') or {}
                            total = info.get('total_token_usage') or {}
                            current = tuple(integer(total.get(k)) for k in ('input_tokens','cached_input_tokens','output_tokens'))
                            if -1 in current or current[1] > current[0]:
                                continue
                            previous = state['previous']
                            state['previous'] = current
                            if previous == current:
                                continue  # quota-only duplicate event
                            timestamp = record['timestamp']
                            when = dt.datetime.fromisoformat(timestamp.replace('Z','+00:00'))
                            if when.tzinfo is None:
                                continue
                            last = info.get('last_token_usage') or {}
                            latest = tuple(integer(last.get(k)) for k in ('input_tokens','cached_input_tokens','output_tokens'))
                            if previous is None or any(a < b for a,b in zip(current,previous)):
                                # First visible record may have inherited/truncated history.
                                delta = latest if -1 not in latest else current
                            else:
                                delta = tuple(a-b for a,b in zip(current,previous))
                            if delta[1] > delta[0]:
                                continue
                            identity = hashlib.sha256(json.dumps([timestamp,current,delta]).encode()).digest()
                            # Only price a delta attributable to one identified request.
                            # Gaps/aggregated deltas lack the request context needed for tiering.
                            cost = request_nano_usd(state['model'],last) if delta == latest else None
                            state['records'].append((identity,when,delta,cost,state['model']))
                        except (ValueError, KeyError, TypeError, AttributeError):
                            continue
                state['size'], state['mtime'] = stat.st_size, stat.st_mtime_ns
            except OSError:
                if path in self.files: self.files[path]['readable'] = False
                continue

    @staticmethod
    def _activity_event(state,record,payload):
        """Retain lifecycle metadata only; no messages/tool arguments are saved."""
        stamp=dt.datetime.fromisoformat(record['timestamp'].replace('Z','+00:00'))
        if stamp.tzinfo is None: return
        stamp=stamp.timestamp(); kind=payload.get('type')
        turn=payload.get('turn_id')
        if kind=='task_started' and isinstance(turn,str):
            state['model']=None  # A new turn must identify its own model.
            state['active']=turn; state['turns'][turn]=(2,stamp)
        elif kind in ('task_complete','turn_aborted'):
            turn=turn or state['active']
            if isinstance(turn,str):
                state['turns'][turn]=(1,stamp)
                if kind=='task_complete':state['completions'][turn]=stamp
            if turn==state['active']: state['active']=None
        elif state['active']:
            state['turns'][state['active']]=(2,stamp)

    def completions(self):
        result=set()
        for path,state in self.files.items():
            if state['readable'] and 'archived_sessions' not in path.relative_to(self.home).parts:
                result.update(state['completions'].items())
        return result

    def activity(self, now=None):
        """0 unknown; 1 idle; 2 active. Stale unfinished turns are not idle."""
        now=time.time() if now is None else now
        turns={}
        for path,state in self.files.items():
            if not state['readable'] or 'archived_sessions' in path.relative_to(self.home).parts: continue
            for turn,value in state['turns'].items():
                if value[1]>now+5: continue
                if turn not in turns or value[1]>turns[turn][1] or (value[1]==turns[turn][1] and value[0]==1):
                    turns[turn]=value
        if any(status==2 and now-stamp<900 for status,stamp in turns.values()): return 2
        if not turns: return 0
        latest=max(turns.values(),key=lambda v:v[1])
        return 1 if latest[0]==1 else 0

    def totals(self, now=None):
        now = now or dt.datetime.now().astimezone()
        today = now.date()
        monday = today - dt.timedelta(days=today.weekday())
        sums = [[0,0,0] for _ in range(3)]
        seen = set()
        for state in self.files.values():
            for key, when, delta, _, _ in state['records']:
                if key in seen or when > now:
                    continue
                seen.add(key)
                day = when.astimezone(now.tzinfo).date()
                for i, include in enumerate((day==today, monday<=day<=today, True)):
                    if include:
                        sums[i] = [a+b for a,b in zip(sums[i],delta)]
        # No records means unavailable, not a measured zero.
        return sums if seen else [[-1,-1,-1] for _ in range(3)]

    def cost_summary(self, now=None):
        now = now or dt.datetime.now().astimezone()
        today = now.date(); monday = today - dt.timedelta(days=today.weekday())
        unique = {}
        for state in self.files.values():
            for key, when, delta, cost, _ in state['records']:
                if when <= now and (key not in unique or unique[key][2] is None):
                    unique[key] = (when, delta, cost)
        sums = [0,0,0]; priced = [0,0,0]; missing = [0,0,0]
        for when, delta, cost in unique.values():
            day = when.astimezone(now.tzinfo).date()
            for i, include in enumerate((day == today, monday <= day <= today, True)):
                if include and delta[0] + delta[2] > 0:
                    if cost is None: missing[i] += 1
                    else: sums[i] += cost; priced[i] += 1
        result = []
        for i in range(3):
            # A measured empty day is zero; no readable history is unavailable.
            status = (2 if missing[i] else 1) if unique and (priced[i] or not missing[i]) else 0
            cents = (sums[i] + 5_000_000) // 10_000_000 if status else -1
            if cents > MAX_VALUE: cents, status = -1, 0
            result.append(dict(cents=cents,status=status,pricedRecords=priced[i],missingRecords=missing[i]))
        return result

    def costs(self, now=None):
        return [[p['cents'],p['status']] for p in self.cost_summary(now)]

    def analytics(self, now=None):
        return summarize((record for state in self.files.values() for record in state['records']),
                         now or dt.datetime.now().astimezone())

def find_codex():
    executable = shutil.which('codex')
    if executable:
        return executable
    root = Path(os.environ.get('LOCALAPPDATA','')) / 'OpenAI/Codex/bin'
    found = list(root.glob('*/codex.exe'))
    return str(max(found,key=lambda p:p.stat().st_mtime)) if found else None

class CodexRPC:
    def __init__(self, executable=None):
        executable = executable or find_codex()
        if not executable:
            raise RuntimeError('Codex executable not found')
        self.proc = subprocess.Popen([executable,'app-server','--listen','stdio://'],stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,stderr=subprocess.DEVNULL,text=True,encoding='utf-8',creationflags=CREATE_FLAGS)
        self.responses = queue.Queue()
        self.counter = 0
        threading.Thread(target=self._reader,daemon=True).start()
        try:
            self.call('initialize',{'clientInfo':{'name':'passport_tokens','title':'AI Passport Token Display','version':'1.0.0'}})
            self.proc.stdin.write('{"method":"initialized"}\n'); self.proc.stdin.flush()
        except Exception:
            self.close(); raise

    def _reader(self):
        for line in self.proc.stdout:
            try:
                item = json.loads(line)
                if 'id' in item and ('result' in item or 'error' in item):
                    self.responses.put(item)
            except ValueError:
                pass
        self.responses.put(None)

    def call(self, method, params=None, timeout=25):
        self.counter += 1
        self.proc.stdin.write(json.dumps({'id':self.counter,'method':method,'params':params or {}})+'\n')
        self.proc.stdin.flush()
        deadline = time.monotonic()+timeout
        while True:
            item = self.responses.get(timeout=max(.01,deadline-time.monotonic()))
            if item is None:
                raise RuntimeError('Codex server closed')
            if item.get('id') != self.counter:
                continue
            if 'error' in item:
                raise RuntimeError('Codex statistics unavailable')
            return item['result']

    def close(self):
        if self.proc.poll() is None:
            self.proc.terminate()
            try: self.proc.wait(timeout=5)
            except subprocess.TimeoutExpired: self.proc.kill(); self.proc.wait(timeout=5)
        for stream in (self.proc.stdin,self.proc.stdout):
            if stream: stream.close()

def quota_values(result, now):
    buckets = result.get('rateLimitsByLimitId') or {}
    bucket = buckets.get('codex') or result.get('rateLimits') or {}
    values=[]
    for name in ('primary','secondary'):
        window = bucket.get(name) or {}
        used = window.get('usedPercent')
        duration = integer(window.get('windowDurationMins'))
        reset = integer(window.get('resetsAt'))
        if not isinstance(used,(int,float)) or not math.isfinite(used) or duration<=0:
            values.extend([-1,-1,-1])
        else:
            # Do not round 99.5% up into a false exhausted/red state.
            values.extend([math.floor(max(0,min(100,used))),min(525600,duration),min(31536000,max(0,reset-int(now))) if reset>=0 else -1])
    credits=integer((result.get('rateLimitResetCredits') or {}).get('availableCount'))
    plans={'free':1,'plus':2,'pro':3,'prolite':3,'team':4,'business':4,'enterprise':5}
    return values+[credits,plans.get(bucket.get('planType'),0)]

class QuotaPoller:
    def __init__(self):
        self.result=None; self.updated=0.; self.lock=threading.Lock(); self.stop=threading.Event()
        self.ready=threading.Event()
        self.thread=threading.Thread(target=self._run,daemon=True); self.thread.start()
    def _run(self):
        rpc=None
        try:
            while not self.stop.is_set():
                try:
                    rpc=rpc or CodexRPC()
                    result=rpc.call('account/rateLimits/read')
                    # Store only the allowed statistics, never raw auth/profile fields.
                    now=time.time()
                    values=quota_values(result,now)
                    with self.lock: self.result,self.updated=values,now
                except (OSError,RuntimeError,queue.Empty,ValueError):
                    if rpc: rpc.close(); rpc=None
                self.ready.set()
                self.stop.wait(60)
        finally:
            if rpc: rpc.close()
    def values(self):
        with self.lock:
            if self.result is None: return [-1]*7+[0,-1]
            age=max(0,int(time.time()-self.updated)); values=list(self.result)
        for i in (2,5):
            if values[i]>=0: values[i]=max(0,values[i]-age)
        return values+[age]
    def close(self):
        self.stop.set(); self.thread.join(timeout=32)

def encode_frame(sequence, totals, quota, activity=None, costs=None, analytics=None):
    values=[sequence]+[v for row in totals for v in row]+quota
    if len(values)!=19 or any(not isinstance(v,int) or not -1<=v<=MAX_VALUE for v in values):
        raise ValueError('Invalid snapshot')
    prefix='CT1 '
    if activity is not None:
        if type(activity) is not int or activity not in (0,1,2): raise ValueError('Invalid activity')
        values.append(activity);prefix='CT2 '
    if costs is not None:
        if activity is None or len(costs) != 3: raise ValueError('Invalid estimates')
        for pair in costs:
            if len(pair) != 2: raise ValueError('Invalid estimate')
            cents, status = pair
            if type(cents) is not int or type(status) is not int or status not in (0,1,2):
                raise ValueError('Invalid estimate')
            if (status == 0 and cents != -1) or (status != 0 and not 0 <= cents <= MAX_VALUE):
                raise ValueError('Invalid estimate amount')
            values.extend(pair)
        prefix='CT3 '
    suffix=''
    if analytics is not None:
        if costs is None or len(analytics['fields'])!=21: raise ValueError('Invalid analytics')
        if any(type(v) is not int or not -1<=v<=MAX_VALUE for v in analytics['fields']):
            raise ValueError('Invalid analytics values')
        heat=analytics['heat'];trend=analytics['trend']
        if len(heat)!=182 or any(c not in '012345' for c in heat): raise ValueError('Invalid heatmap')
        if len(trend)!=180 or any(c not in '0123456789abcdef' for c in trend): raise ValueError('Invalid trend')
        if any(int(trend[i:i+3],16)>1000 for i in range(0,180,3)): raise ValueError('Invalid trend point')
        values.extend(analytics['fields']);prefix='CT4 ';suffix=' '+heat+' '+trend
    body=(prefix+' '.join(map(str,values))+suffix).encode('ascii')
    if len(body)+9>=1024: raise ValueError('Snapshot exceeds firmware frame size')
    checksum=2166136261
    for byte in body: checksum=((checksum^byte)*16777619)&0xffffffff
    return body+f'*{checksum:08x}\n'.encode('ascii')

def candidates(explicit=None):
    from serial.tools import list_ports
    if explicit: return [explicit]
    return [p.device for p in list_ports.comports() if p.vid==0x303A and p.pid==0x1001]

def open_passport(port, version=4):
    import serial
    connection=serial.Serial(port=None,baudrate=115200,timeout=.25,write_timeout=1)
    connection.dtr=False; connection.rts=False; connection.port=port
    try:
        connection.open(); connection.reset_input_buffer()
        for _ in range(3):
            connection.write(f'CT_HELLO {version}\n'.encode() if version>=2 else b'CT_HELLO\n')
            deadline=time.monotonic()+1
            while time.monotonic()<deadline:
                if connection.readline(256).strip()==f'CT_READY {version}'.encode(): return connection
        raise OSError('Port does not identify as Codex Token Display')
    except Exception:
        connection.close(); raise

def send_snapshot(connection, frame, sequence):
    connection.write(frame)
    deadline=time.monotonic()+2
    expected=f'CT_ACK {sequence}'.encode()
    while time.monotonic()<deadline:
        if connection.readline(256).strip()==expected: return
    raise OSError('Device acknowledgement timeout')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port',help='Explicit Passport COM port; otherwise only Espressif 303A:1001')
    parser.add_argument('--transport',choices=('auto','usb','wifi','ble'),default='auto',help='Auto prefers USB, then paired Wi-Fi, then BLE')
    parser.add_argument('--wifi-host',help='Paired device IPv4 address')
    parser.add_argument('--no-browser',action='store_true',help='Disable opening the management page on connection')
    parser.add_argument('--ble-address',help='BLE address from --list-ble (required for multiple devices)')
    parser.add_argument('--codex-home',type=Path,default=Path(os.environ.get('CODEX_HOME',Path.home()/'.codex')))
    parser.add_argument('--system-only',action='store_true',help='Print local PC metrics once without connecting or reading Codex logs')
    parser.add_argument('--dry-run',action='store_true',help='Read statistics once; never open a serial port')
    parser.add_argument('--list-ports',action='store_true')
    parser.add_argument('--postcard-editor',action='store_true',help='Open the local browser postcard editor')
    parser.add_argument('--editor-no-browser',action='store_true',help='Serve the editor without opening a browser (automation)')
    parser.add_argument('--list-ble',action='store_true',help='Discover matching BLE service; do not connect')
    args=parser.parse_args()
    if args.postcard_editor:
        from management_server import existing_manager
        if not args.editor_no_browser and existing_manager():return
        from postcard_server import serve
        serve(open_browser=not args.editor_no_browser); return
    if args.list_ports:
        print('\n'.join(candidates()) or 'No Espressif USB device detected'); return
    if args.list_ble:
        import asyncio
        from token_ble import discover
        for device in asyncio.run(discover()): print(device.address,device.name or 'Codex Passport')
        return
    if args.port and args.transport not in ('auto','usb'): parser.error('--port requires auto or usb transport')
    if args.system_only:
        monitor=SystemPoller()
        try:
            monitor.ready.wait(3)
            deadline=time.monotonic()+8
            while monitor.diagnostics() is None and time.monotonic()<deadline and not monitor.error:time.sleep(.1)
            values=monitor.values()
            print(json.dumps({'system':dict(zip(('cpuPercent','gpuPercent','ramPercent','cpuTempDeciC','gpuTempDeciC','downloadBytesPerSecond','uploadBytesPerSecond','cpuFrequencyMHz','diskPercent','todayUploadBytes','todayDownloadBytes'),values)),
                'trafficMonitor':monitor.diagnostics(),'listenerError':monitor.error,'frame':encode_system(1,values)},ensure_ascii=True))
        finally:monitor.close()
        return
    from management_server import existing_manager
    if not args.dry_run and existing_manager(open_browser=not args.no_browser):
        print('Tokens Kanban is already running; reusing its management page.',flush=True);return
    monitor=SystemPoller()
    index=UsageIndex(args.codex_home); quota=QuotaPoller(); connection=None; sequence=0; events=CompletionTracker(); can_notify=False; can_monitor=False
    management=None
    print('Tokens Kanban v19 | USB + Wi-Fi + Bluetooth LE\nTokens / estimated API USD / models / activity / trends / PC system monitor. Ctrl+C to exit.',flush=True)
    try:
        if args.dry_run:
            index.refresh(); quota.ready.wait(30)
            totals=index.totals(); values=quota.values()
            print(json.dumps({'localDay':totals[0],'localWeek':totals[1],'localTotal':totals[2],
                'quota':values,'activity':index.activity(),'priceDate':PRICE_DATE,'estimates':index.cost_summary(),
                'analytics':index.analytics(),'system':monitor.values(),
                'frame':encode_frame(1,totals,values,index.activity(),index.costs(),index.analytics()).decode().strip()},ensure_ascii=True)); return
        from management_server import LinkManager,ManagementServer
        from token_wifi import WiFiConnection,remember
        manager=LinkManager();management=ManagementServer(manager);last_pair=0
        last_status=''
        while True:
            index.refresh(); events.observe(index.completions()); sequence+=1
            now=dt.datetime.now().astimezone()
            frame=encode_frame(sequence,index.totals(now),quota.values(),index.activity(),index.costs(now),index.analytics(now))
            try:
                with manager.lock:
                    if connection is None:
                        ports=candidates(args.port) if args.transport in ('auto','usb') else []
                        if len(ports)>1:raise OSError('Multiple USB devices; select --port COMx')
                        if ports:connection=open_passport(ports[0])
                        elif args.transport=='usb' or args.port:raise OSError('Connect Passport via USB')
                        else:
                            if args.transport in ('auto','wifi'):
                                try:connection=WiFiConnection.open(args.wifi_host)
                                except OSError:
                                    if args.transport=='wifi':raise
                            if connection is None:
                                from token_ble import BLEConnection
                                connection=BLEConnection.open(args.ble_address)
                        manager.attach(connection)
                        can_notify=supports_events(connection)
                        from token_events import rpc as device_rpc
                        capability=device_rpc(connection,'PC_SYS_HELLO2')
                        system_version=2 if capability=='PC_SYS_READY 2' else 1
                        can_monitor=system_version==2 or device_rpc(connection,'PC_SYS_HELLO')=='PC_SYS_READY 1'
                        last_pair=0
                    send_snapshot(connection,frame,sequence)
                    from token_events import rpc as device_rpc
                    device_rpc(connection,f'PC_CLOCK {int(now.replace(tzinfo=dt.timezone.utc).timestamp())}')
                    if can_monitor:
                        reply=device_rpc(connection,encode_system(sequence%0xffffffff or 1,monitor.values() if system_version==2 else monitor.values()[:7]))
                        if reply!=f'PC_SYS_ACK {sequence%0xffffffff or 1}':raise OSError('System snapshot not acknowledged')
                    if can_notify:events.send(connection)
                    else:events.pending=None
                    if time.monotonic()-last_pair>=30:
                        try:remember(connection)
                        except OSError:print('Wi-Fi pairing cache unavailable; current data connection continues.',flush=True)
                        last_pair=time.monotonic()
                    status='Connected: '+connection.port+' | syncing every 5 seconds'
                if not args.no_browser and status!=last_status:management.open()
            except OSError as error:
                with manager.lock:
                    manager.detach()
                    if connection:connection.close();connection=None
                status=str(error)
            if status!=last_status: print(status,flush=True); last_status=status
            time.sleep(5)
    except KeyboardInterrupt:
        pass
    finally:
        if management:management.close()
        if connection: connection.close()
        quota.close(); monitor.close()

if __name__=='__main__': main()
