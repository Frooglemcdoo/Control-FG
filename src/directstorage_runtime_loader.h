#pragma once
#include <windows.h>
#include <dstorage.h>
#include <wrl/client.h>
#include <string>
namespace control_storage {
struct RuntimeLoadResult {const char* stage="path";HRESULT hr=E_INVALIDARG;DWORD win32=0;};
inline bool LoadRuntimeFactory(const std::wstring& modulePath,HMODULE& module,
    Microsoft::WRL::ComPtr<IDStorageFactory>& factory,RuntimeLoadResult& result){
    const auto slash=modulePath.find_last_of(L"\\/");
    if(slash==std::wstring::npos)return false;
    // The SDK wrapper can resolve dstoragecore.dll relative to the EXE, not
    // relative to the wrapper DLL. Ship both DLLs beside Control_DX12.exe.
    const auto path=modulePath.substr(0,slash+1)+L"dstorage.dll";
    module=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if(!module){result.stage="runtime_load";result.win32=GetLastError();result.hr=result.win32?HRESULT_FROM_WIN32(result.win32):E_FAIL;return false;}
    using GetFactory=HRESULT(WINAPI*)(REFIID,void**);
    auto get=reinterpret_cast<GetFactory>(GetProcAddress(module,"DStorageGetFactory"));
    if(!get){result.stage="factory_export";result.win32=GetLastError();result.hr=result.win32?HRESULT_FROM_WIN32(result.win32):E_NOINTERFACE;return false;}
    result.stage="factory";SetLastError(ERROR_SUCCESS);
    result.hr=get(IID_PPV_ARGS(&factory));result.win32=GetLastError();
    if(FAILED(result.hr))return false;
    if(!factory){result.hr=E_UNEXPECTED;return false;}
    result.stage="ready";return true;
}
}
