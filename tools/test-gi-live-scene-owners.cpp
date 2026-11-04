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
    BufferStorageLease inspected;std::uint64_t wrapper=0;
    Require(bank.Inspect(before.token,inspected,wrapper)&&wrapper==1,"Initial reverse wrapper lookup failed");
    bank.Retire(1);
    Require(bank.Inspect(before.token,inspected,wrapper)&&wrapper==3,"Retired wrapper remained in reverse lookup");
    Require(inspected.address==index->GetGPUVirtualAddress()&&inspected.bytes==12,"Retained description mismatch");
    Require(bank.Identify(index->GetGPUVirtualAddress(),12,after)&&before.token==after.token,"One wrapper retired shared storage");
    TriangleMetadata metadata{position->GetGPUVirtualAddress(),index->GetGPUVirtualAddress(),0,12,3,3,6,42,0};
    PTOwnedScene::Triangle witness{};Require(bank.Geometry(metadata,witness)&&witness.indexToken==before.token,"Build-time geometry witness failed");
    // Exercise the real owner -> ordered scene -> private SBT preparation join.
    // These AS stand-ins are CPU metadata fixtures, never GPU traversal inputs.
    auto attributes=gpu.Upload(std::vector<unsigned>(12));
    auto top=gpu.Upload(std::vector<unsigned>(64)),bottom=gpu.Upload(std::vector<unsigned>(64));
    for(auto* r:{attributes.Get(),top.Get(),bottom.Get()})Require(RetainCreatedStorage(r),"Fixture storage tag failed");
    Require(bank.Created(10,attributes.Get(),attributes->GetGPUVirtualAddress(),48)&&
        bank.Created(11,top.Get(),top->GetGPUVirtualAddress(),256)&&bank.Created(12,bottom.Get(),bottom->GetGPUVirtualAddress(),256),"Scene fixture owners failed");
    D3D12LiveSceneOwners::Identity topID,bottomID;
    Require(bank.Identify(top->GetGPUVirtualAddress(),256,topID)&&bank.Identify(bottom->GetGPUVirtualAddress(),256,bottomID),"AS fixture identities failed");
    auto recording=std::make_shared<OrderedASRecording>();recording->generation=1;
    recording->events.push_back({{ASOperation::Build,{bottomID.address,0,0,1,false,GeometryMetadata::Create({metadata}),false},0},bottomID.token,0});
    recording->events.push_back({{ASOperation::Build,{topID.address,0,12288,1,true,{},false},0},topID.token,0});
    OrderedASMetadata<> history;Require(history.Submit(1,{recording}),"Scene history failed");const auto cut=history.Mark(1);
    auto instances=std::make_shared<std::vector<unsigned char>>(64);
    const float transform[12]={1,0,0,0,0,1,0,0,0,0,1,0};std::memcpy(instances->data(),transform,48);
    const unsigned mask=0xff000000;std::memcpy(instances->data()+48,&mask,4);std::memcpy(instances->data()+56,&bottomID.address,8);
    SceneByteView bytes{12288,999,7,instances};
    PTShaderTableAdapter::Exports exports;exports.nativeOpaque.fill(1);exports.nativeAlpha.fill(2);exports.privateOpaque.fill(3);exports.privateAlpha.fill(4);
    PTNativeBinding::Binding binding{};binding.indices=index->GetGPUVirtualAddress();binding.attributes=attributes->GetGPUVirtualAddress();
    const unsigned constants[10]={0,4,16,1,0,4,12,0,12,48};std::memcpy(binding.constants,constants,sizeof(constants));
    NativeGITable native;Require(BuildNativeGITable(101,102,1,{binding},exports.nativeOpaque,exports.nativeAlpha,native),"Native table fixture failed");
    D3D12LiveSceneOwners::Lease admitted;
    Require(bank.Prepare(history,cut,7,{topID.address,topID.token,2},bytes,{{bottomID.address,bottomID.token,1}},
        {{{bottomID.address,bottomID.token,1},0,witness}},native,{binding},103,exports,admitted)==GISceneProducerError::None,"Complete scene handoff failed");
    Require(admitted.plan.table.bound.size()==1&&admitted.plan.table.bound[0]==1&&admitted.buffers.size()==5,"Scene closure ownership incomplete");
    bank.Retire(10);bank.Retire(11);bank.Retire(12);
    Require(bank.Prepare(history,cut,7,{topID.address,topID.token,2},bytes,{{bottomID.address,bottomID.token,1}},
        {{{bottomID.address,bottomID.token,1},0,witness}},native,{binding},103,exports,admitted)!=GISceneProducerError::None,"Retired scene re-admitted");
    Require(admitted.buffers.size()==5,"Rejection lost prior submitted ownership");
    BufferStorageLease lease;Require(bank.Acquire(before.token,lease),"Lease acquisition failed");
    bank.Retire(3);Require(!bank.Identify(index->GetGPUVirtualAddress(),12,after),"Retired address still resolves");
    Require(lease.resource.Get()==index.Get()&&!bank.Acquire(before.token,lease),"Retired token reacquired or prior lease lost");
    Require(lease.resource.Get()==index.Get(),"Failed acquisition released existing dispatch lease");
    Require(bank.Created(4,index.Get(),index->GetGPUVirtualAddress(),12)&&bank.Identify(index->GetGPUVirtualAddress(),12,after)&&after.token!=before.token,"Reused address kept old token");
    Require(!bank.Created(4,nullptr,0,0)&&!bank.Identify(index->GetGPUVirtualAddress(),12,after),"Rejected replacement preserved stale wrapper");
    auto unknown=gpu.Upload(std::vector<unsigned>{1,2,3});
    Require(!bank.Created(5,unknown.Get(),unknown->GetGPUVirtualAddress(),12),"Unknown storage admitted");
    Require(bank.RetainedBytes()==36,"Retirement accounting incorrect");
    D3D12LiveSceneOwners limitedBank(4);Require(!limitedBank.Created(1,index.Get(),index->GetGPUVirtualAddress(),12),"Budget exceeded");
    std::cout<<"PASS live wrapper identities, shared ownership, exact build witnesses, retirement, generation reuse and lease preservation\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
