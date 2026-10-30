#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <string>
#include "../../src/directstorage_runtime_loader.h"
#include "../../src/directstorage_archive_cache.h"
int wmain(){
    // Deliberately not linked against dstorage.lib. CI places the runtime pair
    // beside this EXE and starts it from a different working directory.
    if(GetModuleHandleW(L"dstorage.dll")){puts("FAIL runtime unexpectedly preloaded");return 1;}
    HMODULE module=nullptr;Microsoft::WRL::ComPtr<IDStorageFactory> factory;
    control_storage::RuntimeLoadResult invalid;
    if(control_storage::LoadRuntimeFactory(L"no_directory",module,factory,invalid)||invalid.hr!=E_INVALIDARG){puts("FAIL invalid path handling");return 2;}
    wchar_t exe[32768]{};const DWORD n=GetModuleFileNameW(nullptr,exe,_countof(exe));
    if(!n||n>=_countof(exe))return 3;
    control_storage::RuntimeLoadResult result;
    if(!control_storage::LoadRuntimeFactory(exe,module,factory,result)){
        printf("FAIL stage=%s hr=0x%08lX win32=%lu\n",result.stage,static_cast<unsigned long>(result.hr),result.win32);return 4;
    }
    control_storage::ArchiveCache cache;
    if(!cache.Initialize(factory.Get())||!cache.CloseIfIdle()){puts("FAIL cache initialization");return 5;}
    puts("PASS R231 dynamic runtime loading: no static import, EXE-adjacent runtime pair, different working directory, factory and queue creation");
    // Module remains loaded through COM teardown / process exit, as in-game.
    return 0;
}
