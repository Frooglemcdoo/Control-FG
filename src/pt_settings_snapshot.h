#pragma once
// Read-only evidence for native settings resets. Keep copies beside this run's
// log, never modify renderer.ini or restore settings over a running game.
static void PTSettingsSnapshot(const wchar_t* stage) noexcept {
    const DWORD saved=GetLastError();
    try {
        if(!ptSettingsSnapshotBase.empty()){
            auto game=ModulePath(nullptr);const auto slash=game.find_last_of(L"\\/");
            if(slash!=std::wstring::npos)game.resize(slash+1);else game.clear();
            wchar_t cwd[32768]{};const DWORD n=GetCurrentDirectoryW(_countof(cwd),cwd);
            const std::wstring current=n&&n<_countof(cwd)?std::wstring(cwd)+L"\\":L"";
            for(unsigned i=0;i<2;++i){const auto& folder=i?current:game;
                if(folder.empty()||(i&&_wcsicmp(game.c_str(),current.c_str())==0))continue;
                const auto source=folder+L"renderer.ini";WIN32_FILE_ATTRIBUTE_DATA data{};
                const bool exists=GetFileAttributesExW(source.c_str(),GetFileExInfoStandard,&data)!=FALSE;
                const auto destination=ptSettingsSnapshotBase+L"-settings-"+stage+(i?L"-cwd.ini":L"-game.ini");
                const bool copied=exists&&!data.nFileSizeHigh&&data.nFileSizeLow<=1048576&&
                    !(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)&&CopyFileW(source.c_str(),destination.c_str(),TRUE)!=FALSE;
                Log("PT_PRIMARY_SETTINGS_SNAPSHOT stage=%ls source=%ls exists=%u copied=%u bytes=%lu error=%lu",stage,source.c_str(),unsigned(exists),unsigned(copied),data.nFileSizeLow,GetLastError());
            }
        }
    }catch(...){Log("PT_PRIMARY_SETTINGS_SNAPSHOT failed=exception");}
    SetLastError(saved);
}
