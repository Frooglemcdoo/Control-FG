#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../src/pt_startup_settings.h"
#include <cassert>
#include <cstdio>

int wmain(int argc,wchar_t** argv) {
    const bool initial=argc>1&&wcscmp(argv[1],L"on")==0;
    wchar_t temp[MAX_PATH]{},old[32768]{};
    GetTempPathW(MAX_PATH,temp);
    GetEnvironmentVariableW(L"LOCALAPPDATA",old,_countof(old));
    const auto root=std::wstring(temp)+L"ControlFG-Startup-Test-"+std::to_wstring(GetCurrentProcessId());
    assert(CreateDirectoryW(root.c_str(),nullptr));
    assert(SetEnvironmentVariableW(L"LOCALAPPDATA",root.c_str()));
    const auto dir=root+L"\\ControlFG",legacy=dir+L"\\settings.ini",primary=dir+L"\\settings-mfg-test.ini";
    assert(!PTStartupReadSelection());
    assert(CreateDirectoryW(dir.c_str(),nullptr));
    assert(WritePrivateProfileStringW(L"FrameGeneration",L"Mode",L"3",legacy.c_str()));
    assert(WritePrivateProfileStringW(L"RayReconstruction",L"Enabled",L"1",legacy.c_str()));
    assert(WritePrivateProfileStringW(L"PTReflections",L"Enabled",initial?L"1":L"0",legacy.c_str()));
    SetLastError(1234);
    assert(PTStartupCaptureEnabled()==initial);
    assert(GetLastError()==1234);
    // First write migrates other preferences without modifying the fallback.
    assert(PTStartupWriteSelection(!initial));
    assert(PTStartupReadSelection()==!initial);
    assert(PTStartupCaptureEnabled()==initial);
    assert(GetPrivateProfileIntW(L"FrameGeneration",L"Mode",0,primary.c_str())==3);
    assert(GetPrivateProfileIntW(L"RayReconstruction",L"Enabled",0,primary.c_str())==1);
    assert(GetPrivateProfileIntW(L"PTReflections",L"Enabled",2,legacy.c_str())==(initial?1u:0u));
    assert(PTStartupWriteSelection(initial));
    assert(PTStartupReadSelection()==initial);
    assert(PTStartupCaptureEnabled()==initial);
    assert(SetFileAttributesW(primary.c_str(),FILE_ATTRIBUTE_READONLY));
    assert(!PTStartupWriteSelection(!initial));
    assert(PTStartupReadSelection()==initial);
    assert(SetFileAttributesW(primary.c_str(),FILE_ATTRIBUTE_NORMAL));
    assert(DeleteFileW(primary.c_str()));
    assert(DeleteFileW(legacy.c_str()));
    assert(RemoveDirectoryW(dir.c_str()));
    assert(RemoveDirectoryW(root.c_str()));
    assert(SetEnvironmentVariableW(L"LOCALAPPDATA",nullptr));
    assert(!PTStartupWriteSelection(true));
    assert(!PTStartupReadSelection());
    assert(PTStartupCaptureEnabled()==initial);
    SetEnvironmentVariableW(L"LOCALAPPDATA",old[0]?old:nullptr);
    puts("PASS: startup selection latched, opposite selection persists, unrelated preferences migrate, write failures rejected.");
}
