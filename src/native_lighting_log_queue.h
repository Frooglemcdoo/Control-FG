#pragma once
#include <windows.h>
#include <atomic>
#include <cstring>

namespace native_lighting {
// Render-side producers never wait for a lock or perform file I/O. Overflow or
// contention loses diagnostic lines, not frames; the consumer reports the loss.
class DiagnosticLogQueue {
    static constexpr size_t kCapacity = 256 * 1024;
    SRWLOCK lock_ = SRWLOCK_INIT;
    char bytes_[kCapacity]{};
    size_t used_ = 0;
    std::atomic<unsigned long long> dropped_{0};
public:
    bool Push(const char* bytes, size_t length) noexcept {
        if (!bytes || !length) return false;
        if (!TryAcquireSRWLockExclusive(&lock_)) { ++dropped_; return false; }
        const bool fits = length <= kCapacity - used_;
        if (fits) { memcpy(bytes_ + used_, bytes, length); used_ += length; }
        else ++dropped_;
        ReleaseSRWLockExclusive(&lock_);
        return fits;
    }
    // Called only by the UI consumer. The destination is allocated once there.
    size_t Drain(char* destination, size_t capacity) noexcept {
        AcquireSRWLockExclusive(&lock_);
        const size_t length = used_ <= capacity ? used_ : 0;
        if (length) { memcpy(destination, bytes_, length); used_ = 0; }
        ReleaseSRWLockExclusive(&lock_);
        return length;
    }
    unsigned long long TakeDropped() noexcept { return dropped_.exchange(0); }
    static constexpr size_t Capacity() noexcept { return kCapacity; }
};
}
