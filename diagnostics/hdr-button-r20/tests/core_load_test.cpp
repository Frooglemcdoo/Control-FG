#include <windows.h>
#include <cstdio>
#include <string>
#include "MinHook.h"
static void FakeFrame(){}
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 1;std::wstring path=argv[1],dir=path.substr(0,path.find_last_of(L"\\/"));SetDllDirectoryW(dir.c_str());
 HMODULE core=LoadLibraryExW(path.c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);if(!core){std::printf("core load fail %lu\n",GetLastError());return 2;}
 HMODULE helper=GetModuleHandleW(L"ControlFGHDRButton.dll");if(!helper)return 3;
 void* target=reinterpret_cast<unsigned char*>(core)+0x1DF60;void* original=nullptr;
 if(MH_Initialize()!=MH_OK)return 4;if(MH_CreateHook(target,(void*)&FakeFrame,&original)!=MH_OK||!original)return 5;
 if(MH_EnableHook(target)!=MH_OK)return 6;if(MH_DisableHook(target)!=MH_OK)return 7;if(MH_Uninitialize()!=MH_OK)return 8;
 std::puts("PASS packaged core/helper load and frame-hook install");return 0;
}
