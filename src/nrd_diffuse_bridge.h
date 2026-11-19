#pragma once
#include <atomic>
#include <cstdint>
namespace control_nrd_bridge {
struct Specular {void (*produce)(void*) noexcept;bool (*ready)(std::uint64_t) noexcept;bool (*filter)() noexcept;};
inline std::atomic<const Specular*> specular{nullptr};
struct Producer {void (*begin)() noexcept;void (*end)() noexcept;void (*dispatch)() noexcept;};
inline std::atomic<const Producer*> producer{nullptr};
}
