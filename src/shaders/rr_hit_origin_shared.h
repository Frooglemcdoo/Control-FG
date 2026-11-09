#ifndef CONTROL_RR_HIT_ORIGIN_SHARED_H
#define CONTROL_RR_HIT_ORIGIN_SHARED_H
#include "rr_hit_distance_shared.h"
struct RRHDVector4 {float x;float y;float z;float w;};
// Exact sys_constants register order, not a guessed transpose of CameraSnapshot.
struct RRHDClipToView {RRHDVector4 column0;RRHDVector4 column1;RRHDVector4 column2;RRHDVector4 column3;};
struct RRHDPrimary {RRHDVector position;uint valid;};
bool RRHDFinite4(RRHDVector4 v){return RRHDIsFinite(v.x)&&RRHDIsFinite(v.y)&&RRHDIsFinite(v.z)&&RRHDIsFinite(v.w);}
// Native reflection raygen: (DispatchRaysIndex + 0.5) * g_vInvOutputRes,
// clip=(2*u-1,1-2*v,clipDepth,1), then g_mClipToView and divide by W.
// Consume the native matrix as supplied. Do not add/subtract NGX jitter again.
RRHDPrimary RRHDPrimaryFromClip(uint pixelX,uint pixelY,float invOutputX,float invOutputY,float depth,RRHDClipToView m){
 RRHDPrimary outValue;outValue.position.x=0;outValue.position.y=0;outValue.position.z=0;outValue.valid=0;
 if(!RRHDIsFinite(invOutputX)||!RRHDIsFinite(invOutputY)||invOutputX<=0||invOutputY<=0||
    !RRHDIsFinite(depth)||depth<0||depth>1||!RRHDFinite4(m.column0)||!RRHDFinite4(m.column1)||
    !RRHDFinite4(m.column2)||!RRHDFinite4(m.column3))return outValue;
 float x=(float(pixelX)+0.5f)*invOutputX*2.0f-1.0f;
 float y=1.0f-(float(pixelY)+0.5f)*invOutputY*2.0f;
 float hx=((x*m.column0.x+y*m.column1.x)+depth*m.column2.x)+m.column3.x;
 float hy=((x*m.column0.y+y*m.column1.y)+depth*m.column2.y)+m.column3.y;
 float hz=((x*m.column0.z+y*m.column1.z)+depth*m.column2.z)+m.column3.z;
 float hw=((x*m.column0.w+y*m.column1.w)+depth*m.column2.w)+m.column3.w;
 if(!RRHDIsFinite(hw)||hw==0)return outValue;
 float inverseW=1.0f/hw;
 RRHDVector position;position.x=hx*inverseW;position.y=hy*inverseW;position.z=hz*inverseW;
 if(!RRHDIsFinite(position.x)||!RRHDIsFinite(position.y)||!RRHDIsFinite(position.z))return outValue;
 outValue.position=position;outValue.valid=1;return outValue;
}
#endif
