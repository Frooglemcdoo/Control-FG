#include "gpu_test_device.h"
#include <d3d12sdklayers.h>
#include "../src/gi_reuse/d3d12_geometry_snapshot.h"
int main()try {
 ComPtr<ID3D12Debug> debug;Check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));debug->EnableDebugLayer();
 Device gpu(false);ComPtr<ID3D12InfoQueue> info;Check(gpu.device.As(&info));
 const std::vector<unsigned> old{1,2,3,4},changed{9,10,11,12};auto upload=gpu.Upload(old),replacement=gpu.Upload(changed);
 auto source=gpu.Buffer(16,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST);
 Require(gi_reuse::RetainCreatedStorage(source.Get()),"Source storage lease failed");
 gpu.list->CopyBufferRegion(source.Get(),0,upload.Get(),0,16);
 gpu.Transition(source.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
 auto budget=std::make_shared<PTOwnedScene::LeaseBudget>(64);auto pool=std::make_shared<GIGeometrySnapshot::Pool>();
 GIGeometrySnapshot::Source input{{1,source->GetGPUVirtualAddress(),16},source,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,4,10,{}};
 std::shared_ptr<GIGeometrySnapshot::Transfer> transfer;
 auto rejected=input;rejected.state=D3D12_RESOURCE_STATE_COPY_DEST;
 Require(!GIGeometrySnapshot::Record(gpu.list.Get(),{rejected},budget,transfer,pool),"Writable input state accepted");
 Require(budget->Held()==0&&!transfer,"Rejected snapshot retained storage");
 Require(GIGeometrySnapshot::Record(gpu.list.Get(),{input,input},budget,transfer,pool),"Snapshot recording failed");
 Require(transfer->sources.size()==1&&budget->Held()==12,"Shared source not deduplicated or R16 tail not padded");
 auto frozen=transfer->storage;auto copy=frozen->copies.at(1);
 gi_reuse::BufferStorageLease privateLease;Require(gi_reuse::AcquireStorageLease(copy.Get(),privateLease),"Private copy lacks backing lease");
 gpu.Transition(source.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
 gpu.list->CopyBufferRegion(source.Get(),0,replacement.Get(),0,16);
 gpu.Transition(source.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COPY_SOURCE);
 auto readback=gpu.Buffer(32,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 gpu.list->CopyBufferRegion(readback.Get(),0,copy.Get(),0,10);
 gpu.list->CopyBufferRegion(readback.Get(),16,source.Get(),0,16);
 gpu.Submit();void* mapped=nullptr;const D3D12_RANGE read{0,32};Check(readback->Map(0,&read,&mapped));
 Require(std::memcmp(mapped,reinterpret_cast<const unsigned char*>(old.data())+4,10)==0,"Later native update changed frozen geometry");
 Require(std::memcmp(static_cast<unsigned char*>(mapped)+16,changed.data(),16)==0,"Source update did not execute");
 const D3D12_RANGE none{};readback->Unmap(0,&none);
 transfer.reset();Require(budget->Held()==12,"Scene owner failed to retain frozen storage");
 copy.Reset();privateLease={};frozen.reset();Require(budget->Held()==0&&pool->Statistics().bytes==12,"Completed storage not returned to pool");
 auto reused=pool->Take(gpu.device.Get(),12);Require(reused&&pool->Statistics().reuses==1,"Idle snapshot allocation not reused");
 for(UINT64 i=0;i<info->GetNumStoredMessages();++i){SIZE_T size=0;Check(info->GetMessage(i,nullptr,&size));std::vector<unsigned char> bytes(size);
  auto* message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());Check(info->GetMessage(i,message,&size));
  if(message->Severity<=D3D12_MESSAGE_SEVERITY_ERROR)throw std::runtime_error(message->pDescription);
 }
 std::cout<<"PASS build-time geometry preservation: source mutation, narrow R16 tail, source state restoration, storage leases, retention and idle reuse\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
