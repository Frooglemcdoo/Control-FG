#pragma once
#include <atomic>
// Independent from the glass switch and the retired P1/P2 experiment.
inline std::atomic<bool> opaqueReflectionEnabled{true};
