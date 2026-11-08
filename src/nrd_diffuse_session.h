#pragma once
#include <NRD.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

// Backend-independent RELAX session. Owns the dispatch snapshot because NRD
// overwrites its returned pointers on the next GetComputeDispatches call.
// No live renderer hooks are installed by this header.
namespace control_nrd {
constexpr nrd::Identifier DiffuseId=1;
struct Dispatch {
 std::uint16_t pipeline=0,width=0,height=0;
 std::vector<nrd::ResourceDesc> resources;
 std::vector<std::uint8_t> constants;
};
struct Frame {
 std::uint64_t frame=0,view=0,producer=0;
 std::uint16_t width=0,height=0;
 nrd::CommonSettings settings{};
 // Only the boundary adapter can establish these facts. Never silently use
 // pre-RR view normals or a previous-frame guide at the earlier GI boundary.
 bool sameFrameGuides=false,worldNormals=false,linearViewDepth=false;
 bool unjitteredMotion=false,radianceContract=false,hitDistanceContract=false;
};
inline bool FiniteMatrix(const float* m) {
 for(unsigned i=0;i<16;++i)if(!std::isfinite(m[i]))return false;
 // A real projection can have a zero diagonal entry. Use determinant, not
 // the diagonal, to reject empty/singular matrices.
 double a[4][4];for(unsigned y=0;y<4;++y)for(unsigned x=0;x<4;++x)a[y][x]=m[x*4+y];
 for(unsigned col=0;col<4;++col){
  unsigned pivot=col;for(unsigned y=col+1;y<4;++y)if(std::abs(a[y][col])>std::abs(a[pivot][col]))pivot=y;
  if(std::abs(a[pivot][col])<1e-12)return false;
  for(unsigned x=0;x<4;++x)std::swap(a[col][x],a[pivot][x]);
  for(unsigned y=col+1;y<4;++y){const double f=a[y][col]/a[col][col];for(unsigned x=col;x<4;++x)a[y][x]-=f*a[col][x];}
 }return true;
}
inline bool Valid(const Frame& f) {
 const auto&s=f.settings;
 if(!f.frame||!f.view||!f.width||!f.height||f.width>8192||f.height>8192||
    !f.sameFrameGuides||!f.worldNormals||!f.linearViewDepth||!f.unjitteredMotion||
    !f.radianceContract||!f.hitDistanceContract)return false;
 if(!FiniteMatrix(s.viewToClipMatrix)||!FiniteMatrix(s.worldToViewMatrix)||
    !FiniteMatrix(s.viewToClipMatrixPrev)||!FiniteMatrix(s.worldToViewMatrixPrev))return false;
 for(float v:s.cameraJitter)if(!std::isfinite(v)||std::abs(v)>.5f)return false;
 for(float v:s.cameraJitterPrev)if(!std::isfinite(v)||std::abs(v)>.5f)return false;
 for(float v:s.motionVectorScale)if(!std::isfinite(v))return false;
 for(float v:s.motionVectorBias)if(!std::isfinite(v))return false;
 return std::isfinite(s.timeDeltaBetweenFrames)&&s.timeDeltaBetweenFrames>0&&
        std::isfinite(s.denoisingRange)&&s.denoisingRange>0;
}
class Session {
 nrd::Instance* instance=nullptr;
 Frame last{},candidate{};
 bool pending=false,hasLast=false,needsReset=true;
 std::uint32_t index=0;
 std::uint64_t lastFence=0,queue=0;
 std::vector<Dispatch> plan;
 static void Check(nrd::Result result){if(result!=nrd::Result::SUCCESS)throw std::runtime_error("NRD API rejected the diffuse contract");}
public:
 Session(){
  const nrd::DenoiserDesc d{DiffuseId,nrd::Denoiser::RELAX_DIFFUSE};
  nrd::InstanceCreationDesc c{};c.denoisers=&d;c.denoisersNum=1;
  Check(nrd::CreateInstance(c,instance));
 }
 ~Session(){if(instance)nrd::DestroyInstance(*instance);}
 Session(const Session&)=delete;Session&operator=(const Session&)=delete;
 const nrd::InstanceDesc& Description()const{return *nrd::GetInstanceDesc(*instance);}
 const std::vector<Dispatch>& Plan()const{return plan;}
 bool Prepare(const Frame& f,std::uint64_t queueId){
  if(pending||!queueId||!Valid(f)||(queue&&queue!=queueId)||
     (hasLast&&f.frame<=last.frame))return false;
  auto s=f.settings;
  const bool restart=needsReset||!hasLast||f.frame!=last.frame+1||f.view!=last.view||
   f.producer!=last.producer||f.width!=last.width||f.height!=last.height;
  s.resourceSize[0]=s.rectSize[0]=f.width;s.resourceSize[1]=s.rectSize[1]=f.height;
  s.resourceSizePrev[0]=s.rectSizePrev[0]=hasLast?last.width:f.width;
  s.resourceSizePrev[1]=s.rectSizePrev[1]=hasLast?last.height:f.height;
  if(restart)s.accumulationMode=nrd::AccumulationMode::CLEAR_AND_RESTART;
  s.frameIndex=index;
  nrd::RelaxSettings relax{};
  // Fixed accumulation time avoids frame-rate-dependent temporal lag.
  const float frames=.25f*1000.f/s.timeDeltaBetweenFrames;
  relax.diffuseMaxAccumulatedFrameNum=std::uint32_t(std::fmin(60.f,std::fmax(2.f,frames)));
  relax.diffuseMaxFastAccumulatedFrameNum=std::max(1u,relax.diffuseMaxAccumulatedFrameNum/5u);
  relax.historyFixFrameNum=std::min(3u,relax.diffuseMaxFastAccumulatedFrameNum);
  relax.enableAntiFirefly=true;
  Check(nrd::SetCommonSettings(*instance,s));Check(nrd::SetDenoiserSettings(*instance,DiffuseId,&relax));
  const nrd::DispatchDesc* ds=nullptr;std::uint32_t count=0;
  Check(nrd::GetComputeDispatches(*instance,&DiffuseId,1,ds,count));
  plan.clear();plan.reserve(count);
  for(std::uint32_t i=0;i<count;++i){const auto& d=ds[i];
   Dispatch owned;owned.pipeline=d.pipelineIndex;owned.width=d.gridWidth;owned.height=d.gridHeight;
   if(d.resourcesNum)owned.resources.assign(d.resources,d.resources+d.resourcesNum);
   if(d.constantBufferDataSize)owned.constants.assign(d.constantBufferData,d.constantBufferData+d.constantBufferDataSize);
   plan.push_back(std::move(owned));
  }
  if(plan.empty())throw std::runtime_error("NRD returned no diffuse dispatches");
  candidate=f;queue=queueId;pending=true;needsReset=false;return true;
 }
 // The D3D12 adapter calls this only after actual Execute + successful Signal.
 bool Submitted(std::uint64_t queueId,std::uint64_t fence){
  if(!pending||queueId!=queue||fence<=lastFence)return false;
  lastFence=fence;last=candidate;pending=false;hasLast=true;++index;return true;
 }
 // A cancelled command list requires a full clear because CPU ping-pong state
 // was already advanced by GetComputeDispatches. Never pretend it executed.
 void CancelAfterReset(){pending=false;needsReset=true;plan.clear();}
};
}
