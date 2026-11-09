#pragma once
#ifdef __cplusplus
#include <cstdint>
namespace rt_lights {
using uint = std::uint32_t;
#endif
// One full-resolution interior plus a read-only reconstruction border.
struct ShadowTile {
    uint firstReceiver, receiverCount, frameTag, frameWidth;
    uint originX, originY, width, height;
    uint coreX, coreY, coreWidth, coreHeight;
    uint frameHeight, reserved0, reserved1, reserved2;
};
#ifdef __cplusplus
struct TileLayout {
    static constexpr uint Core = 128, Border = 10;
    static uint Count(uint width, uint height) { return ((width+Core-1)/Core)*((height+Core-1)/Core); }
    static ShadowTile At(uint index, uint width, uint height, uint tag) {
        const uint columns=(width+Core-1)/Core;
        ShadowTile t{};t.coreX=(index%columns)*Core;t.coreY=(index/columns)*Core;
        t.coreWidth=width-t.coreX<Core?width-t.coreX:Core;
        t.coreHeight=height-t.coreY<Core?height-t.coreY:Core;
        t.originX=t.coreX>Border?t.coreX-Border:0;t.originY=t.coreY>Border?t.coreY-Border:0;
        const uint right=t.coreX+t.coreWidth+Border,bottom=t.coreY+t.coreHeight+Border;
        t.width=(right<width?right:width)-t.originX;t.height=(bottom<height?bottom:height)-t.originY;
        t.frameWidth=width;t.frameHeight=height;t.frameTag=tag;
        t.firstReceiver=t.originY*width+t.originX;t.receiverCount=t.width*t.height;
        return t;
    }
};
static_assert(sizeof(ShadowTile)==64);
}
#endif
