#include "gpu_test_device.h"
#include "../src/gi_reuse/d3d12_live_scene_owners.h"
int main()try {
    using namespace gi_reuse;Device gpu(false);D3D12LiveSceneOwners bank(4096);
    auto index=gpu.Upload(std::vector<unsigned>{0,1,2});
    auto position=gpu.Upload(std::vector<float>{0,0,0,1,0,0,0,1,0});
    for(auto* r:{index.Get(),position.Get()})Require(RetainCreatedStorage(r),"Storage tag failed");
    Require(bank.Created(1,index.Get(),index->GetGPUVirtualAddress(),12),"Index owner failed");
    Require(bank.Created(2,position.Get(),position->GetGPUVirtualAddress(),36),"Position owner failed");
    D3D12LiveSceneOwners::Identity before,after;
    Require(bank.Identify(index->GetGPUVirtualAddress(),12,before),"Owner lookup failed");
    Require(bank.Created(3,index.Get(),index->GetGPUVirtualAddress(),12),"Shared wrapper failed");
    Require(bank.Identify(index->GetGPUVirtualAddress(),12,after)&&before.token==after.token,"Shared resource created ambiguous identity");
    bank.Retire(1);Require(bank.Identify(index->GetGPUVirtualAddress(),12,after)&&before.token==after.token,"One wrapper retired shared storage");
    TriangleMetadata metadata{position->GetGPUVirtualAddress(),index->GetGPUVirtualAddress(),0,12,3,3,6,42,0};
    PTOwnedScene::Triangle witness{};Require(bank.Geometry(metadata,witness)&&witness.indexToken==before.token,"Build-time geometry witness failed");
    BufferStorageLease lease;Require(bank.Acquire(before.token,lease),"Lease acquisition failed");
    bank.Retire(3);Require(!bank.Identify(index->GetGPUVirtualAddress(),12,after),"Retired address still resolves");
    Require(lease.resource.Get()==index.Get()&&!bank.Acquire(before.token,lease),"Retired token reacquired or prior lease lost");
    Require(lease.resource.Get()==index.Get(),"Failed acquisition released existing dispatch lease");
    Require(bank.Created(4,index.Get(),index->GetGPUVirtualAddress(),12)&&bank.Identify(index->GetGPUVirtualAddress(),12,after)&&after.token!=before.token,"Reused address kept old token");
    Require(!bank.Created(4,nullptr,0,0)&&!bank.Identify(index->GetGPUVirtualAddress(),12,after),"Rejected replacement preserved stale wrapper");
    auto unknown=gpu.Upload(std::vector<unsigned>{1,2,3});
    Require(!bank.Created(5,unknown.Get(),unknown->GetGPUVirtualAddress(),12),"Unknown storage admitted");
    Require(bank.RetainedBytes()==36,"Retirement accounting incorrect");
    D3D12LiveSceneOwners small(4);Require(!small.Created(1,index.Get(),index->GetGPUVirtualAddress(),12),"Budget exceeded");
    std::cout<<"PASS live wrapper identities, shared ownership, exact build witnesses, retirement, generation reuse and lease preservation\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
