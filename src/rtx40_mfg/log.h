#pragma once

#include <windows.h>

namespace mfglog
{
// Opens RTX40MFG.log beside the running executable. Safe to call once.
void Open(const wchar_t* executableDirectory) noexcept;
void Close() noexcept;
void Write(const wchar_t* format, ...) noexcept;

// Adapter for components that publish a plain wide-string callback.
void WriteMessage(const wchar_t* message) noexcept;
}
