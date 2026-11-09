#include "../../src/storage_restart.h"
#include <cassert>
#include <fstream>
#include <filesystem>
#include <cstdio>
namespace fs=std::filesystem;
static PROCESS_INFORMATION Spawn(const std::wstring& exe,std::wstring command){
    STARTUPINFOW si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{};
    assert(CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&si,&pi));
    CloseHandle(pi.hThread);pi.hThread=nullptr;return pi;
}
int wmain(int argc,wchar_t** argv){
    wchar_t exeBuffer[32768]{},cwd[32768]{};assert(GetModuleFileNameW(nullptr,exeBuffer,32768));assert(GetCurrentDirectoryW(32768,cwd));
    const std::wstring exe=exeBuffer;
    if(argc==4&&std::wstring(argv[1])==L"--child"){
        assert(std::wstring(argv[3])==L"argument with spaces and \"quotes\"\\");
        wchar_t env[8]{};assert(!GetEnvironmentVariableW(L"CONTROLFG_NATIVE_GPU_STORAGE",env,8));
        std::ofstream(fs::path(argv[2]))<<"restarted";return 0;
    }
    if(argc==5&&std::wstring(argv[1])==L"--parent"){
        const fs::path marker=argv[3];const bool cancel=std::wstring(argv[4])==L"cancel";
        SetEnvironmentVariableW(L"CONTROLFG_NATIVE_GPU_STORAGE",L"1");
        storage_restart::Pending pending;
        const auto command=storage_restart::Quote(exe)+L" --child "+storage_restart::Quote(marker.wstring())+L" "+storage_restart::Quote(L"argument with spaces and \"quotes\"\\");
        assert(storage_restart::Prepare(argv[2],exe,cwd,command,pending));
        if(cancel){
            SetEvent(pending.cancel);assert(WaitForSingleObject(pending.process,10000)==WAIT_OBJECT_0);
            DWORD code=0;assert(GetExitCodeProcess(pending.process,&code)&&code==5);
        }
        std::ofstream(fs::path(marker.wstring()+L".ready"))<<"ready";
        // Parent stays alive briefly: child must not launch during this period.
        Sleep(800);assert(!fs::exists(marker));pending.Close();return 0;
    }
    assert(argc==2);
    wchar_t temp[32768]{};assert(GetTempPathW(32768,temp));
    const fs::path root=fs::path(temp)/(L"ControlFG Restart Test "+std::to_wstring(GetCurrentProcessId()));
    fs::create_directories(root);
    storage_restart::Pending missing;
    assert(!storage_restart::Prepare((root/L"missing.exe").wstring(),exe,cwd,L"unused",missing));
    for(bool cancel:{false,true}){
        const fs::path marker=root/(cancel?L"cancel.txt":L"restart.txt");
        auto pi=Spawn(exe,storage_restart::Quote(exe)+L" --parent "+storage_restart::Quote(argv[1])+L" "+storage_restart::Quote(marker.wstring())+(cancel?L" cancel":L" restart"));
        // Deliberate separate arguments, including the no-cancel mode.
        assert(WaitForSingleObject(pi.hProcess,15000)==WAIT_OBJECT_0);
        DWORD code=0;assert(GetExitCodeProcess(pi.hProcess,&code)&&code==0);CloseHandle(pi.hProcess);
        assert(fs::exists(marker.wstring()+L".ready"));
        if(!cancel)for(unsigned i=0;i<100&&!fs::exists(marker);++i)Sleep(100);
        assert(fs::exists(marker)==!cancel);
    }
    fs::remove_all(root);
    puts("PASS R255 restart helper: parent-exit ordering, cancellation, missing helper, quoted arguments and cleared storage override");
}
