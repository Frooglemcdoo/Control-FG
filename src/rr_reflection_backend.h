#pragma once
#include "rr_reflection_capture.h"
#include "rr_memory_budget.h"

namespace control_rr_reflection {
// Three full-array material/position pairs, admitted against current DXGI usage.
enum class PrepareFailure {None,InvalidState,InvalidShape,AllocationInfo,BudgetQuery,Budget,Create};
struct PrepareReport {
 PrepareFailure failure=PrepareFailure::None;
 std::uint64_t budgetBytes=0,resourceBytes[2]{},requiredBytes=0;
 control_rr::MemoryBudget memory{};
 unsigned failedResource=0; // one-based query (1..2) or creation (1..6)
};
inline const char* PrepareFailureName(PrepareFailure failure) noexcept {
 switch(failure){
 case PrepareFailure::None:return "none";
 case PrepareFailure::InvalidState:return "invalid_prepare_state";
 case PrepareFailure::InvalidShape:return "invalid_capture_shape";
 case PrepareFailure::AllocationInfo:return "allocation_info_failed";
 case PrepareFailure::BudgetQuery:return "gpu_memory_budget_query_failed";
 case PrepareFailure::Budget:return "available_gpu_memory_budget_exceeded";
 case PrepareFailure::Create:return "resource_allocation_failed";
 }
 return "unknown_prepare_failure";
}
// Api is a nonthrowing D3D12 leaf adapter. Owner serializes backend/capture calls.
// No destructor releases resources: Dispose requires independently verified idle.
// On ambiguous command, AddRef, or Signal failure, retain owner through exit.
template<class Api> class Backend {
 Api& api_;
 Address device_=0,queue_=0,fence_=0;
 Address destinations_[3][2]{},sources_[3][2]{};
 Shape shapes_[2]{};
 Context recording_{};
 std::size_t selected_=3;
 bool prepared_=false,poisoned_=false;
 std::uint64_t bytes_=0;
 PrepareReport prepareReport_{};
public:
 explicit Backend(Api& api):api_(api){}
 Backend(const Backend&)=delete;
 Backend& operator=(const Backend&)=delete;
 const auto& Destinations() const {return destinations_;}
 std::uint64_t AllocationBytes() const {return bytes_;}
 const PrepareReport& LastPrepare() const {return prepareReport_;}
 bool Poisoned() const {return poisoned_;}
 control_rr::FenceKey Key() const {return {device_,queue_,fence_};}
 // Worker-only; all six allocation sizes are validated before the first create.
 // device/queue/fence remain caller-owned and retained for this owner's lifetime.
 bool Prepare(Address device,Address queue,Address fence,const Shape& m,const Shape& p) {
  prepareReport_={};
  if(prepared_||poisoned_||!device||!queue||!fence){prepareReport_.failure=PrepareFailure::InvalidState;return false;}
  const Shape pair[2]{m,p};
  for(unsigned i=0;i<2;++i){const auto& s=pair[i];
   if(s.dimension!=3||s.width<64||s.width>8192||s.height<64||s.height>8192||
      !s.layers||s.layers>32||s.mips!=1||
      s.samples!=1||s.quality||(s.flags&(2u|8u|16u|32u))||s.format!=(i?10u:57u)){
    prepareReport_.failure=PrepareFailure::InvalidShape;return false;
   }
  }
  if(m.width!=p.width||m.height!=p.height||m.layers!=p.layers){prepareReport_.failure=PrepareFailure::InvalidShape;return false;}
  auto& size=prepareReport_.resourceBytes;
  for(unsigned i=0;i<2;++i){
   if(!api_.AllocationSize(device,pair[i],size[i])||!size[i]||size[i]==UINT64_MAX){
    prepareReport_.failure=PrepareFailure::AllocationInfo;prepareReport_.failedResource=i+1;return false;
   }
  }
  // Divide first: no addition or multiplication of untrusted sizes can overflow.
  if(size[0]>UINT64_MAX/3||size[1]>UINT64_MAX/3-size[0]){
   prepareReport_.requiredBytes=UINT64_MAX;prepareReport_.failure=PrepareFailure::Budget;return false;
  }
  prepareReport_.requiredBytes=3*(size[0]+size[1]);
  if(!api_.QueryBudget(device,prepareReport_.memory)){
   prepareReport_.failure=PrepareFailure::BudgetQuery;return false;
  }
  prepareReport_.budgetBytes=prepareReport_.memory.Allowance();
  if(prepareReport_.requiredBytes>prepareReport_.budgetBytes){prepareReport_.failure=PrepareFailure::Budget;return false;}
  for(unsigned i=0;i<6;++i){
   auto& destination=destinations_[i/2][i%2];
   if(!api_.Create(device,pair[i%2],destination)||!destination){
    prepareReport_.failure=PrepareFailure::Create;prepareReport_.failedResource=i+1;
    for(auto& slot:destinations_)for(auto& d:slot){if(d)api_.Release(d);d=0;}
    return false;
   }
  }
  device_=device;queue_=queue;fence_=fence;shapes_[0]=m;shapes_[1]=p;
  bytes_=prepareReport_.requiredBytes;prepared_=true;return true;
 }
 // Synchronous recording; api.Current must re-read native frame/list identity.
 bool SetRecording(const Context& context) {
  if(!prepared_||poisoned_||context.device!=device_||context.queue!=queue_||
     !context.frame||!context.view||!context.list||!api_.Current(context))return false;
  recording_=context;return true;
 }
 bool Current(const Context& c) {return !poisoned_&&Same(c,recording_)&&api_.Current(c);}
 bool Destination(Address d,const Shape& shape,Address device) {
  if(!prepared_||poisoned_||device!=device_||selected_>=3)return false;
  for(unsigned i=0;i<2;++i)if(destinations_[selected_][i]==d)return Same(shapes_[i],shape);
  return false;
 }
 void Select(std::size_t slot){selected_=slot;}
 bool HoldSources(Address material,Address position) {
  if(poisoned_||selected_>=3||!material||!position||material==position||
     sources_[selected_][0]||sources_[selected_][1])return false;
  const Address pair[2]{material,position};
  for(unsigned i=0;i<2;++i){
   // Store before AddRef: on an SEH fault the acquisition may have happened.
   sources_[selected_][i]=pair[i];
   if(!api_.Hold(pair[i])){poisoned_=true;return false;}
  }
  return true;
 }
 bool Barriers(const Transition* transitions,std::size_t count) {
  if(poisoned_||!transitions||count!=4)return false;
  if(!api_.Barriers(recording_,transitions,count)){poisoned_=true;return false;}
  return true;
 }
 bool Copy(Address destination,Address source) {
  if(poisoned_||!destination||!source)return false;
  if(!api_.Copy(recording_,destination,source)){poisoned_=true;return false;}
  return true;
 }
 void ReleaseSources(std::size_t slot) {
  if(poisoned_||slot>=3)return;
  for(auto& source:sources_[slot]){if(source)api_.Release(source);source=0;}
 }
 bool Signal(control_rr::FenceKey key,std::uint64_t value) {
  if(poisoned_||!control_rr::SameFence(key,Key())||!value||value==UINT64_MAX)return false;
  if(!api_.Signal(queue_,fence_,value)){poisoned_=true;return false;}return true;
 }
 std::uint64_t Completed(control_rr::FenceKey key) {
  if(poisoned_||!control_rr::SameFence(key,Key()))return UINT64_MAX;
  return api_.Completed(fence_);
 }
 // Call only after Capture::Idle and !Capture::Stopped; never on uncertain work.
 bool Dispose(bool verifiedIdle) {
  if(!verifiedIdle||poisoned_)return false;
  for(const auto& slot:sources_)for(auto source:slot)if(source)return false;
  for(auto& slot:destinations_)for(auto& d:slot){if(d)api_.Release(d);d=0;}
  prepared_=false;bytes_=0;device_=queue_=fence_=0;recording_={};selected_=3;return true;
 }
};
}
