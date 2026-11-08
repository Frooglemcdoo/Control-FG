#define main OwnedSequenceMain
#include "test-gi-owned-sequence.cpp"
#undef main
#include "../src/gi_dual_history_bridge.h"
#include "../src/pt_glass_descriptors.h"
#include "../src/gi_reuse/owned_request.h"
#include "../build/gi_dual_history.generated.h"
#include "../build/gi_motion_history.generated.h"
#include <memory>
#include <cstdio>
static HMODULE verifiedD3d=nullptr,verifiedRenderer=nullptr;
static constexpr char kRRGetNativeTextureSymbol[]="unused_fixture";
template<class... A>static void Log(const char* f,A... a){std::printf(f,a...);std::puts("");}
namespace PTGlassRuntime {
static ComPtr<ID3D12Device> device;
static bool copySimple=true,createCbv=true;
struct Context {void* manager=nullptr;ID3D12GraphicsCommandList* list=nullptr;ID3D12GraphicsCommandList4* rays=nullptr;ID3D12CommandQueue* queue=nullptr;PTGlass::DescriptorRange table;D3D12_GPU_DESCRIPTOR_HANDLE gpu{};};
static Context current;static UINT nextTable=PTGlass::LightingTableLayout::NativeMaterialPrefix;
static bool ReadContext(Context* c)noexcept{*c=current;return true;}
static bool Fresh(const Context& c,PTGlass::DescriptorRange* r)noexcept{*r={c.table.heap,nextTable,49};nextTable+=49;return nextTable<c.table.heap->GetDesc().NumDescriptors;}
}
namespace native_lighting {namespace rt_spot_table_audit {inline bool InstallGameQueue(ID3D12CommandQueue*)noexcept{return true;}}}
#include "../src/gi_dual_history_runtime.h"
static void STDMETHODCALLTYPE Forward(ID3D12GraphicsCommandList* l,UINT x,UINT y,UINT z){l->Dispatch(x,y,z);}
static void Wait(ID3D12Fence* f,UINT64 value){HANDLE e=CreateEventW(nullptr,FALSE,FALSE,nullptr);Require(e!=nullptr,"event");Check(f->SetEventOnCompletion(value,e));Require(WaitForSingleObject(e,30000)==WAIT_OBJECT_0,"fence timeout");CloseHandle(e);}
int main(){try{
 Harness h(false);auto& g=h.gpu;PTGlassRuntime::device=g.device;
 gi_dual_bridge::resetReady=+[](ID3D12GraphicsCommandList*)noexcept{return true;};
 D3D12_DESCRIPTOR_RANGE ranges[3]={{D3D12_DESCRIPTOR_RANGE_TYPE_CBV,5,4,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,36,0,0,5},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,8,0,0,41}};
 D3D12_ROOT_PARAMETER ps[3]{};ps[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;ps[0].DescriptorTable={3,ranges};for(UINT i=1;i<3;++i){ps[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;ps[i].Descriptor.ShaderRegister=i-1;}
 D3D12_STATIC_SAMPLER_DESC sam[2]{};for(UINT i=0;i<2;++i){auto&s=sam[i];s.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;s.AddressU=s.AddressV=s.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;s.MaxLOD=D3D12_FLOAT32_MAX;s.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;s.MaxAnisotropy=1;s.ShaderRegister=i?11:6;s.RegisterSpace=1;}
 D3D12_ROOT_SIGNATURE_DESC rd{3,ps,2,sam,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob>b,e;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&b,&e));ComPtr<ID3D12RootSignature> root;Check(g.device->CreateRootSignature(0,b->GetBufferPointer(),b->GetBufferSize(),IID_PPV_ARGS(&root)));
 D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={kGIDualHistory,sizeof(kGIDualHistory)};ComPtr<ID3D12PipelineState> dual,native;Check(g.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&dual)));pd.CS={kGIStableEnergyShader,sizeof(kGIStableEnergyShader)};Check(g.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&native)));
 D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=PTGlass::LightingTableLayout::NativeMaterialPrefix+4096;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;ComPtr<ID3D12DescriptorHeap> visible,cpu;Check(g.device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&visible)));hd.NumDescriptors=49;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_NONE;Check(g.device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&cpu)));
 auto handle=[&](ID3D12DescriptorHeap* heap,UINT i){auto v=heap->GetCPUDescriptorHandleForHeapStart();v.ptr+=SIZE_T(i)*g.descriptorSize;return v;};
 D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;srv.Texture2D.MipLevels=1;srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
 D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.Format=srv.Format;uav.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
 for(UINT i=0;i<5;++i)g.device->CreateConstantBufferView(nullptr,handle(cpu.Get(),i));for(UINT i=5;i<41;++i)g.device->CreateShaderResourceView(nullptr,&srv,handle(cpu.Get(),i));for(UINT i=41;i<49;++i)g.device->CreateUnorderedAccessView(nullptr,nullptr,&uav,handle(cpu.Get(),i));
 Image flat(W*H,Pixel{.5f,.5f,.5f,.5f}),zero(W*H,Pixel{}),light(W*H,Pixel{.4f,.4f,.4f,.4f});auto depth=h.Texture(&flat),mv=h.Texture(&zero),current=h.Texture(&light),initial=h.Texture(&light);
 std::array<float,192> sys{};sys[0]=sys[4]=W;sys[1]=sys[5]=H;sys[2]=sys[6]=1.f/W;sys[3]=sys[7]=1.f/H;auto cb0=g.Buffer(sizeof(sys),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,sys.data());std::array<UINT,64> f{};auto cb1=g.Buffer(sizeof(f),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,f.data());
 std::array<Tex,6> outputs;for(auto& t:outputs)t=h.Texture();g.Submit();
 std::array<ComPtr<ID3D12GraphicsCommandList>,6> lists;std::array<ComPtr<ID3D12CommandAllocator>,6> allocators;
 ComPtr<ID3D12Fence> gate;Check(g.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&gate)));Check(g.queue->Wait(gate.Get(),1));
 struct ReleaseGate {ID3D12Fence* f;~ReleaseGate(){f->Signal(1);}} releaseGate{gate.Get()};
 std::vector<ComPtr<ID3D12Resource>> reads;
 for(UINT frame=1;frame<=5;++frame){
  const UINT i=frame-1;Check(g.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocators[i])));Check(g.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocators[i].Get(),nullptr,IID_PPV_ARGS(&lists[i])));auto* l=lists[i].Get();ID3D12DescriptorHeap* heap=visible.Get();l->SetDescriptorHeaps(1,&heap);l->SetComputeRootSignature(root.Get());l->SetComputeRootConstantBufferView(1,cb0->GetGPUVirtualAddress());l->SetComputeRootConstantBufferView(2,cb1->GetGPUVirtualAddress());l->SetPipelineState(native.Get());
  const UINT first=PTGlassRuntime::nextTable;PTGlassRuntime::nextTable+=49;auto gpu=visible->GetGPUDescriptorHandleForHeapStart();gpu.ptr+=UINT64(first)*g.descriptorSize;PTGlassRuntime::current={nullptr,l,nullptr,g.queue.Get(),{visible.Get(),first,49},gpu};l->SetComputeRootDescriptorTable(0,gpu);
  ID3D12Resource* inputs[]={depth.r.Get(),depth.r.Get(),mv.r.Get(),current.r.Get(),frame==1?initial.r.Get():outputs[i-1].r.Get()};for(UINT j=0;j<5;++j)g.device->CreateShaderResourceView(inputs[j],&srv,handle(cpu.Get(),5+j));g.device->CreateUnorderedAccessView(outputs[i].r.Get(),nullptr,&uav,handle(cpu.Get(),41));
  GIDualRuntime::Begin(frame,1);g.device->CopyDescriptorsSimple(49,handle(visible.Get(),first),handle(cpu.Get(),0),hd.Type);GIDualRuntime::Copy(handle(visible.Get(),first),49,handle(cpu.Get(),0));GIDualRuntime::End();
  const bool applied=GIDualRuntime::DispatchTarget(l,native.Get(),dual.Get(),W/8,H/8,1,&Forward,outputs[i].r.Get());Require(applied==(frame<=4),"inflight pool admission");
  if(!applied){Check(l->Close());break;}
  D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition={outputs[i].r.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE};l->ResourceBarrier(1,&barrier);
  auto read=g.Buffer(outputs[i].bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=read.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=outputs[i].f;src.pResource=outputs[i].r.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;l->CopyTextureRegion(&dst,0,0,0,&src,nullptr);reads.push_back(read);
  barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_SOURCE;barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;l->ResourceBarrier(1,&barrier);Check(l->Close());ID3D12CommandList* list=l;auto token=GIDualRuntime::Before(g.queue.Get(),1,&list);Require(token!=0,"submission not recognized");g.queue->ExecuteCommandLists(1,&list);GIDualRuntime::After(g.queue.Get(),token);
 }
 Check(gate->Signal(1));Wait(GIDualRuntime::fence.Get(),GIDualRuntime::nextFence);
 for(UINT i=0;i<4;++i){void* data=nullptr;Check(reads[i]->Map(0,nullptr,&data));auto* p=reinterpret_cast<float*>(static_cast<unsigned char*>(data)+outputs[i].f.Offset+(H/2)*outputs[i].f.Footprint.RowPitch)+4*(W/2);Require(std::abs(p[0]-.4f)<.001f,"adapter changed stable energy");reads[i]->Unmap(0,nullptr);}
 Require(GIDualRuntime::recordings.size()==4,"closed recordings were retired before Reset");
 for(UINT i=0;i<4;++i){Check(allocators[i]->Reset());Check(lists[i]->Reset(allocators[i].Get(),nullptr));GIDualRuntime::Reset(lists[i].Get());Check(lists[i]->Close());}
 Require(GIDualRuntime::recordings.empty()&&!GIDualRuntime::policy.Failed(),"fence and Reset retirement");
 std::cout<<"PASS production adapter: native root table, four queued histories, delayed-fence exhaustion, energy and Reset retirement"<<std::endl;return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
