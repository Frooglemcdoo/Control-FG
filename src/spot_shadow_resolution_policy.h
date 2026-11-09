#pragma once
#include <cstdint>

namespace native_lighting::spot_resolution {
struct Extent { int width, height; };
inline constexpr int kTextureLimit = 16384;
// Preserve atlas width; four times the height provides area for twice-sized
// square tiles without exceeding D3D12's maximum texture width.
inline bool ExpandAtlas(Extent native, Extent& expanded) noexcept {
    if ((native.width != 8192 && native.width != 16384) ||
        (native.height != 1024 && native.height != 2048 && native.height != 4096)) return false;
    expanded = {native.width, native.height * 4};
    return expanded.height <= kTextureLimit;
}
inline Extent TileRequest(Extent native, bool enabled, Extent atlas) noexcept {
    if (!enabled || native.width <= 0 || native.width != native.height ||
        (native.width & (native.width - 1)) != 0 ||
        native.width > atlas.width / 2 || native.height > atlas.height / 2) return native;
    return {native.width * 2, native.height * 2};
}
} // namespace native_lighting::spot_resolution
