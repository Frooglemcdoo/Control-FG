#ifndef GI_MOTION_LAYOUT
#define GI_MOTION_LAYOUT
#include "gi_weight_transport.hlsl"
bool GIMotionEnabled(uint step,uint packed){return (step&33554432u)&&(packed&65536u);}
uint GIMotionBase(uint w,uint h,uint packed,uint rays){
 return GIWeightBase(w,h,packed,rays)+((w*h*rays*4u+31u)&~31u);
}
uint GIMotionAddress(uint2 p,uint ray,uint w,uint h,uint packed,uint rays){
 return GIMotionBase(w,h,packed,rays)+2048u+((p.y*w+p.x)*rays+ray)*224u;
}
#endif
