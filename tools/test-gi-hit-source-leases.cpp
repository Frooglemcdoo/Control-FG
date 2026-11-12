#include "gpu_test_device.h"
#include "../src/gi_reuse/d3d12_live_scene_owners.h"
#include <array>
int main()try {
 Device d(false);auto source=d.Buffer(256,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COMMON);
 Require(gi_reuse::RetainCreatedStorage(source.Get()),"source backing registration");
 std::array<unsigned char,0x50> wrapper{},alias{};
 auto* pointer=source.Get();const UINT64 address=source->GetGPUVirtualAddress(),bytes=256;
 UINT state=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
 const auto write=[&](auto& w){std::memcpy(w.data(),&pointer,8);std::memcpy(w.data()+0x20,&state,4);
  std::memcpy(w.data()+0x30,&bytes,8);std::memcpy(w.data()+0x48,&address,8);};write(wrapper);write(alias);
 gi_reuse::D3D12LiveSceneOwners owners;
 Require(owners.Created(reinterpret_cast<UINT64>(wrapper.data()),source.Get(),address,bytes),"wrapper creation");
 PTNativeBinding::Binding b{};b.indices=address;b.attributes=address+64;b.constants[1]=2;b.constants[8]=10;b.constants[9]=64;
 std::vector<gi_reuse::D3D12LiveSceneOwners::HitSource> leases;const char* reason=nullptr;
 Require(owners.AcquireHitSources({b,b},leases,&reason)&&leases.size()==1&&leases[0].storage.resource.Get()==source.Get(),"deduplicated original source lease");
 Require(leases[0].state==D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,"original native state");
 auto invalid=b;invalid.constants[9]=256;
 Require(!owners.AcquireHitSources({b,invalid},leases,&reason)&&leases.empty(),"partial batch published after missing range");
 const UINT64 shortView=80;std::memcpy(wrapper.data()+0x30,&shortView,8);
 Require(!owners.AcquireHitSources({b},leases,&reason),"native view shorter than resource allocation admitted");
 state=D3D12_RESOURCE_STATE_COPY_DEST;write(wrapper);
 Require(!owners.AcquireHitSources({b},leases,&reason)&&leases.empty(),"writable source admitted");
 state=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;write(wrapper);
 Require(owners.Created(reinterpret_cast<UINT64>(alias.data()),source.Get(),address,bytes),"alias registration");
 state=D3D12_RESOURCE_STATE_COMMON;write(alias);
 Require(!owners.AcquireHitSources({b},leases,&reason),"disagreeing native wrapper state admitted");
 owners.Retire(reinterpret_cast<UINT64>(alias.data()));
 const UINT64 moved=address+256;std::memcpy(wrapper.data()+0x48,&moved,8);
 Require(!owners.AcquireHitSources({b},leases,&reason),"stale wrapper address admitted");
 state=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;write(wrapper);
 Require(owners.AcquireHitSources({b},leases,&reason),"restored original wrapper rejected");
 auto retained=leases[0].storage.resource;owners.Retire(reinterpret_cast<UINT64>(wrapper.data()));source.Reset();
 Require(!owners.AcquireHitSources({b},leases,&reason)&&leases.empty()&&retained->GetGPUVirtualAddress()==address,"retirement removed acquired backing or allowed new source");
 std::cout<<"PASS original hit-source leases: complete bounds, deduplication, native wrapper identity/read states, alias disagreement and retirement. No content/queue proof.\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
