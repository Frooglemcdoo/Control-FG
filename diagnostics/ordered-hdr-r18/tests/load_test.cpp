#include <windows.h>
#include <cstdio>
int wmain(int argc,wchar_t** argv){if(argc!=2)return 1;auto m=LoadLibraryW(argv[1]);if(!m){std::printf("load failure %lu\n",GetLastError());return 2;}
 auto fn=reinterpret_cast<unsigned(WINAPI*)()>(GetProcAddress(m,"ControlFGHDRGuard_Version"));
 if(!fn || fn()!=0x00120001 || !GetProcAddress(m,"ControlFGHDRGuard_Bootstrap"))return 3;
 std::puts("PASS helper PE loading and exports; no graphics or game code invoked");return 0;}
