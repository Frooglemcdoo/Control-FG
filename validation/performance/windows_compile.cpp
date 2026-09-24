#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <cstdint>
#include <new>
static LARGE_INTEGER frequency{};
static void Log(const char*,...) noexcept {}
struct RRGuideInputSnapshot {
 ID3D12GraphicsCommandList* commandList=nullptr;ID3D12CommandQueue* queue=nullptr;
 void* commandContext=nullptr;unsigned long long engineFrame=0,presentToken=0;
};
static bool RRGuideReadRendererContext(RRGuideInputSnapshot*,const char**) noexcept {return false;}
#include "../../src/rr_performance.h"
void CompilePerformanceAdapter(RRGuideInputSnapshot& in){
 RRPerfFrameMode(8,3840,2160,2560,1440,true,true,true,1);
 auto ticket=RRPerfBegin(in,RRPerfStage::NativeEvaluation);RRPerfEnd(ticket);RRPerfAfterPresent(1);
 RRPerfReplayPrepared(8,1);RRPerfReplayJoined(8,2,100);RRPerfFilter(8,1);
 RRPerfEvaluation(8,true,true,true,RRPerfClock(),1,1,0,0);
}
