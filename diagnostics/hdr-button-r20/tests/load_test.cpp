#include <windows.h>
#include <cstdio>
int wmain(int argc,wchar_t** argv){if(argc!=2)return 1;auto m=LoadLibraryW(argv[1]);if(!m){std::printf("load failure %lu\n",GetLastError());return 2;}
 auto v=reinterpret_cast<unsigned(WINAPI*)()>(GetProcAddress(m,"ControlFGHDRButton_Version"));
 if(!v||v()!=0x00360001||!GetProcAddress(m,"ControlFGHDRButton_Bootstrap"))return 3;
 std::puts("PASS helper load/exports");return 0;}
