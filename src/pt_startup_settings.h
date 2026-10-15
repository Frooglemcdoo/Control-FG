#pragma once
#include <windows.h>
#include <string>

// One persisted selection, latched before installing any PT hooks. A restart
// supplies fresh scene capture and guarantees old recorded lists are gone.
static std::wstring PTStartupSettingsPath(bool prepare) {
    wchar_t base[32768]{};
    const DWORD n=GetEnvironmentVariableW(L"LOCALAPPDATA",base,_countof(base));
    if(!n||n>=_countof(base))return {};
    const std::wstring dir=std::wstring(base)+L"\\ControlFG";
    const std::wstring path=dir+L"\\settings-mfg-test.ini";
    const std::wstring legacy=dir+L"\\settings.ini";
    if(prepare){
        if(!CreateDirectoryW(dir.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS)return {};
        if(GetFileAttributesW(path.c_str())==INVALID_FILE_ATTRIBUTES){
            if(!CopyFileW(legacy.c_str(),path.c_str(),TRUE)){
                const DWORD error=GetLastError();
                if(error!=ERROR_FILE_NOT_FOUND&&error!=ERROR_PATH_NOT_FOUND&&error!=ERROR_FILE_EXISTS)return {};
            }
        }
        return path;
    }
    return GetFileAttributesW(path.c_str())==INVALID_FILE_ATTRIBUTES?legacy:path;
}
static bool PTStartupReadSelection() {
    const DWORD error=GetLastError();
    const auto path=PTStartupSettingsPath(false);
    const bool enabled=!path.empty()&&GetPrivateProfileIntW(L"PTReflections",L"Enabled",0,path.c_str())!=0;
    SetLastError(error);return enabled;
}
static bool PTStartupCaptureEnabled() {
    static const bool enabled=PTStartupReadSelection();
    return enabled;
}
static bool PTStartupWriteSelection(bool enabled) {
    const auto path=PTStartupSettingsPath(true);
    if(path.empty())return false;
    if(!WritePrivateProfileStringW(L"PTReflections",L"Enabled",enabled?L"1":L"0",path.c_str()))return false;
    if(!WritePrivateProfileStringW(nullptr,nullptr,nullptr,path.c_str()))return false;
    return GetPrivateProfileIntW(L"PTReflections",L"Enabled",2,path.c_str())==(enabled?1u:0u);
}
