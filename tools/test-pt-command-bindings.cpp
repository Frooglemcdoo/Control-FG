#define NOMINMAX
#include <windows.h>
#include <dxgi1_6.h>
#include <cassert>
#include <cstdio>
#include <future>
#include <thread>
#include <chrono>
#include "../src/pt_command_bindings.h"
#include "../src/pt_scene_queue_runtime.h"
#include "pt-native-root-fixture.h"
using Microsoft::WRL::ComPtr;
struct ForwardingFixture {
    void** table=nullptr;
    void* slots[40]{};
    UINT phase=0,nested=0;
    IUnknown* identity=nullptr;
    static HRESULT STDMETHODCALLTYPE Query(IUnknown* self,REFIID,void** out){auto* f=reinterpret_cast<ForwardingFixture*>(self);*out=f->identity?f->identity:self;return S_OK;}
    static ULONG STDMETHODCALLTYPE Ref(IUnknown*){return 1;}
    static void STDMETHODCALLTYPE Dispatch(ID3D12GraphicsCommandList* list,UINT x,UINT y,UINT z){
        auto& f=*reinterpret_cast<ForwardingFixture*>(list);assert(x==7&&y==8&&z==9&&f.phase==0);f.phase=1;SetLastError(0x555);
    }
    static void STDMETHODCALLTYPE Draw(ID3D12GraphicsCommandList* list,UINT vertices,UINT instances,UINT firstVertex,UINT firstInstance){
        auto& f=*reinterpret_cast<ForwardingFixture*>(list);if(!vertices&&!instances){++f.nested;return;}
        assert(vertices==3&&instances==4&&firstVertex==5&&firstInstance==6&&f.phase==1&&GetLastError()==0x444);f.phase=2;SetLastError(0x555);
    }
    static void STDMETHODCALLTYPE Indexed(ID3D12GraphicsCommandList* list,UINT indices,UINT instances,UINT firstIndex,INT baseVertex,UINT firstInstance){
        auto& f=*reinterpret_cast<ForwardingFixture*>(list);assert(indices==3&&instances==4&&firstIndex==5&&baseVertex==-7&&firstInstance==6&&f.phase==1&&GetLastError()==0x444);f.phase=2;SetLastError(0x555);
    }
    static void Dispatched(void* context,ID3D12GraphicsCommandList*,UINT x,UINT y,UINT z)noexcept{
        auto& f=*static_cast<ForwardingFixture*>(context);assert(x==7&&y==8&&z==9&&f.phase==1);f.phase=3;SetLastError(0x666);
    }
    static void Drawing(void* context,ID3D12GraphicsCommandList* list)noexcept{
        auto& f=*static_cast<ForwardingFixture*>(context);assert(f.phase==0);f.phase=1;list->DrawInstanced(0,0,0,0);SetLastError(0x777);
    }
    static void Drawn(void* context,ID3D12GraphicsCommandList*)noexcept{
        auto& f=*static_cast<ForwardingFixture*>(context);assert(f.phase==2);f.phase=3;SetLastError(0x666);
    }
};
static void TestForwarding(){
    // Process-lifetime COM ABI fixture: installed hooks retain its identity.
    auto* f=new ForwardingFixture();f->table=f->slots;
    f->slots[0]=reinterpret_cast<void*>(&ForwardingFixture::Query);f->slots[1]=f->slots[2]=reinterpret_cast<void*>(&ForwardingFixture::Ref);
    f->slots[12]=reinterpret_cast<void*>(&ForwardingFixture::Draw);f->slots[13]=reinterpret_cast<void*>(&ForwardingFixture::Indexed);f->slots[14]=reinterpret_cast<void*>(&ForwardingFixture::Dispatch);
    auto* list=reinterpret_cast<ID3D12GraphicsCommandList*>(f);assert(PTCommandBindings::Watch(list));
    PTCommandBindings::Consumer consumer{f,&ForwardingFixture::Dispatched,&ForwardingFixture::Drawing,&ForwardingFixture::Drawn};
    assert(PTCommandBindings::Register(&consumer));assert(!PTCommandBindings::Register(&consumer));
    list->Dispatch(7,8,9);assert(f->phase==3&&GetLastError()==0x555);f->phase=0;
    SetLastError(0x444);list->DrawInstanced(3,4,5,6);assert(f->phase==3&&f->nested==1&&GetLastError()==0x555);f->phase=0;
    SetLastError(0x444);list->DrawIndexedInstanced(3,4,5,-7,6);assert(f->phase==3&&f->nested==2&&GetLastError()==0x555);
    PTCommandBindings::consumer.store(nullptr);
}
static void TestComputeOnlyHooks(){
    auto* f=new ForwardingFixture();f->table=f->slots;
    f->slots[0]=reinterpret_cast<void*>(&ForwardingFixture::Query);f->slots[1]=f->slots[2]=reinterpret_cast<void*>(&ForwardingFixture::Ref);
    f->slots[12]=reinterpret_cast<void*>(&ForwardingFixture::Draw);f->slots[13]=reinterpret_cast<void*>(&ForwardingFixture::Indexed);f->slots[14]=reinterpret_cast<void*>(&ForwardingFixture::Dispatch);
    // Sentinel graphics setters must remain untouched; they are never called.
    for(UINT slot:{30u,32u,38u})f->slots[slot]=reinterpret_cast<void*>(&ForwardingFixture::Ref);
    PTCommandBindings::Consumer invalid{f,&ForwardingFixture::Dispatched,&ForwardingFixture::Drawing,&ForwardingFixture::Drawn,nullptr,true};
    assert(!PTCommandBindings::Register(&invalid));
    PTCommandBindings::Consumer compute{f,&ForwardingFixture::Dispatched,nullptr,nullptr,nullptr,true};
    assert(PTCommandBindings::Register(&compute));
    auto* list=reinterpret_cast<ID3D12GraphicsCommandList*>(f);assert(PTCommandBindings::Watch(list));
    assert(f->slots[12]==reinterpret_cast<void*>(&ForwardingFixture::Draw));
    assert(f->slots[13]==reinterpret_cast<void*>(&ForwardingFixture::Indexed));
    for(UINT slot:{30u,32u,38u})assert(f->slots[slot]==reinterpret_cast<void*>(&ForwardingFixture::Ref));
    assert(f->slots[29]==reinterpret_cast<void*>(&PTCommandBindings::RootHook<false>));
    assert(f->slots[31]==reinterpret_cast<void*>(&PTCommandBindings::TableHook<false>));
    assert(f->slots[37]==reinterpret_cast<void*>(&PTCommandBindings::ConstantHook<false>));
    assert(f->slots[26]==reinterpret_cast<void*>(&PTCommandBindings::BarriersHook));
    list->Dispatch(7,8,9);assert(f->phase==3&&GetLastError()==0x555);
    PTCommandBindings::consumer.store(nullptr);
}
struct BlockingPipeline {
    void** table=nullptr;void* slots[3]{};
    HANDLE entered=CreateEventW(nullptr,TRUE,FALSE,nullptr),release=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    std::atomic<bool> block{true};
    static ULONG STDMETHODCALLTYPE Add(IUnknown* self){
        auto* p=reinterpret_cast<BlockingPipeline*>(self);
        if(p->block.exchange(false)){SetEvent(p->entered);WaitForSingleObject(p->release,INFINITE);}
        return 1;
    }
    static ULONG STDMETHODCALLTYPE Release(IUnknown*){return 1;}
};
static void TestIndependentRecording(){
    PTCommandBindings::Tracker tracker;
    const auto make=[](){auto* f=new ForwardingFixture();f->table=f->slots;
        f->slots[0]=reinterpret_cast<void*>(&ForwardingFixture::Query);
        f->slots[1]=f->slots[2]=reinterpret_cast<void*>(&ForwardingFixture::Ref);return f;};
    auto* a=make();auto* b=make();auto* alias=make();alias->identity=reinterpret_cast<IUnknown*>(a);
    auto* first=reinterpret_cast<ID3D12GraphicsCommandList*>(a);
    auto* second=reinterpret_cast<ID3D12GraphicsCommandList*>(b);
    auto* otherAlias=reinterpret_cast<ID3D12GraphicsCommandList*>(alias);
    assert(tracker.Watch(first)&&tracker.Watch(second)&&tracker.Watch(otherAlias));
    auto* p=new BlockingPipeline();assert(p->entered&&p->release);p->table=p->slots;
    p->slots[1]=reinterpret_cast<void*>(&BlockingPipeline::Add);p->slots[2]=reinterpret_cast<void*>(&BlockingPipeline::Release);
    auto* pipeline=reinterpret_cast<ID3D12PipelineState*>(p);
    std::thread blocked([&]{tracker.Pipeline(first,pipeline);});
    assert(WaitForSingleObject(p->entered,5000)==WAIT_OBJECT_0);
    auto independent=std::async(std::launch::async,[&]{tracker.ArgumentValue(second,2,PTCommandBindings::ArgumentKind::constant,256,false);});
    const bool progressed=independent.wait_for(std::chrono::seconds(3))==std::future_status::ready;
    SetEvent(p->release);blocked.join();independent.get();assert(progressed);
    assert(tracker.PipelineFor(otherAlias).Get()==pipeline);
    tracker.Reset(otherAlias,nullptr);assert(!tracker.PipelineFor(first));
    CloseHandle(p->entered);CloseHandle(p->release);
    puts("PASS: independent recording progresses while another list is blocked; aliases share pipeline and Reset state");
}
int main(){
    TestIndependentRecording();
    TestForwarding();
    TestComputeOnlyHooks();
    ComPtr<IDXGIFactory4> factory;ComPtr<IDXGIAdapter> adapter;ComPtr<ID3D12Device> device;
    assert(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))));assert(SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter))));
    assert(SUCCEEDED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))));
    PTNativeRoot::Registry roots;ComPtr<ID3D12RootSignature> compute,graphics;
    for(bool isGraphics:{false,true}){
        PTNativeRootFixture fixture(isGraphics);ComPtr<ID3DBlob> bytes,error;ComPtr<ID3D12RootSignature> root;
        assert(SUCCEEDED(D3D12SerializeRootSignature(&fixture.desc,D3D_ROOT_SIGNATURE_VERSION_1_0,&bytes,&error)));
        assert(SUCCEEDED(device->CreateRootSignature(0,bytes->GetBufferPointer(),bytes->GetBufferSize(),IID_PPV_ARGS(&root))));
        assert(roots.Observe(bytes->GetBufferPointer(),bytes->GetBufferSize(),root.Get(),&D3D12CreateVersionedRootSignatureDeserializer));
        (isGraphics?graphics:compute)=root;
    }
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;
    assert(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))));
    assert(SUCCEEDED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list))));
    assert(PTCommandBindings::Watch(list.Get()));
    ComPtr<ID3D12GraphicsCommandList4> ray;assert(SUCCEEDED(list.As(&ray)));assert(PTCommandBindings::Watch(ray.Get()));
    // Reset interception composes with the existing scene queue/list observer.
    PTOwnedScene::Runtime scene;PTSceneLive::Capture capture{scene};assert(PTSceneQueues::Start(&capture));assert(PTSceneQueues::WatchList(list.Get()));
    D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=244100;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ComPtr<ID3D12DescriptorHeap> resources,replacement,samplers;assert(SUCCEEDED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&resources))));
    assert(SUCCEEDED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&replacement))));
    hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;hd.NumDescriptors=68;assert(SUCCEEDED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&samplers))));
    D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Width=1024;bd.Height=1;bd.DepthOrArraySize=bd.MipLevels=1;bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_UPLOAD;ComPtr<ID3D12Resource> constants;
    assert(SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&constants))));
    auto nativeTable=resources->GetGPUDescriptorHandleForHeapStart(),materialTable=nativeTable;
    materialTable.ptr+=49ull*device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    auto nativeSamplers=samplers->GetGPUDescriptorHandleForHeapStart(),globalSamplers=nativeSamplers;
    globalSamplers.ptr+=4ull*device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    ID3D12DescriptorHeap* heaps[]={resources.Get(),samplers.Get()};list->SetDescriptorHeaps(2,heaps);
    list->SetComputeRootSignature(compute.Get());list->SetGraphicsRootSignature(graphics.Get());
    const auto bind=[&]{
        list->SetComputeRootDescriptorTable(0,nativeTable);list->SetComputeRootDescriptorTable(1,nativeSamplers);
        list->SetComputeRootDescriptorTable(6,globalSamplers);list->SetComputeRootDescriptorTable(7,materialTable);
        for(UINT n=0;n<4;++n){list->SetComputeRootConstantBufferView(2+n,constants->GetGPUVirtualAddress()+n*256);
            list->SetGraphicsRootDescriptorTable(n,nativeTable);list->SetGraphicsRootDescriptorTable(4+n,nativeSamplers);
            list->SetGraphicsRootConstantBufferView(8+n,constants->GetGPUVirtualAddress()+n*256);list->SetGraphicsRootConstantBufferView(12+n,constants->GetGPUVirtualAddress()+n*256);}
        list->SetGraphicsRootDescriptorTable(16,globalSamplers);list->SetGraphicsRootDescriptorTable(17,materialTable);
    };
    bind();PTCommandBindings::Snapshot snapshot;constexpr UINT allCompute=255,allGraphics=(1u<<18)-1;
    assert(PTCommandBindings::tracker.Read(list.Get(),roots,false,allCompute,snapshot));
    assert(snapshot.arguments[2].value==constants->GetGPUVirtualAddress());
    assert(PTCommandBindings::tracker.Read(ray.Get(),roots,false,allCompute,snapshot));
    assert(PTCommandBindings::tracker.Read(list.Get(),roots,true,allGraphics,snapshot));
    assert(snapshot.arguments[12].value==constants->GetGPUVirtualAddress());
    list->SetComputeRootSignature(compute.Get());assert(PTCommandBindings::tracker.Read(list.Get(),roots,false,allCompute,snapshot));
    {PTCommandBindings::Mute privateCommands;auto shifted=nativeTable;shifted.ptr+=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        list->SetComputeRootDescriptorTable(0,shifted);assert(PTCommandBindings::tracker.Read(list.Get(),roots,false,allCompute,snapshot));assert(snapshot.arguments[0].value==nativeTable.ptr);
        list->SetComputeRootDescriptorTable(0,nativeTable);}
    // Inject invalid metadata, never invalid native GPU calls.
    PTCommandBindings::tracker.ArgumentValue(list.Get(),2,PTCommandBindings::ArgumentKind::constant,constants->GetGPUVirtualAddress()+4,false);
    assert(!PTCommandBindings::tracker.Read(list.Get(),roots,false,allCompute,snapshot));bind();
    PTCommandBindings::tracker.ArgumentValue(list.Get(),7,PTCommandBindings::ArgumentKind::table,materialTable.ptr+1000000,false);
    assert(!PTCommandBindings::tracker.Read(list.Get(),roots,false,allCompute,snapshot));bind();
    ID3D12DescriptorHeap* changed[]={replacement.Get(),samplers.Get()};list->SetDescriptorHeaps(2,changed);
    assert(!PTCommandBindings::tracker.Read(list.Get(),roots,false,allCompute,snapshot));
    assert(PTCommandBindings::tracker.Read(list.Get(),roots,false,1u<<2,snapshot)); // Root CBV survives heap change.
    list->SetDescriptorHeaps(2,heaps);bind();assert(PTCommandBindings::tracker.Read(list.Get(),roots,false,allCompute,snapshot));
    assert(SUCCEEDED(list->Close()));assert(SUCCEEDED(list->Reset(allocator.Get(),nullptr)));
    assert(!PTCommandBindings::tracker.Read(list.Get(),roots,false,1u<<2,snapshot));assert(!PTCommandBindings::tracker.Read(list.Get(),roots,true,1u<<12,snapshot));
    PTSceneQueues::observer.store(nullptr);assert(SUCCEEDED(list->Close()));
    puts("PASS: intercepted native compute/graphics bindings, alias identity, private-recording exclusion, bounds, heap changes and chained Reset");
}
