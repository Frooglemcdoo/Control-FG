#include <windows.h>
#include <cstdio>
#include <string>
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 1;
 std::wstring path=argv[1];std::wstring dir=path.substr(0,path.find_last_of(L"\\/"));
 if(!SetDllDirectoryW(dir.c_str()))return 2;
 HMODULE core=LoadLibraryExW(path.c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
 if(!core){std::printf("core load failed error=%lu\n",GetLastError());return 3;}
 const char* exports[]={"CreateDXGIFactory","CreateDXGIFactory1","CreateDXGIFactory2","DXGIDeclareAdapterRemovalSupport","DXGIGetDebugInterface1"};
 for(const char* n:exports)if(!GetProcAddress(core,n)){std::printf("missing core export %s\n",n);return 4;}
 HMODULE helper=GetModuleHandleW(L"ControlFGHDRGuard.dll");if(!helper){std::puts("helper not imported");return 5;}
 auto version=reinterpret_cast<unsigned(WINAPI*)()>(GetProcAddress(helper,"ControlFGHDRGuard_Version"));
 if(!version || version()!=0x00120001)return 6;
 std::puts("PASS packaged DXGI loads, five original exports resolve, controller import resolves; game rendering not invoked");
 return 0;
}
