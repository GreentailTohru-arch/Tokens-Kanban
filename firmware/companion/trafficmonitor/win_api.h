#pragma once
/* Minimal x64 Windows SDK declarations. Kept header-only for SDK-free /Zl builds.
   Signatures: Microsoft CreateFileW / WriteFile / CloseHandle / GetTickCount64 /
   GetCurrentProcessId documentation. No private APIs or process-memory access. */
static_assert(sizeof(void*)==8 && sizeof(unsigned long)==4,"Windows x64 required");
extern "C" {
__declspec(dllimport) void* __stdcall CreateFileW(const wchar_t*,unsigned long,unsigned long,void*,unsigned long,unsigned long,void*);
__declspec(dllimport) int __stdcall WriteFile(void*,const void*,unsigned long,unsigned long*,void*);
__declspec(dllimport) int __stdcall CloseHandle(void*);
__declspec(dllimport) unsigned long long __stdcall GetTickCount64();
__declspec(dllimport) unsigned long __stdcall GetCurrentProcessId();
}
