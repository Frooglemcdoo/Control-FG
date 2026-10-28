#pragma once
#include <windows.h>
#include <atomic>
#include <cwchar>

namespace native_lighting {
// Caller serializes access. Each instance owns one session file; never truncates
// an earlier report. No dependency on the game's install-directory permissions.
class SessionReport {
    HANDLE file_ = INVALID_HANDLE_VALUE;
    wchar_t path_[MAX_PATH]{};
    DWORD error_ = ERROR_SUCCESS;
    inline static std::atomic<unsigned> serial_{0};

    bool Directory(const wchar_t* path) noexcept {
        if (CreateDirectoryW(path, nullptr)) return true;
        const DWORD error = GetLastError();
        if (error == ERROR_ALREADY_EXISTS &&
            (GetFileAttributesW(path) & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
            GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES) return true;
        error_ = error;
        return false;
    }
    bool TryRoot(const wchar_t* root) noexcept {
        if (!root || !*root) { error_ = ERROR_PATH_NOT_FOUND; return false; }
        if (wcslen(root) > MAX_PATH - 120) { error_ = ERROR_FILENAME_EXCED_RANGE; return false; }
        wchar_t directory[MAX_PATH]{};
        if (swprintf_s(directory, L"%s\\Control-FG", root) < 0) { error_ = ERROR_FILENAME_EXCED_RANGE; return false; }
        if (!Directory(directory)) return false;
        if (swprintf_s(directory, L"%s\\Control-FG\\Diagnostics", root) < 0) { error_ = ERROR_FILENAME_EXCED_RANGE; return false; }
        if (!Directory(directory)) return false;
        SYSTEMTIME time{}; GetLocalTime(&time);
        for (unsigned attempt = 0; attempt != 16; ++attempt) {
            if (swprintf_s(path_, L"%s\\Lighting-R191-%04u%02u%02u-%02u%02u%02u-%lu-%u.log", directory,
                unsigned(time.wYear), unsigned(time.wMonth), unsigned(time.wDay), unsigned(time.wHour),
                unsigned(time.wMinute), unsigned(time.wSecond), GetCurrentProcessId(), serial_.fetch_add(1)) < 0) {
                error_ = ERROR_FILENAME_EXCED_RANGE; return false;
            }
            file_ = CreateFileW(path_, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file_ != INVALID_HANDLE_VALUE) { error_ = ERROR_SUCCESS; return true; }
            error_ = GetLastError();
            if (error_ != ERROR_FILE_EXISTS) break;
        }
        path_[0] = L'\0';
        return false;
    }
public:
    ~SessionReport() { if (file_ != INVALID_HANDLE_VALUE) CloseHandle(file_); }
    SessionReport() = default;
    SessionReport(const SessionReport&) = delete;
    SessionReport& operator=(const SessionReport&) = delete;
    bool Open(const wchar_t* preferred, const wchar_t* fallback) noexcept {
        return file_ != INVALID_HANDLE_VALUE || TryRoot(preferred) || TryRoot(fallback);
    }
    bool OpenDefault() noexcept {
        wchar_t local[MAX_PATH]{}, temporary[MAX_PATH]{};
        const DWORD localSize = GetEnvironmentVariableW(L"LOCALAPPDATA", local, MAX_PATH);
        if (!localSize || localSize >= MAX_PATH) local[0] = L'\0';
        const DWORD tempSize = GetTempPathW(MAX_PATH, temporary);
        if (!tempSize || tempSize >= MAX_PATH) temporary[0] = L'\0';
        return Open(local, temporary);
    }
    bool Write(const char* bytes, DWORD length) noexcept {
        if (file_ == INVALID_HANDLE_VALUE) { error_ = ERROR_INVALID_HANDLE; return false; }
        DWORD written = 0;
        if (!WriteFile(file_, bytes, length, &written, nullptr)) { error_ = GetLastError(); return false; }
        if (written != length) { error_ = ERROR_WRITE_FAULT; return false; }
        error_ = ERROR_SUCCESS;
        return true;
    }
    bool Flush() noexcept {
        if (file_ == INVALID_HANDLE_VALUE) { error_ = ERROR_INVALID_HANDLE; return false; }
        if (!FlushFileBuffers(file_)) { error_ = GetLastError(); return false; }
        error_ = ERROR_SUCCESS;
        return true;
    }
    const wchar_t* Path() const noexcept { return path_; }
    DWORD Error() const noexcept { return error_; }
};
} // namespace native_lighting

