#include "gpu_test_device.h"
#include "../src/gi_reuse/d3d12_submission_metadata.h"
namespace rt=gi_reuse::runtime;
static unsigned builds=0,copies=0,resets=0,initializes=0,barriers=0;
static void Build(ID3D12GraphicsCommandList4*,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*) noexcept {++builds;}
static void Copy(ID3D12GraphicsCommandList4*,UINT64,UINT64,D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE) noexcept {++copies;}
static void Reset(ID3D12GraphicsCommandList*) noexcept {++resets;}
static void Initialize(ID3D12GraphicsCommandList*) noexcept {++initializes;}
static void Barriers(ID3D12GraphicsCommandList*,UINT,const D3D12_RESOURCE_BARRIER*) noexcept {++barriers;}
int main()try {
 SetEnvironmentVariableA("CONTROLTOH_GI_METADATA","1");
 SetEnvironmentVariableA("CONTROLTOH_GI_LEGACY_AUDIT","0");
 Require(rt::Enabled()&&!rt::InventoryEnabled(),"Live-only configuration failed");
 Device gpu(false);auto buffer=gpu.Upload(std::vector<unsigned>{1,2,3});
 rt::ObserveCreated(buffer.Get(),D3D12_RESOURCE_STATE_GENERIC_READ,true);
 gi_reuse::BufferStorageLease lease;
 Require(gi_reuse::AcquireStorageLease(buffer.Get(),lease),"Disabling inventory removed live storage proof");
 rt::ObserveBuffer(buffer.Get());
 D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;b.UAV.pResource=buffer.Get();
 rt::ObserveBarriers(1,&b);
 rt::liveBuild=&Build;rt::liveCopy=&Copy;rt::liveReset=&Reset;rt::liveInitialize=&Initialize;rt::liveBarriers=&Barriers;
 rt::Initialize(nullptr);rt::Reset(nullptr);rt::RecordBarriers(nullptr,1,&b);
 rt::RecordBuild(nullptr,nullptr);rt::RecordCopy(nullptr,256,512,D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE);
 Require(builds==1&&copies==1&&resets==1&&initializes==1&&barriers==1,"Live callback lost or duplicated");
 Require(!rt::BeforeExecute(nullptr,0,nullptr)&&!rt::fault.load(),"Disabled ledger processed submission");
 unsigned count=0;UINT64 bytes=0;bool limited=false;rt::owners.Stats(count,bytes,limited);
 Require(count==0&&bytes==0&&rt::buildCount==0&&rt::barrierObservations==0&&rt::creationObservations==0,"Legacy work still active");
 std::cout<<"PASS live-only mode: storage proof and callbacks preserved; duplicate inventory, barriers and ledger disabled\n";
 return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
