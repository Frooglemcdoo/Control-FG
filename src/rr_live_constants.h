#pragma once
// Owned guide-pass ABI, independent of the game's native root signatures.
struct RRLiveConstants {
    unsigned invertSmoothness=1, guideFlags=0, width=0, height=0;
    float projectionX=0, projectionY=0, reservedX=0, reservedY=0;
};
static_assert(sizeof(RRLiveConstants)==32, "Live guide constant ABI");
inline constexpr unsigned kRRLiveConstantCount=sizeof(RRLiveConstants)/sizeof(unsigned);
