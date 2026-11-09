#include "log.h"

#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <share.h>
#include <string>

namespace mfglog
{
namespace
{
FILE* gFile = nullptr;
std::mutex gMutex;

void WriteLine(const wchar_t* message) noexcept
{
    std::lock_guard lock(gMutex);
    if (!gFile || !message)
        return;
    SYSTEMTIME now{};
    GetLocalTime(&now);
    fwprintf(gFile, L"[%02u:%02u:%02u.%03u] %s\n", now.wHour, now.wMinute,
        now.wSecond, now.wMilliseconds, message);
    fflush(gFile);
}
}

void Open(const wchar_t* executableDirectory) noexcept
{
    std::lock_guard lock(gMutex);
    if (gFile || !executableDirectory)
        return;
    std::wstring path = executableDirectory;
    if (!path.empty() && path.back() != L'\\' && path.back() != L'/')
        path.push_back(L'\\');
    path += L"RTX40MFG.log";
    // Shared read so the log can be tailed while the game runs.
    gFile = _wfsopen(path.c_str(), L"w, ccs=UTF-8", _SH_DENYWR);
}

void Close() noexcept
{
    std::lock_guard lock(gMutex);
    if (!gFile)
        return;
    fclose(gFile);
    gFile = nullptr;
}

void Write(const wchar_t* format, ...) noexcept
{
    if (!format)
        return;
    wchar_t message[1024]{};
    va_list args;
    va_start(args, format);
    _vsnwprintf_s(message, _countof(message), _TRUNCATE, format, args);
    va_end(args);
    WriteLine(message);
}

void WriteMessage(const wchar_t* message) noexcept
{
    WriteLine(message);
}
}
