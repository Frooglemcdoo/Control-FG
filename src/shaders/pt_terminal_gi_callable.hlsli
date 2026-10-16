#ifndef CONTROL_PT_TERMINAL_GI_CALLABLE
#define CONTROL_PT_TERMINAL_GI_CALLABLE
#include "pt_terminal_gi.hlsli"
// The same parameter layout is used at the sole call site and export. This
// avoids inlining volume traversal into the ray-generation/TraceRay loop.
struct PTGICallParameters { float3 position;float3 normal;float3 radiance; };
[shader("callable")]
void PTTerminalGICallable(inout PTGICallParameters data){
    data.radiance=PTTerminalGI(data.position,data.normal);
}
float3 PTCallTerminalGI(float3 position,float3 normal){
    PTGICallParameters data;data.position=position;data.normal=normal;data.radiance=0;
    CallShader(0,data);
    return data.radiance;
}
#endif
