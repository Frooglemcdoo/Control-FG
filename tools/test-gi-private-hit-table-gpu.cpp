#include "gpu_test_device.h"
#include <d3d12sdklayers.h>
#include "../src/gi_reuse/d3d12_sbt_snapshot.h"
int main()try {
 ComPtr<ID3D12Debug> debug;Check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));debug->EnableDebugLayer();
 Device d(false);ComPtr<ID3D12InfoQueue> info;Check(d.device.As(&info));
 std::vector<unsigned char> bytes(512);for(std::size_t i=0;i<bytes.size();++i)bytes[i]=static_cast<unsigned char>(i*7+3);
 auto source=d.Upload(bytes);const auto base=source->GetGPUVirtualAddress();
 D3D12_DISPATCH_RAYS_DESC native{};native.Width=64;native.Height=32;native.Depth=1;
 native.RayGenerationShaderRecord={base,32};native.MissShaderTable={base+64,32,32};native.HitGroupTable={base+128,384,96};
 gi_reuse::D3D12GISbtSnapshot copy;Check(copy.Initialize(d.device.Get(),source.Get(),native));
 PTNativeBinding::Binding a{},b{};a.indices=0x10000;a.attributes=0x20000;b=a;b.type=1;b.attributes+=256;
 a.constants[8]=b.constants[8]=32;a.constants[9]=b.constants[9]=64;
 PTShaderTableAdapter::Identifier opaque{},alpha{};opaque[0]=7;alpha[0]=9;
 gi_reuse::NativeGITable table;Require(gi_reuse::BuildNativeGITable(31,42,1,{a,b},opaque,alpha,table),"native table reconstruction");
 Require(FAILED(copy.ReplaceHitTable(d.device.Get(),table,32,42,65536)),"wrong state object accepted");
 Require(FAILED(copy.ReplaceHitTable(d.device.Get(),table,31,43,65536)),"stale writer accepted");
 Require(FAILED(copy.ReplaceHitTable(d.device.Get(),table,31,42,384)),"logical-only upload budget accepted");
 auto bad=table;bad.active[1]=1;Require(FAILED(copy.ReplaceHitTable(d.device.Get(),bad,31,42,65536)),"reserved row activated");
 bad=table;bad.bytes[100]=1;Require(FAILED(copy.ReplaceHitTable(d.device.Get(),bad,31,42,65536)),"reserved row bytes accepted");
 Check(copy.ReplaceHitTable(d.device.Get(),table,31,42,65536));
 Require(copy.HitUploadAllocationBytes()==65536&&!copy.Resource(),"upload budget or premature publication");
 Require(FAILED(copy.ReplaceHitTable(d.device.Get(),table,31,42,65536)),"duplicate hit override");
 Require(copy.Record(d.list.Get()),"private table record");
 auto back=d.Buffer(1024,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 d.Transition(copy.Resource(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
 d.list->CopyBufferRegion(back.Get(),0,copy.Resource(),0,512);d.list->CopyBufferRegion(back.Get(),512,source.Get(),0,512);d.Submit();
 void* mapped=nullptr;D3D12_RANGE read{0,1024};Check(back->Map(0,&read,&mapped));auto* actual=static_cast<unsigned char*>(mapped);
 Require(!std::memcmp(actual,bytes.data(),128),"native raygen/miss region changed");
 Require(!std::memcmp(actual+128,table.bytes.data(),384),"private rebased hit table wrong");
 Require(!std::memcmp(actual+512,bytes.data(),512),"native table overwritten");D3D12_RANGE empty{0,0};back->Unmap(0,&empty);
 const auto upload=copy.HitUploadStorage()->GetGPUVirtualAddress();
 Check(d.allocator->Reset());Check(d.list->Reset(d.allocator.Get(),nullptr));
 Check(d.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(d.fence.ReleaseAndGetAddressOf())));
 Require(copy.RearmAfterRetirement(),"retired table rearm");Check(copy.Initialize(d.device.Get(),source.Get(),native));
 table.version=43;Check(copy.ReplaceHitTable(d.device.Get(),table,31,43,65536));
 Require(copy.HitUploadStorage()->GetGPUVirtualAddress()==upload,"retired upload unnecessarily reallocated");
 Require(copy.Record(d.list.Get()),"second table record");d.Submit();
 for(UINT64 i=0;i<info->GetNumStoredMessages();++i){SIZE_T size=0;Check(info->GetMessage(i,nullptr,&size));std::vector<unsigned char> data(size);
  auto* message=reinterpret_cast<D3D12_MESSAGE*>(data.data());Check(info->GetMessage(i,message,&size));
  if(message->Severity<=D3D12_MESSAGE_SEVERITY_ERROR)throw std::runtime_error(message->pDescription);
 }
 std::cout<<"PASS private hit table: original table untouched, raygen/miss preserved, rebased active rows, writer identity, committed budget, retired reuse. No DXR claim.\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
