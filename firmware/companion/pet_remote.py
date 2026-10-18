"""Device input is ephemeral: never replay after reconnection or focus changes."""
import secrets
import time


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
                if not self.voice:self.backend.action(action)
                self.backend.error=''
            except (OSError,TimeoutError) as error:self.backend.error=str(error)
        self.backend.tick()


class DesktopKeys:
    """Windows SendInput for hold/Appshots; UIA for exact composer actions.

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
        item=Input(type=1,data=Data(keyboard=Keyboard(code,0,(0 if down else 2)|(1 if code==0xA5 else 0),0,0)))
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
            for code in (0x11,0x12,0x78): # Ctrl+Alt+F9, user-configured hold-to-dictate
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
        if self.held and (self.foreground()!=self.window or time.monotonic()-self.started>=20):
            self.release()

    def action(self, action):
        if not self.supported:
            raise OSError('当前宠物遥控支持 Windows；macOS 数据同步不受影响')
        if action==4:
            # Intentional global Appshot: capture the foreground app via Codex itself.
            pressed=[]
            try:
                for key in (0xA4,0xA5):self.key(key,True);pressed.append(key)
                time.sleep(.08)
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
        text=result.stdout.decode('utf8','replace').strip()
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
$buttons=$root.FindAll($scope,$button)
function InvokeNamed($names){
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
$edit=[System.Windows.Automation.AutomationElement]::FocusedElement
if($null -eq $edit -or $edit.Current.ProcessId -ne $root.Current.ProcessId -or $edit.Current.ControlType -ne [System.Windows.Automation.ControlType]::Edit){exit 2}
if($edit.Current.Name -notmatch '(?i)codex|message|prompt|消息|输入|询问'){exit 2}
if([PetForeground]::GetForegroundWindow() -ne $target){exit 2}
if($command -eq 'undo'){[System.Windows.Forms.SendKeys]::SendWait('^z');Write-Output 'undone';exit 0}
if($command -eq 'toggle' -and PAUSED_VALUE){
 $value=$edit.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern)
 if($value.Current.Value.Trim().Length -ne 0){exit 2}
 $value.SetValue('继续')
 if(InvokeNamed @('Send message','Send','发送消息','发送')){Write-Output 'continued';exit 0}
}
exit 2
'''
