#pragma once
#include <atomic>
namespace control_rr_temporal {
// Optional diagnostic PSO. Native motion/depth/camera-cut rejection remains in
// the shader; the history weight is not an equivalent fresh-ray sample count.
inline std::atomic<bool> ready{false};
inline constexpr unsigned kHistoryPercent=60;
}
