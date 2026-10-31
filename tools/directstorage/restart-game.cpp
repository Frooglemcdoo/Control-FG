#include <windows.h>
#include <string>
#include <cwchar>
#include <cstdlib>
#include <initializer_list>
static HANDLE ParseHandle(const wchar_t* text){
    wchar_t* end=nullptr;const auto value=std::wcstoull(text,&end,10);
    return !value||!end||*end?nullptr:reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(value));
}
int wmain(int argc,wchar_t** argv){
    if(argc!=8||std::wcscmp(argv[1],L"--restart"))return 2;
    HANDLE parent=ParseHandle(argv[2]),ready=ParseHandle(argv[3]),cancel=ParseHandle(argv[4]);
    if(!parent||!ready||!cancel||!GetProcessId(parent)||GetProcessId(parent)==GetCurrentProcessId()||
        WaitForSingleObject(parent,0)!=WAIT_TIMEOUT||GetFileAttributesW(argv[5])==INVALID_FILE_ATTRIBUTES)return 3;
    if(!SetEvent(ready))return 4;
    HANDLE waits[]={cancel,parent};
    const auto wait=WaitForMultipleObjects(2,waits,FALSE,120000);
    CloseHandle(parent);CloseHandle(ready);CloseHandle(cancel);
    if(wait!=WAIT_OBJECT_0+1)return 5; // Cancellation/timeout never starts a second game.
    // A previous diagnostic launcher must not override the newly saved menu choice.
    for(const auto* key:{L"CONTROLFG_NATIVE_GPU_STORAGE",L"CONTROLFG_TEXTURE_UPGRADE_GATE",
        L"CONTROLFG_STREAMING_PIN_SLICES",L"CONTROLFG_STREAMING_SLICES",L"CONTROLFG_STREAMING_BUDGET_MS"})
        SetEnvironmentVariableW(key,nullptr);
    std::wstring command=argv[7];STARTUPINFOW si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{};
    if(!CreateProcessW(argv[5],command.data(),nullptr,nullptr,FALSE,0,nullptr,argv[6],&si,&pi))return 6;
    CloseHandle(pi.hThread);CloseHandle(pi.hProcess);return 0;
}
