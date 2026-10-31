#pragma once
#include <windows.h>
#include <string>
#include <vector>
namespace storage_restart {
// Windows argv quoting, including embedded quotes and trailing backslashes.
inline std::wstring Quote(const std::wstring& value){
    std::wstring out=L"\"";size_t slashes=0;
    for(wchar_t c:value){
        if(c==L'\\'){++slashes;continue;}
        out.append(c==L'\"'?slashes*2+1:slashes,L'\\');slashes=0;out+=c;
    }
    out.append(slashes*2,L'\\');out+=L'\"';return out;
}
struct Pending {
    HANDLE process=nullptr,cancel=nullptr;
    void Close(bool abort=false){
        if(abort&&cancel)SetEvent(cancel);
        if(cancel)CloseHandle(cancel);if(process)CloseHandle(process);
        process=cancel=nullptr;
    }
};
// The helper confirms it owns the wait handles before the caller requests exit.
inline bool Prepare(const std::wstring& helper,const std::wstring& exe,
    const std::wstring& directory,const std::wstring& command,Pending& out){
    if(out.process)return false;
    SECURITY_ATTRIBUTES sa{sizeof(sa),nullptr,TRUE};
    HANDLE parent=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,TRUE,GetCurrentProcessId());
    HANDLE ready=CreateEventW(&sa,TRUE,FALSE,nullptr),cancel=CreateEventW(&sa,TRUE,FALSE,nullptr);
    bool ok=false;PROCESS_INFORMATION pi{};SIZE_T bytes=0;
    InitializeProcThreadAttributeList(nullptr,1,0,&bytes);
    std::vector<unsigned char> memory(bytes);
    auto* attributes=reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(memory.data());
    const bool initialized=bytes&&InitializeProcThreadAttributeList(attributes,1,0,&bytes);
    if(parent&&ready&&cancel&&initialized){
        HANDLE handles[]={parent,ready,cancel};
        if(UpdateProcThreadAttribute(attributes,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,handles,sizeof(handles),nullptr,nullptr)){
            auto cmd=Quote(helper)+L" --restart "+std::to_wstring(reinterpret_cast<ULONG_PTR>(parent))+L" "+
                std::to_wstring(reinterpret_cast<ULONG_PTR>(ready))+L" "+std::to_wstring(reinterpret_cast<ULONG_PTR>(cancel))+L" "+
                Quote(exe)+L" "+Quote(directory)+L" "+Quote(command);
            STARTUPINFOEXW si{};si.StartupInfo.cb=sizeof(si);si.lpAttributeList=attributes;
            if(CreateProcessW(helper.c_str(),cmd.data(),nullptr,nullptr,TRUE,EXTENDED_STARTUPINFO_PRESENT|CREATE_NO_WINDOW,
                nullptr,directory.c_str(),&si.StartupInfo,&pi)){
                HANDLE waits[]={ready,pi.hProcess};
                ok=WaitForMultipleObjects(2,waits,FALSE,5000)==WAIT_OBJECT_0;
                if(ok){out.process=pi.hProcess;out.cancel=cancel;cancel=nullptr;pi.hProcess=nullptr;}
                else SetEvent(cancel);
            }
        }
    }
    if(initialized)DeleteProcThreadAttributeList(attributes);
    if(pi.hThread)CloseHandle(pi.hThread);if(pi.hProcess)CloseHandle(pi.hProcess);
    if(parent)CloseHandle(parent);if(ready)CloseHandle(ready);if(cancel)CloseHandle(cancel);
    return ok;
}
}
