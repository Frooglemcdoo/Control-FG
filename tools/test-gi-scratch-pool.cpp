#include "gpu_test_device.h"
#include <d3d12sdklayers.h>
#include "../src/gi_reuse/d3d12_gi_scratch_pool.h"
int main()try {
 ComPtr<ID3D12Debug> debug;Check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));debug->EnableDebugLayer();
 Device gpu(false);ComPtr<ID3D12InfoQueue> messages;Check(gpu.device.As(&messages));
 gi_reuse::GIScratchPool pool;
 auto scratch=gpu.Buffer(256,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COMMON,true);
 const auto identity=scratch.Get();
 for(unsigned pass=0;pass<2;++pass){
  Require(!pool.Take(gpu.device.Get(),256,D3D12_HEAP_TYPE_DEFAULT),"Active buffer became available");
  std::vector<unsigned> values(64,pass+17);auto source=gpu.Upload(values);
  auto readback=gpu.Buffer(256,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
  gpu.Transition(scratch.Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_DEST);
  gpu.list->CopyBufferRegion(scratch.Get(),0,source.Get(),0,256);
  gpu.Transition(scratch.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COPY_SOURCE);
  gpu.list->CopyBufferRegion(readback.Get(),0,scratch.Get(),0,256);
  gpu.Transition(scratch.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COMMON);
  gpu.Submit();void* data=nullptr;Check(readback->Map(0,nullptr,&data));
  Require(!std::memcmp(data,values.data(),256),"Reused storage contains stale results");readback->Unmap(0,nullptr);
  pool.Return(std::move(scratch),D3D12_HEAP_TYPE_DEFAULT);
  Require(!pool.Take(gpu.device.Get(),512,D3D12_HEAP_TYPE_DEFAULT),"Wrong size reused");
  Require(!pool.Take(gpu.device.Get(),256,D3D12_HEAP_TYPE_UPLOAD),"Wrong heap type reused");
  scratch=pool.Take(gpu.device.Get(),256,D3D12_HEAP_TYPE_DEFAULT);Require(scratch.Get()==identity,"Retired buffer not reused");
  if(!pass){Check(gpu.allocator->Reset());Check(gpu.list->Reset(gpu.allocator.Get(),nullptr));gpu.fence.Reset();Check(gpu.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&gpu.fence)));}
 }
 for(UINT64 i=0;i<messages->GetNumStoredMessages();++i){SIZE_T n=0;Check(messages->GetMessage(i,nullptr,&n));std::vector<unsigned char> bytes(n);
  auto* message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());Check(messages->GetMessage(i,message,&n));Require(message->Severity>D3D12_MESSAGE_SEVERITY_ERROR,"Scratch reuse state error");}
 std::cout<<"PASS scratch pool: active exclusion, exact size/heap key, fence-completed reuse, overwritten bytes and COMMON state on WARP\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
