#pragma once
#define __cdecl
#define __try try
#define __except(x) catch(...)
using HMODULE=void*;
inline void* GetProcAddress(HMODULE,const char*) {return nullptr;}
