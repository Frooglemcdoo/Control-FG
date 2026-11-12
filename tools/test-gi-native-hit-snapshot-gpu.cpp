#include "gpu_test_device.h"
#include <d3d12sdklayers.h>
#include "../src/gi_reuse/d3d12_native_hit_snapshot.h"
int main()try {
 ComPtr<ID3D12Debug> debug;Check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));debug->EnableDebugLayer();
 Device gpu(false);ComPtr<ID3D12InfoQueue> info;Check(gpu.device.As(&info));
 const std::vector<unsigned> initial{10,20,30,40,50,60,70,80},changed{1,2,3,4,5,6,7,8};
 auto upload=gpu.Upload(initial),replacement=gpu.Upload(changed);
 auto source=gpu.Buffer(32,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST);
 Require(gi_reuse::RetainCreatedStorage(source.Get()),"source backing lease");
 gi_reuse::BufferStorageLease lease;Require(gi_reuse::AcquireStorageLease(source.Get(),lease),"source acquisition");
 gpu.list->CopyBufferRegion(source.Get(),0,upload.Get(),0,32);
 gpu.Transition(source.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
 PTNativeBinding::Binding binding{};binding.indices=lease.address;binding.attributes=lease.address+16;
 binding.constants[1]=2;binding.constants[8]=10;binding.constants[9]=16;
 using Snapshot=gi_reuse::D3D12NativeHitSnapshot;
 Snapshot snapshot;std::vector<Snapshot::Source> sources{{1,lease,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE}};
 Require(FAILED(snapshot.Prepare(gpu.device.Get(),{binding},sources,256)),"logical bytes bypassed committed budget");
 Require(!snapshot.Storage()&&!snapshot.AllocationBytes(),"failed preparation published allocation");
 Check(snapshot.Prepare(gpu.device.Get(),{binding},sources,65536));
 Require(snapshot.AllocationBytes()==65536&&!snapshot.Resource(),"allocation/accounting before commands");
 auto* first=snapshot.Storage();Require(snapshot.Record(gpu.list.Get())&&!snapshot.Record(gpu.list.Get()),"duplicate recording");
 const auto rebased=snapshot.Bindings()[0];const auto privateBase=snapshot.Resource()->GetGPUVirtualAddress();
 Require(rebased.indices==privateBase&&rebased.attributes==privateBase+16,"shared source translation");
 gpu.Transition(source.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
 gpu.list->CopyBufferRegion(source.Get(),0,replacement.Get(),0,32);
 gpu.Transition(source.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COPY_SOURCE);
 gpu.Transition(snapshot.Resource(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
 auto readback=gpu.Buffer(64,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 gpu.list->CopyBufferRegion(readback.Get(),0,snapshot.Resource(),0,32);
 gpu.list->CopyBufferRegion(readback.Get(),32,source.Get(),0,32);gpu.Submit();
 void* mapped=nullptr;const D3D12_RANGE read{0,64};Check(readback->Map(0,&read,&mapped));
 Require(!std::memcmp(mapped,initial.data(),32),"native mutation corrupted private hit bytes");
 Require(!std::memcmp(static_cast<unsigned char*>(mapped)+32,changed.data(),32),"native mutation missing");
 const D3D12_RANGE none{};readback->Unmap(0,&none);
 // Submit has waited for GPU completion. Seal/reset the sole fixture recording
 // before rearming; live owners require their producer AND consumer seals.
 Check(gpu.allocator->Reset());Check(gpu.list->Reset(gpu.allocator.Get(),nullptr));
 Require(snapshot.RearmAfterRetirement()&&!snapshot.RearmAfterRetirement()&&!snapshot.Resource()&&snapshot.Bindings().empty(),"rearm state");
 Require(snapshot.AllocationBytes()==65536,"idle storage disappeared from budget");
 sources[0].state=D3D12_RESOURCE_STATE_COMMON;
 Check(snapshot.Prepare(gpu.device.Get(),{binding},sources,65536));
 Require(snapshot.Storage()==first,"unchanged size allocated again");Require(snapshot.Record(gpu.list.Get()),"second recording");
 gpu.Transition(snapshot.Resource(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
 gpu.list->CopyBufferRegion(readback.Get(),0,snapshot.Resource(),0,32);
 Check(gpu.list->Close());ID3D12CommandList* submitted[]={gpu.list.Get()};gpu.queue->ExecuteCommandLists(1,submitted);
 Check(gpu.queue->Signal(gpu.fence.Get(),2));HANDLE done=CreateEventW(nullptr,FALSE,FALSE,nullptr);
 Require(done!=nullptr,"second fence event");Check(gpu.fence->SetEventOnCompletion(2,done));
 const auto waited=WaitForSingleObject(done,15000);CloseHandle(done);Require(waited==WAIT_OBJECT_0,"second submission completion");
 Check(gpu.device->GetDeviceRemovedReason());
 Check(readback->Map(0,&read,&mapped));Require(!std::memcmp(mapped,changed.data(),32),"reused snapshot kept old content");readback->Unmap(0,&none);
 for(UINT64 i=0;i<info->GetNumStoredMessages();++i){SIZE_T size=0;Check(info->GetMessage(i,nullptr,&size));std::vector<unsigned char> bytes(size);
  auto* message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());Check(info->GetMessage(i,message,&size));
  if(message->Severity<=D3D12_MESSAGE_SEVERITY_ERROR)throw std::runtime_error(message->pDescription);
 }
 std::cout<<"PASS private native hit buffers: actual source mutation, packed views, committed budget, fresh retired reuse and debug clean. No DXR/image claim.\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
