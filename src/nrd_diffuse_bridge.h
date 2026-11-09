#pragma once
#include <atomic>
namespace control_nrd_bridge {
struct Producer {void (*begin)() noexcept;void (*end)() noexcept;void (*dispatch)() noexcept;};
inline std::atomic<const Producer*> producer{nullptr};
}
