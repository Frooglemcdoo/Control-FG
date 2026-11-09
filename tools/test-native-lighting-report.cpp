#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../src/native_lighting_report.h"
#include <string>
#include <stdexcept>
#include <iostream>
#include <cstring>
static void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
static std::string Read(const wchar_t* path) {
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    Require(file != INVALID_HANDLE_VALUE, "cannot reopen report");
    char data[128]{}; DWORD count = 0;
    const bool ok = ReadFile(file, data, sizeof(data), &count, nullptr) != FALSE;
    CloseHandle(file); Require(ok, "read failed"); return std::string(data, count);
}
int main() try {
    wchar_t temp[MAX_PATH]{}, root[MAX_PATH]{}, blocked[MAX_PATH]{};
    Require(GetTempPathW(MAX_PATH, temp) > 0, "temp unavailable");
    swprintf_s(root, L"%sReportTest-%lu-%llu", temp, GetCurrentProcessId(), GetTickCount64());
    Require(CreateDirectoryW(root, nullptr) != FALSE, "test directory failed");
    swprintf_s(blocked, L"%s\\blocked", root);
    HANDLE file = CreateFileW(blocked, GENERIC_WRITE, 0, nullptr, CREATE_NEW, 0, nullptr);
    Require(file != INVALID_HANDLE_VALUE, "blocker failed"); CloseHandle(file);
    std::wstring firstPath, secondPath;
    {
        native_lighting::SessionReport first;
        Require(first.Open(blocked, root), "fallback did not open");
        firstPath = first.Path();
        Require(first.Write("selection\n", 10), "first write failed");
        Require(first.Open(blocked, root), "reopen failed");
        Require(first.Write("result\n", 7), "second write failed");
        Require(first.Flush(), "explicit flush failed");
        Require(Read(first.Path()) == "selection\nresult\n", "earlier events lost or not flushed");
        native_lighting::SessionReport second;
        Require(second.Open(root, root), "second session failed");
        secondPath = second.Path();
        Require(firstPath != secondPath, "session overwritten");
        Require(Read(first.Path()) == "selection\nresult\n", "old log changed");
        native_lighting::SessionReport failure;
        Require(!failure.Open(blocked, blocked) && failure.Error() != ERROR_SUCCESS, "failure hidden");
    }
    DeleteFileW(firstPath.c_str()); DeleteFileW(secondPath.c_str()); DeleteFileW(blocked);
    RemoveDirectoryW((std::wstring(root) + L"\\Control-FG\\Diagnostics").c_str());
    RemoveDirectoryW((std::wstring(root) + L"\\Control-FG").c_str()); RemoveDirectoryW(root);
    std::cout << "PASS: fallback, flushed readable log, retained events, unique sessions, explicit failure\n";
    return 0;
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
