#pragma once
#include "../../src/rt_ptlas_submission.h"
#include <memory>

// This fixture deliberately holds the GPU queue behind a CPU-signalled fence
// while all updates are submitted. It cannot accidentally pass by finishing
// each update before the next CPU recording. No game process is accessed.
static void QueuedValidation(GPU& g,ID3D12Resource* triangle){
 struct Recording {
  ComPtr<ID3D12CommandAllocator> allocator;
  ComPtr<ID3D12GraphicsCommandList4> list;
  ComPtr<ID3D12Resource> upload;
  std::unique_ptr<Probe> probe;
  Hits expected;
  UINT64 ticket=0,fence=0;
 };
 struct State {
  NvBackend backend;
  SubmissionJournal journal{4,1,1,1};
  ComPtr<ID3D12Resource> first,alternate;
  ComPtr<ID3D12Fence> gate;
  std::vector<std::unique_ptr<Recording>> recordings;
  explicit State(GPU& gpu):backend(gpu.device.Get(),4,1){}
 };
 auto state=std::make_unique<State>(g);state->first=triangle;state->alternate=MakeBLAS(g);
 Check(g.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&state->gate)),"queue gate");
 auto* gate=state->gate.Get();
 const auto fenceBefore=g.serial;bool submittedAny=false;
 Check(g.queue->Wait(gate,1),"hold update queue");
 try {
  const auto record=[&](const std::vector<Instance>& scene){
   auto r=std::make_unique<Recording>();r->probe=std::make_unique<Probe>(g);r->expected=AnalyticHits(scene);
   Check(g.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&r->allocator)),"queued allocator");
   Check(g.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,r->allocator.Get(),nullptr,IID_PPV_ARGS(&r->list)),"queued list");
   r->ticket=state->journal.Prepare(scene,std::vector<std::uint32_t>(scene.size()));
   // Retain before the first GPU command, including an uncertain submission.
   state->recordings.push_back(std::move(r));auto& owned=*state->recordings.back();
   owned.upload=state->backend.Record(owned.list.Get(),state->journal.Prepared(owned.ticket));
   if(!state->journal.Recorded(owned.ticket))throw std::runtime_error("journal record rejected");
   owned.probe->Record(owned.list.Get(),state->backend.Address());
   Check(owned.list->Close(),"queued close");
   if(!state->journal.Closed(owned.ticket))throw std::runtime_error("journal close rejected");
   return &owned;
  };
  const auto discard=[&](const std::vector<Instance>& scene){
   auto* r=record(scene);
   // An unsubmitted list may be reset; its allocator has no GPU users.
   Check(r->list->Reset(r->allocator.Get(),nullptr),"discard queued recording");
   if(!state->journal.DiscardAfterReset(r->ticket))throw std::runtime_error("discard rejected");
   Check(r->list->Close(),"close discarded list");state->recordings.pop_back();
  };
  std::vector<Instance> scene(4);
  for(unsigned i=0;i<4;++i){auto& v=scene[i];v.transform[0]=v.transform[5]=v.transform[10]=1;
   v.transform[3]=float(i)*2;v.idMask=0xff000007;v.contributionFlags=0x01000000;v.blas=triangle->GetGPUVirtualAddress();}
  discard(scene); // Discarded initialization must not turn the next build into update.
  unsigned noBuild=0;
  for(unsigned frame=0;frame<32;++frame){
   if(frame&&frame%3!=1){
    scene.resize(frame%7==6?0:1+frame%4);
    for(unsigned i=0;i<scene.size();++i){auto& v=scene[i];v={};v.transform[0]=v.transform[5]=v.transform[10]=1;
     v.transform[3]=float((i+frame)%8)*2;v.idMask=((i+frame)%5==0?0u:0xff000000u)|7;
     v.contributionFlags=0x01000000;v.blas=(frame%2?state->alternate:state->first)->GetGPUVirtualAddress();}
   }
   if(frame==8||frame==19)discard({}); // Discarded removals must not change submitted history.
   auto* r=record(scene);noBuild+=!r->upload;
   r->fence=++g.serial;
   if(!state->journal.BeginExecute(r->ticket,1,1,r->fence))throw std::runtime_error("queue admission rejected");
   ID3D12CommandList* lists[]={r->list.Get()};submittedAny=true;g.queue->ExecuteCommandLists(1,lists);
   const auto signaled=g.queue->Signal(g.fence.Get(),r->fence);
   if(!state->journal.Submitted(r->ticket,SUCCEEDED(signaled)))throw std::runtime_error("queued Signal failed; quarantined");
   state->backend.Submitted();
  }
  if(state->journal.InFlight()!=32||g.fence->GetCompletedValue()>fenceBefore||!noBuild)
   throw std::runtime_error("fixture did not retain 32 blocked submissions and unchanged updates");
  std::cout<<"QUEUED submissions=32 held_before_gpu_execution=32 discarded_recordings=3 unchanged_without_build="<<noBuild<<" per_update_cpu_waits=0\n";
  Check(gate->Signal(1),"release update queue");
  // One drain after the entire sequence; it also checks device/debug errors.
  g.Flush();
  if(!state->journal.Completed(1,1,state->journal.LastSubmitted())||state->journal.InFlight())throw std::runtime_error("retirement failed");
  unsigned checked=0;
  for(const auto& r:state->recordings){const auto hits=r->probe->Read();
   if(hits!=r->expected)throw std::runtime_error("queued PTLAS analytic hit mismatch at ticket "+std::to_string(r->ticket));
   checked+=static_cast<unsigned>(hits.size());
  }
  std::cout<<"PASS queued PTLAS ownership; updates=32 rays="<<checked<<" native_tlas_builds=0 per_update_cpu_waits=0 private_mesh_copies=0 final_drain=1 game_integration=0 performance_claim=0\n";
 }catch(...){
  // Release a blocked queue even on CPU validation/allocation failure. Retain
  // all GPU owners permanently if a final drain cannot prove safe retirement.
  const auto released=gate->Signal(1);
  if(submittedAny){try{Check(released,"release failed queue");g.Flush();}catch(...){(void)state.release();}}
  throw;
 }
}
