#pragma once
#include <atomic>
#include <cstdint>
namespace gi_path_capture {
// Written by the UI before enabled is published. Jobs snapshot all settings.
inline std::atomic<bool> enabled{false};
inline wchar_t directory[260]{};
inline std::atomic<unsigned> condition{0}; // 1 raw, 2 Stable
inline std::atomic<unsigned> validFrames[3]{},invalidFrames[3]{};
inline constexpr unsigned stride=4;
inline unsigned Extent(unsigned n,unsigned step) noexcept {return (n+step-1)/step;}
}
