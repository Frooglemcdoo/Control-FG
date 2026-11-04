#pragma once
#include <cstdint>
#include <algorithm>
namespace gi_reuse {
// Keep ray/request/sample working storage bounded independently of resolution.
// Full-frame outputs remain in the native textures; only the working tile reuses
// storage. Constants for each tile are immutable after command recording.
struct GIFrameTiles {
    std::uint32_t first=0,pixels=0,rays=0,capacity=0,tiles=0;
    struct Span {std::uint32_t first=0,pixels=0,samples=0;};
    static bool Make(std::uint32_t width,std::uint32_t height,std::uint32_t first,
                     std::uint32_t pixels,std::uint32_t rays,GIFrameTiles& out) noexcept {
        out={};const auto size=std::uint64_t(width)*height;
        if(!width||!height||size>16777216||!pixels||first>=size||pixels>size-first||!rays||rays>64)return false;
        const auto capacity=(std::min)(pixels,(std::min)(65536u,262144u/rays));
        out={first,pixels,rays,capacity,(pixels+capacity-1)/capacity};return true;
    }
    Span At(std::uint32_t tile) const noexcept {
        if(tile>=tiles||!capacity)return {};
        const auto offset=tile*capacity,count=(std::min)(capacity,pixels-offset);
        return {first+offset,count,count*rays};
    }
};
}
