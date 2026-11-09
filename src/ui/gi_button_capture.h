#pragma once
#include <windows.h>
#include <cwchar>
#include <shellapi.h>
#include "../gi_frame_capture.h"
#pragma comment(lib,"shell32.lib")
namespace control_gi_button {
inline HANDLE worker=nullptr;
inline bool Active() noexcept {
    if(!worker)return false;
    DWORD code=STILL_ACTIVE;
    if(GetExitCodeProcess(worker,&code)&&code==STILL_ACTIVE)return true;
    CloseHandle(worker);worker=nullptr;return false;
}
inline bool Start(HMODULE module,HWND owner) noexcept {
    if(Active())return true;
    wchar_t directory[32768]{};
    const DWORD size=GetModuleFileNameW(module,directory,32768);
    if(!size||size>=32768)return false;
    auto* slash=wcsrchr(directory,L'\\');if(!slash)return false;*slash=0;
    wchar_t helper[32768]{};
    if(swprintf_s(helper,L"%s\\ControlTOH\\Diagnostics\\Capture-GI-Button.ps1",directory)<0 || GetFileAttributesW(helper)==INVALID_FILE_ATTRIBUTES)return false;
    wchar_t executable[MAX_PATH]{};
    if(!GetSystemDirectoryW(executable,MAX_PATH))return false;
    if(wcscat_s(executable,L"\\WindowsPowerShell\\v1.0\\powershell.exe"))return false;
    wchar_t parameters[32768]{};
    if(swprintf_s(parameters,L"-NoProfile -ExecutionPolicy Bypass -File \"%s\" -GameProcessId %lu -ModuleFolder \"%s\"",helper,GetCurrentProcessId(),directory)<0)return false;
    SHELLEXECUTEINFOW run{};run.cbSize=sizeof(run);run.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NOASYNC;
    run.hwnd=owner;run.lpVerb=L"open";run.lpFile=executable;run.lpParameters=parameters;run.lpDirectory=directory;run.nShow=SW_SHOWMINNOACTIVE;
    if(!ShellExecuteExW(&run)||!run.hProcess)return false;
    worker=run.hProcess;control_gi_frames::Start();return true;
}
}
