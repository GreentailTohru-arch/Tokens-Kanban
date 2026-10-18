"""Device input is ephemeral: never replay after reconnection or focus changes."""
import secrets
import time


def shortcut_keys(path=None, setting="dictation_hotkey", default="Ctrl+Alt+F9"):
    """Local preference survives package upgrades; default matches initial setup."""
    import os,json
    from pathlib import Path
    path=Path(path) if path is not None else Path(os.environ.get('LOCALAPPDATA',Path.home()))/'TokensKanban'/'pet-controls.json'
    try:
        value=json.loads(path.read_text(encoding='utf-8')).get(setting,default) if path.exists() else default
        parts=[p.strip().upper() for p in value.split('+')]
        modifiers={'CTRL':0xA2,'ALT':0xA4,'SHIFT':0xA0}
        if len(parts)<2 or len(parts)>4 or len(set(parts))!=len(parts) or any(p not in modifiers for p in parts[:-1]):raise ValueError()
        last=parts[-1]
        if len(last)==1 and 'A'<=last<='Z':code=ord(last)
        elif last.startswith('F') and last[1:].isdigit() and 1<=int(last[1:])<=24:code=0x70+int(last[1:])-1
        else:raise ValueError()
        return tuple(modifiers[p] for p in parts[:-1])+(code,)
    except (ValueError,AttributeError,OSError) as error:
        raise OSError('快捷键配置无效，请检查 TokensKanban/pet-controls.json') from error


def dictation_keys(path=None):
    return shortcut_keys(path)

def custom_shortcut_path():
    import os
    from pathlib import Path
    return Path(os.environ.get('LOCALAPPDATA',Path.home()))/'TokensKanban'/'pet-controls.json'


def parse_custom_shortcut(value):
    if not isinstance(value,str) or len(value)>80:raise ValueError('请输入单键或组合键')
    if not value.strip():return ()
    names={'CTRL':0xA2,'ALT':0xA4,'SHIFT':0xA0,'WIN':0x5B,'LEFTALT':0xA4,'RIGHTALT':0xA5,
           'ENTER':0x0D,'SPACE':0x20,'TAB':0x09,'ESC':0x1B,'BACKSPACE':0x08,'DELETE':0x2E,
           'UP':0x26,'DOWN':0x28,'LEFT':0x25,'RIGHT':0x27,'HOME':0x24,'END':0x23,'PAGEUP':0x21,'PAGEDOWN':0x22,
           'PLUS':0xBB,'MINUS':0xBD,'EQUAL':0xBB,'COMMA':0xBC,'PERIOD':0xBE,'SLASH':0xBF,
           'SEMICOLON':0xBA,'QUOTE':0xDE,'BRACKETLEFT':0xDB,'BRACKETRIGHT':0xDD,'BACKSLASH':0xDC,'BACKQUOTE':0xC0,
           'PRINTSCREEN':0x2C,'PAUSE':0x13,'CONTEXTMENU':0x5D,'INSERT':0x2D,'CAPSLOCK':0x14,'NUMLOCK':0x90,'SCROLLLOCK':0x91,
           'NUMPADADD':0x6B,'NUMPADSUBTRACT':0x6D,'NUMPADMULTIPLY':0x6A,'NUMPADDIVIDE':0x6F,'NUMPADDECIMAL':0x6E,
           **{'NUMPAD'+str(n):0x60+n for n in range(10)}}
    modifiers={0xA2,0xA4,0xA0,0x5B,0xA5}
    keys=[]
    for part in value.upper().split('+'):
        part=part.strip()
        if part in names:code=names[part]
        elif len(part)==1 and part.isascii() and part.isalnum():code=ord(part)
        elif part.startswith('F') and part[1:].isdigit() and 1<=int(part[1:])<=24:code=0x6F+int(part[1:])
        else:raise ValueError('不支持的按键：'+part)
        keys.append(code)
    if not 1<=len(keys)<=4 or len(set(keys))!=len(keys) or any(k not in modifiers for k in keys[:-1]):
        raise ValueError('组合键格式示例：Ctrl+Shift+A')
    return tuple(keys)


def custom_shortcut(value=None):
    import json,os
    path=custom_shortcut_path()
    data=json.loads(path.read_text(encoding='utf-8')) if path.exists() else {}
    if value is not None:
        parse_custom_shortcut(value)
        data['custom_hotkey']=value.strip()
        path.parent.mkdir(parents=True,exist_ok=True)
        temp=path.with_suffix('.tmp')
        temp.write_text(json.dumps(data,ensure_ascii=False),encoding='utf-8');os.replace(temp,path)
    return data.get('custom_hotkey','')


def screenshot_keys(path=None):
    # Legacy action 4 now invokes the explicitly configured shortcut only.
    return parse_custom_shortcut(custom_shortcut())


def open_accessible_codex():
    """Explicit launcher action; never terminates an existing Codex session."""
    import sys,subprocess
    if sys.platform!='win32':raise OSError('此兼容启动项目前仅支持 Windows')
    script=r'''
$ErrorActionPreference='Stop'
if(Get-Process ChatGPT -ErrorAction SilentlyContinue){exit 2}
$package=Get-AppxPackage -Name OpenAI.Codex | Select-Object -First 1
if(-not $package){exit 3}
$exe=Join-Path $package.InstallLocation 'app\ChatGPT.exe'
if(-not (Test-Path -LiteralPath $exe)){exit 3}
Start-Process -FilePath $exe -ArgumentList '--force-renderer-accessibility' -WindowStyle Hidden
'''
    result=subprocess.run(['powershell.exe','-NoProfile','-NonInteractive','-Command',script],capture_output=True,timeout=15,creationflags=0x08000000)
    if result.returncode==2:raise OSError('请先保存工作并完全退出 Codex／ChatGPT，再选择此项；不会自动关闭你的聊天。')
    if result.returncode:raise OSError('未找到受支持的 Windows Codex 安装，无法进行兼容启动。')
    return '已请求以辅助访问模式打开 Codex；请回到原聊天，之后启动同步并测试。'


class PetRemote:
    def __init__(self, backend):
        self.backend = backend
        self.session = secrets.randbelow(0x7ffffffe) + 1
        self.ack = 0
        self.voice = False

    def reset(self):
        self.backend.release()
        self.voice = False
        self.ack = 0
        self.session = secrets.randbelow(0x7ffffffe) + 1

    def poll(self, connection, rpc):
        reply = rpc(connection, f'PC_CTRL_POLL {self.session} {self.ack}').split()
        if len(reply) != 5 or reply[:2] != ['PC_CTRL', '1']:
            raise OSError('Invalid pet control response')
        seq, action, voice = map(int, reply[2:])
        if not 0 <= seq <= 0xffffffff or not 0 <= action <= 4 or voice not in (0, 1):
            raise OSError('Invalid pet control values')
        self.backend.last_poll=time.monotonic()
        if bool(voice) != self.voice:
            self.voice = bool(voice)
            try:
                if voice:self.backend.hold()
                else:self.backend.release()
                self.backend.error=''
            except OSError as error:self.backend.error=str(error)
        if action and seq != self.ack:
            # Claim before dispatch: errors and lost ACKs must never resend text.
            self.ack = seq
            try:
                if not self.voice:
                    self.backend.action(action)
                    print('Pet control: action '+str(action)+' dispatched',flush=True)
                self.backend.error=''
            except (OSError,TimeoutError) as error:self.backend.error=str(error)
        self.backend.tick()


class DesktopKeys:
    """Windows SendInput for dictation/custom shortcuts; UIA for exact composer actions.

    Never focuses another application, writes clipboard data or uses coordinates.
    macOS keeps ordinary sync available and reports remote-control unsupported.
    """
    def __init__(self):
        import sys
        import threading
        self.guard=threading.RLock()
        self.closed=threading.Event()
        self.last_poll=time.monotonic()
        self.supported = sys.platform == 'win32'
        self.held = []
        self.window = 0
        self.paused_window = 0
        self.started = 0
        self.error = ''
        if self.supported:
            import ctypes
            from ctypes import wintypes
            self.ctypes = ctypes
            self.user = ctypes.WinDLL('user32', use_last_error=True)
            self.kernel = ctypes.WinDLL('kernel32', use_last_error=True)
            self.user.GetForegroundWindow.restype = wintypes.HWND
            self.user.GetWindowThreadProcessId.argtypes = [wintypes.HWND,ctypes.POINTER(wintypes.DWORD)]
            self.kernel.OpenProcess.restype = wintypes.HANDLE
            self.kernel.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
            self.kernel.QueryFullProcessImageNameW.argtypes = [wintypes.HANDLE,wintypes.DWORD,wintypes.LPWSTR,ctypes.POINTER(wintypes.DWORD)]
            self.kernel.CloseHandle.argtypes = [wintypes.HANDLE]
        threading.Thread(target=self._watch,daemon=True,name='pet-key-release').start()

    def _watch(self):
        while not self.closed.wait(.1):
            with self.guard:
                if self.held and time.monotonic()-self.last_poll>1.5:self.release()
                self.tick()

    def close(self):
        self.closed.set();self.release()

    def foreground(self):
        if not self.supported:
            return 0
        c = self.ctypes
        from ctypes import wintypes
        window = self.user.GetForegroundWindow()
        pid = wintypes.DWORD()
        self.user.GetWindowThreadProcessId(window, c.byref(pid))
        handle = self.kernel.OpenProcess(0x1000, False, pid.value)
        if not handle:
            return 0
        try:
            path = c.create_unicode_buffer(32768)
            size = wintypes.DWORD(len(path))
            if not self.kernel.QueryFullProcessImageNameW(handle,0,path,c.byref(size)):
                return 0
            from pathlib import PureWindowsPath
            executable = PureWindowsPath(path.value)
            if executable.name.lower() not in ('chatgpt.exe','codex.exe'):
                return 0
            # Also check installation lineage; unrelated same-name executables fail closed.
            if 'openai' not in path.value.lower():
                return 0
            return window
        finally:
            self.kernel.CloseHandle(handle)

    def key(self, code, down):
        import ctypes as c
        from ctypes import wintypes as w
        class Keyboard(c.Structure):
            _fields_=[('vk',w.WORD),('scan',w.WORD),('flags',w.DWORD),('time',w.DWORD),('extra',c.c_size_t)]
        class Data(c.Union):
            _fields_=[('keyboard',Keyboard),('padding',c.c_byte*32)]
        class Input(c.Structure):
            _fields_=[('type',w.DWORD),('data',Data)]
        # Supply physical scan codes: native shortcut listeners need left/right identity.
        scan=self.user.MapVirtualKeyW(code,0)
        if not scan:raise OSError('Unsupported shortcut key')
        flags=0x0008 | (0 if down else 0x0002) | (0x0001 if code in (0xA5,0x5B,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x2D,0x2E,0x6F,0x90) else 0)
        item=Input(type=1,data=Data(keyboard=Keyboard(0,scan,flags,0,0)))
        if self.user.SendInput(1,c.byref(item),c.sizeof(item))!=1:
            raise OSError('Windows rejected input; use the same privilege level as Codex')

    def hold(self):
        with self.guard:self._hold()

    def _hold(self):
        self.release()
        self.window=self.foreground()
        if not self.window:
            raise OSError('先将 Codex 聊天输入框置于前台，再按住 OK 听写')
        self.started=time.monotonic()
        try:
            for code in dictation_keys(): # Match this computer's Codex hold-to-dictate binding
                self.key(code,True);self.held.append(code)
        except Exception:
            self.release();raise

    def release(self):
        with self.guard:self._release()

    def _release(self):
        keys,self.held=self.held,[]
        for code in reversed(keys):
            try:self.key(code,False)
            except OSError:pass
        self.window=0

    def tick(self):
        if self.held and (not self.foreground() or time.monotonic()-self.started>=20):
            self.release()

    def action(self, action):
        if not self.supported:
            raise OSError('当前宠物遥控支持 Windows；macOS 数据同步不受影响')
        if action==4:
            # Explicit user-configured shortcut; an empty setting is a no-op.
            pressed=[]
            try:
                keys=screenshot_keys()
                for index,key in enumerate(keys):
                    self.key(key,True);pressed.append(key)
                    # Hold left Alt first, then press right Alt while it stays down.
                    time.sleep(.30 if keys==(0xA4,0xA5) and index==0 else .05)
                time.sleep(.35)
            finally:
                for key in reversed(pressed):self.key(key,False)
            return
        window=self.foreground()
        if not window:
            self.paused_window=0
            raise OSError('先将 Codex 聊天输入框置于前台；未执行按键')
        command='send' if action==1 else 'undo' if action==2 else 'toggle'
        result=self._composer(command,window,self.paused_window==window)
        if result=='stopped':self.paused_window=window
        elif result=='continued':self.paused_window=0

    @staticmethod
    def _composer(command, window, paused):
        import subprocess,base64
        # Static script and bounded numeric/enum substitutions, no shell/user text interpolation.
        script = COMPOSER_SCRIPT.replace('COMMAND_VALUE',command).replace('WINDOW_VALUE',str(int(window))).replace('PAUSED_VALUE','$true' if paused else '$false')
        try:result=subprocess.run(['powershell.exe','-NoProfile','-NonInteractive','-EncodedCommand',base64.b64encode(script.encode('utf-16le')).decode()],capture_output=True,timeout=4,creationflags=0x08000000)
        except subprocess.TimeoutExpired as error:raise OSError('Codex 输入控件响应超时，未重试') from error
        text=result.stdout.decode('utf-8-sig','replace').strip()
        if result.returncode==35:raise OSError('输入框已有草稿，未覆盖，也未自动发送继续')
        if result.returncode or text not in ('sent','undone','stopped','continued'):
            raise OSError('无法确认 Codex 输入控件，未执行。请聚焦聊天输入框；必要时重启 Codex。')
        return text


COMPOSER_SCRIPT = r'''
$ErrorActionPreference='Stop'
[Console]::OutputEncoding=[System.Text.Encoding]::UTF8
Add-Type -AssemblyName UIAutomationClient,UIAutomationTypes,System.Windows.Forms
Add-Type 'using System;using System.Runtime.InteropServices;public static class PetForeground{[DllImport("user32.dll")]public static extern IntPtr GetForegroundWindow();}'
$target=[IntPtr]WINDOW_VALUE
if([PetForeground]::GetForegroundWindow() -ne $target){exit 2}
$root=[System.Windows.Automation.AutomationElement]::FromHandle($target)
$scope=[System.Windows.Automation.TreeScope]::Descendants
$button=[System.Windows.Automation.PropertyCondition]::new([System.Windows.Automation.AutomationElement]::ControlTypeProperty,[System.Windows.Automation.ControlType]::Button)
function InvokeNamed($names){
 $buttons=$root.FindAll($scope,$button)
 $matches=@($buttons | Where-Object {$_.Current.Name -cin $names -and $_.Current.IsEnabled -and -not $_.Current.IsOffscreen})
 if($matches.Count -ne 1){return $false}
 if([PetForeground]::GetForegroundWindow() -ne $target){throw 'Focus changed'}
 $pattern=$matches[0].GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern)
 $pattern.Invoke();return $true
}
$command='COMMAND_VALUE'
if($command -eq 'send'){
 if(InvokeNamed @('Send message','Send','发送消息','发送')){Write-Output 'sent';exit 0};exit 2
}
if($command -eq 'toggle' -and (InvokeNamed @('Stop','Stop task','停止','停止任务'))){Write-Output 'stopped';exit 0}
if($command -eq 'undo'){
 $focused=[System.Windows.Automation.AutomationElement]::FocusedElement
 if($null -eq $focused -or $focused.Current.ProcessId -ne $root.Current.ProcessId -or $focused.Current.ControlType -ne [System.Windows.Automation.ControlType]::Edit){exit 2}
 if($focused.Current.Name -notmatch '(?i)codex|message|prompt|消息|输入|询问'){exit 2}
 if([PetForeground]::GetForegroundWindow() -ne $target){exit 2}
 [System.Windows.Forms.SendKeys]::SendWait('^z');Write-Output 'undone';exit 0
}
# Locate the unique visible composer even after Stop moved keyboard focus.
$editCondition=[System.Windows.Automation.PropertyCondition]::new([System.Windows.Automation.AutomationElement]::ControlTypeProperty,[System.Windows.Automation.ControlType]::Edit)
$editors=@($root.FindAll($scope,$editCondition) | Where-Object {
 $_.Current.IsEnabled -and -not $_.Current.IsOffscreen -and $_.Current.ClassName -eq 'ProseMirror' -and $_.Current.Name -match '(?i)codex|message|prompt|消息|输入|询问'
})
if($editors.Count -ne 1){exit 2}
$edit=$editors[0]
if([PetForeground]::GetForegroundWindow() -ne $target){exit 2}
$edit.SetFocus()
Start-Sleep -Milliseconds 80
if([PetForeground]::GetForegroundWindow() -ne $target){exit 2}
if($command -eq 'toggle' -and PAUSED_VALUE){
 $value=$edit.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern)
 $text=$value.Current.Value.Trim()
 # Chromium can expose an empty contenteditable's placeholder as Value.
 $sendButtons=@($root.FindAll($scope,$button) | Where-Object {$_.Current.Name -cin @('Send message','Send','发送消息','发送') -and -not $_.Current.IsOffscreen})
 $placeholder=$text -eq $edit.Current.Name -and $edit.Current.Name -cin @('和 Codex 一起工作','Work with Codex','Ask Codex anything','向 Codex 提问') -and @($sendButtons | Where-Object {$_.Current.IsEnabled}).Count -eq 0
 if($text.Length -ne 0 -and -not $placeholder){exit 35}
 $value.SetValue('继续')
 # Wait for the editor's input event to enable/recreate Send; never inject Enter blindly.
 for($attempt=0;$attempt -lt 12;$attempt++){
  Start-Sleep -Milliseconds 50
  if([PetForeground]::GetForegroundWindow() -ne $target){exit 2}
  if(InvokeNamed @('Send message','Send','发送消息','发送')){Write-Output 'continued';exit 0}
 }
}

exit 2
'''
