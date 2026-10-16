param([string]$OutputDirectory = "$PSScriptRoot/../../build/trafficmonitor",[string]$CompilerBin)
$ErrorActionPreference='Stop'
if(!$CompilerBin) {
  $candidates=Get-Item "$env:ProgramFiles/Microsoft Visual Studio/*/*/VC/Tools/MSVC/*/bin/Hostx64/x64/cl.exe" -ErrorAction SilentlyContinue
  $selected=$candidates | Sort-Object LastWriteTime -Descending | Select-Object -First 1
  if(!$selected){throw 'Install an x64 MSVC C++ compiler or pass -CompilerBin.'}
  $CompilerBin=$selected.DirectoryName
}
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$out=(Resolve-Path -LiteralPath $OutputDirectory).Path
& "$CompilerBin/lib.exe" /nologo /machine:x64 "/def:$PSScriptRoot/kernel32.def" "/out:$out/kernel32-bridge.lib"
if($LASTEXITCODE){throw 'Import library build failed'}
& "$CompilerBin/cl.exe" /nologo /c /O1 /Zl /GS- /GR- /EHs-c- /std:c++20 /W4 /wd4100 /utf-8 "$PSScriptRoot/bridge.cpp" "/Fo$out/bridge.obj"
if($LASTEXITCODE){throw 'Bridge compilation failed'}
& "$CompilerBin/link.exe" /NOLOGO /DLL /NODEFAULTLIB /ENTRY:DllMain /MACHINE:X64 /DYNAMICBASE /NXCOMPAT "/OUT:$out/TokensKanbanBridge.dll" "$out/bridge.obj" "$out/kernel32-bridge.lib"
if($LASTEXITCODE){throw 'Bridge link failed'}
Get-FileHash -Algorithm SHA256 -LiteralPath "$out/TokensKanbanBridge.dll"
