#include <windows.h>
#include <cstdio>
#include <string>
#include "MinHook.h"
static void FakeFrame(){}
static void FakeMode(unsigned long long,bool){}
static void FakeCommit(unsigned long long,HRESULT,const char*){}
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
 void* targets[]={reinterpret_cast<unsigned char*>(core)+0x1DF60,reinterpret_cast<unsigned char*>(core)+0x41130,reinterpret_cast<unsigned char*>(core)+0x10950};
 void* detours[]={(void*)&FakeFrame,(void*)&FakeMode,(void*)&FakeCommit};
 if(MH_Initialize()!=MH_OK)return 7;
 for(int i=0;i<3;++i){void* original=nullptr;if(MH_CreateHook(targets[i],detours[i],&original)!=MH_OK || !original)return 8;if(MH_QueueEnableHook(targets[i])!=MH_OK)return 9;}
 if(MH_ApplyQueued()!=MH_OK || MH_DisableHook(MH_ALL_HOOKS)!=MH_OK || MH_Uninitialize()!=MH_OK)return 10;
 std::puts("PASS all three core-entry detours create, enable and remove on Windows; rendering functions not called");
 std::puts("PASS packaged DXGI loads, five original exports resolve, controller import resolves; game rendering not invoked");
 return 0;
}
