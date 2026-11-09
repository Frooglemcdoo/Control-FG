#pragma once
// POSIX test shim for the unmodified exporter. Not used by the Windows build.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <filesystem>
#include <string>
#include <algorithm>
using DWORD=std::uint32_t;using HANDLE=std::FILE*;
inline HANDLE INVALID_HANDLE_VALUE=nullptr;
constexpr int FALSE=0,GENERIC_WRITE=1,CREATE_NEW=1,FILE_ATTRIBUTE_NORMAL=0,
 FILE_FLAG_SEQUENTIAL_SCAN=0,FILE_ATTRIBUTE_DIRECTORY=16,MOVEFILE_WRITE_THROUGH=1;
constexpr DWORD INVALID_FILE_ATTRIBUTES=~DWORD(0),ERROR_ALREADY_EXISTS=183;
inline bool failPart1Write=false,failReflectanceWrite=false;
inline HANDLE reflectanceFile=nullptr;
inline HANDLE part1File=nullptr;
inline std::string TestPath(const wchar_t* w){std::wstring s(w);std::string r(s.begin(),s.end());std::replace(r.begin(),r.end(),'\\','/');return r;}
inline HANDLE CreateFileW(const wchar_t* w,int,int,void*,int,int,void*){
 auto name=TestPath(w);if(std::filesystem::exists(name))return nullptr;
 auto f=std::fopen(name.c_str(),"wb");if(name.find("material-part1.bin")!=std::string::npos)part1File=f;if(name.find("specular-reflectance-candidate")!=std::string::npos)reflectanceFile=f;return f;
}
inline bool WriteFile(HANDLE f,const void* b,DWORD n,DWORD* wrote,void*){
 if(failReflectanceWrite && f==reflectanceFile){*wrote=0;return false;}
 if(failPart1Write && f==part1File){*wrote=0;return false;}
 *wrote=static_cast<DWORD>(std::fwrite(b,1,n,f));return *wrote==n;
}
inline bool CloseHandle(HANDLE f){if(f==part1File)part1File=nullptr;if(f==reflectanceFile)reflectanceFile=nullptr;return std::fclose(f)==0;}
inline DWORD GetFileAttributesW(const wchar_t* w){return std::filesystem::is_directory(TestPath(w))?16:INVALID_FILE_ATTRIBUTES;}
inline bool CreateDirectoryW(const wchar_t* w,void*){return std::filesystem::create_directory(TestPath(w));}
inline DWORD GetLastError(){return ERROR_ALREADY_EXISTS;}
inline DWORD GetCurrentProcessId(){return 1;}
inline DWORD GetEnvironmentVariableW(const wchar_t*,wchar_t* out,DWORD cap){
 const char* p=std::getenv("CONTROL_EXPORT_TEST_ROOT");if(!p)return 0;
 std::string s(p);if(s.size()+1>cap)return static_cast<DWORD>(s.size()+1);
 std::wstring w(s.begin(),s.end());std::wcscpy(out,w.c_str());return static_cast<DWORD>(w.size());
}
struct SYSTEMTIME{unsigned short wYear=2026,wMonth=9,wDayOfWeek=1,wDay=14,wHour=0,wMinute=0,wSecond=0,wMilliseconds=0;};
inline void GetSystemTime(SYSTEMTIME* t){*t={};}
inline bool MoveFileExW(const wchar_t* a,const wchar_t* b,int){std::filesystem::rename(TestPath(a),TestPath(b));return true;}
