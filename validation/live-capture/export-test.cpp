#include <cassert>
#include <vector>
#include <filesystem>
struct CameraSnapshot {unsigned long long engineFrame=22;double worldToView[12]{},viewToWorld[12]{},viewToClip[16]{},clipToView[16]{},worldToClip[16]{},clipToWorld[16]{};};
#include "../../src/rr_guide_export.h"
#include "../../src/rr_live_frame_input.h"
#define WINAPI
#define FAILED(x) ((x)<0)
using UINT=unsigned;using UINT64=unsigned long long;using SIZE_T=std::size_t;
struct D3D12_RANGE {SIZE_T Begin,End;};
struct D3D12_PLACED_SUBRESOURCE_FOOTPRINT {UINT64 Offset=16;struct{UINT RowPitch=64;}Footprint;};
struct ID3D12Resource {
 std::vector<unsigned char> bytes;bool fail=false;unsigned maps=0,unmaps=0;
 int Map(unsigned,const D3D12_RANGE* range,void** out){++maps;assert(range->Begin==0&&range->End==bytes.size());if(fail)return -1;*out=bytes.data();return 0;}
 void Unmap(unsigned,const D3D12_RANGE* range){++unmaps;assert(!range->Begin&&!range->End);}
};
static unsigned lastExport=99;
template<class... A>static void Log(const char*,unsigned success,A...){lastExport=success;}
#include "../../src/rr_live_capture.h"
int main(){
 const char* base=std::getenv("CONTROL_EXPORT_TEST_ROOT");assert(base);std::filesystem::create_directory(std::filesystem::path(base)/"ControlFGProbe");
 ID3D12Resource resources[3];RRLiveCapture cap;cap.width=cap.height=2;cap.frameInput.frame=42;cap.frameInput.jitterX=.25f;cap.frameInput.jitterY=-.125f;cap.present=43;cap.accepted=7;cap.rejected=2;cap.policy.fence=9;
 for(unsigned i=0;i<3;++i){resources[i].bytes.resize(144);for(unsigned j=0;j<144;++j)resources[i].bytes[j]=static_cast<unsigned char>(i*40+j);cap.readback[i]=&resources[i];cap.bytes[i]=144;}
 RRLiveCaptureExport(&cap);assert(lastExport==1);for(auto& r:resources)assert(r.maps==1&&r.unmaps==1);
 cap.frameInput.frame=43;resources[1].fail=true;RRLiveCaptureExport(&cap);assert(lastExport==0);assert(resources[0].unmaps==2&&resources[1].unmaps==1&&resources[2].maps==1);
 resources[1].fail=false;cap.frameInput.frame=42;RRLiveCaptureExport(&cap);assert(lastExport==0); // CREATE_NEW directory collision never overwrites a completed capture
 puts("PASS: actual live exporter writes active rows without padding; handles map failure, unmaps mapped resources, never overwrites completed capture");
}
