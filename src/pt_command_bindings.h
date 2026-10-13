#pragma once
#include "pt_native_root_layout.h"
#include <array>
#include <atomic>
// Records API arguments on the exact native command-list interfaces. Snapshots
// contain owned root/heap identities and GPU addresses, never CPU guesses about
// the engine's cached state. Resource contents/lifetimes require separate proof.
namespace PTCommandBindings {
using Microsoft::WRL::ComPtr;
enum class ArgumentKind {none,table,constant};
struct Argument {ArgumentKind kind=ArgumentKind::none;UINT64 value=0;};
struct RootState {ComPtr<ID3D12RootSignature> root;std::array<Argument,18> arguments{};};
struct Snapshot {
    std::shared_ptr<const PTNativeRoot::Layout> layout;
    ComPtr<ID3D12PipelineState> pipeline;
    ComPtr<ID3D12DescriptorHeap> resources,samplers;
    std::array<Argument,18> arguments{};
};
class Tracker {
    struct Entry {
        ComPtr<IUnknown> identity;
        ComPtr<ID3D12PipelineState> pipeline;
        ComPtr<ID3D12DescriptorHeap> resources,samplers;
        RootState compute,graphics;
        struct BufferState {ComPtr<ID3D12Resource> resource;D3D12_RESOURCE_STATES state{};bool pending=false;};
        std::map<ID3D12Resource*,BufferState> buffers;
    };
    std::mutex mutex;
    std::map<IUnknown*,std::shared_ptr<Entry>> entries;
    std::map<ID3D12GraphicsCommandList*,std::shared_ptr<Entry>> aliases;
    std::atomic<bool> healthy{true};
    template<class F> void Change(ID3D12GraphicsCommandList* list,F&& fn){
        // A hooked vtable is shared by command lists and COM interface aliases.
        // Observing its creation interface does not register every interface
        // subsequently used by the engine. Resolve an unseen caller before
        // recording its FIRST setter; silently dropping it loses the PSO/root.
        if(!healthy.load())return;
        {std::lock_guard<std::mutex> guard(mutex);const auto found=aliases.find(list);
            if(found!=aliases.end()){fn(*found->second);return;}}
        if(!Ensure(list))return;std::lock_guard<std::mutex> guard(mutex);
        const auto found=aliases.find(list);if(found!=aliases.end())fn(*found->second);
    }
    static void Tables(RootState& state){for(auto& a:state.arguments)if(a.kind==ArgumentKind::table)a={};}
    static bool Inside(ID3D12DescriptorHeap* heap,UINT64 address,UINT count){
        if(!heap||!address||!count)return false;const auto d=heap->GetDesc();
        if(!(d.Flags&D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)||count>d.NumDescriptors)return false;
        ComPtr<ID3D12Device> device;if(FAILED(heap->GetDevice(IID_PPV_ARGS(&device))))return false;
        const auto step=device->GetDescriptorHandleIncrementSize(d.Type);
        const auto base=heap->GetGPUDescriptorHandleForHeapStart().ptr;
        return step&&address>=base&&(address-base)%step==0&&(address-base)/step<=d.NumDescriptors-count;
    }
public:
    void Fail()noexcept{healthy=false;}
    bool Healthy()const noexcept{return healthy.load();}
    bool Ensure(ID3D12GraphicsCommandList* list){
        if(!list||!healthy.load())return false;
        {std::lock_guard<std::mutex> guard(mutex);if(aliases.count(list))return true;}
        if(Watch(list))return true;Fail();return false;
    }
    bool Watch(ID3D12GraphicsCommandList* list){
        if(!list||!healthy.load())return false;ComPtr<IUnknown> identity;
        if(FAILED(list->QueryInterface(IID_PPV_ARGS(&identity))))return false;
        std::lock_guard<std::mutex> guard(mutex);if(aliases.count(list))return true;
        auto found=entries.find(identity.Get());std::shared_ptr<Entry> entry;
        if(found!=entries.end())entry=found->second;
        else {if(entries.size()>=4096)return false;entry=std::make_shared<Entry>();entry->identity=identity;entries.emplace(identity.Get(),entry);}
        if(aliases.size()>=8192)return false;aliases.emplace(list,std::move(entry));return true;
    }
    void Reset(ID3D12GraphicsCommandList* list,ID3D12PipelineState* initial){
        Change(list,[&](Entry& e){e.pipeline=initial;e.resources.Reset();e.samplers.Reset();e.compute={};e.graphics={};e.buffers.clear();});
    }
    // Native wrappers describe the default state, not temporary transitions
    // around skinning/BLAS work. Keep exact recording-local buffer transitions.
    void Barriers(ID3D12GraphicsCommandList* list,UINT count,const D3D12_RESOURCE_BARRIER* barriers){
        Change(list,[&](Entry& e){
            for(UINT n=0;n<count;++n){const auto& b=barriers[n];
                if(b.Type==D3D12_RESOURCE_BARRIER_TYPE_ALIASING){e.buffers.clear();healthy=false;return;}
                if(b.Type!=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)continue;
                auto* resource=b.Transition.pResource;
                if(!resource||resource->GetDesc().Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER)continue;
                if(e.buffers.size()>=8192&&!e.buffers.count(resource)){healthy=false;return;}
                auto& tracked=e.buffers[resource];tracked.resource=resource;
                tracked.state=b.Transition.StateAfter;
                tracked.pending=b.Flags==D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
            }
        });
    }
    bool BufferState(ID3D12GraphicsCommandList* list,ID3D12Resource* resource,D3D12_RESOURCE_STATES fallback,D3D12_RESOURCE_STATES& state){
        if(!Ensure(list))return false;std::lock_guard<std::mutex> guard(mutex);
        const auto found=aliases.find(list);if(found==aliases.end())return false;
        const auto buffer=found->second->buffers.find(resource);
        if(buffer==found->second->buffers.end()){
            // Native static vertex/index wrappers keep their creation-time
            // read flags after ExecuteCommandLists has decayed the buffer.
            // A local explicit transition (including a split) always wins.
            // Restrict this normalization to the verified native read defaults;
            // UAV/COPY_DEST are never guessed to have completed.
            const unsigned bits=unsigned(fallback);
            const unsigned nativeReads=unsigned(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER|
                D3D12_RESOURCE_STATE_INDEX_BUFFER|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            state=(bits&&!(bits&~nativeReads))?D3D12_RESOURCE_STATE_COMMON:fallback;return true;
        }
        if(buffer->second.pending)return false;state=buffer->second.state;return true;
    }
    void Pipeline(ID3D12GraphicsCommandList* list,ID3D12PipelineState* value){Change(list,[&](Entry& e){e.pipeline=value;});}
    void Root(ID3D12GraphicsCommandList* list,ID3D12RootSignature* value,bool graphics){
        Change(list,[&](Entry& e){auto& s=graphics?e.graphics:e.compute;
            // Same root pointer does not invalidate D3D12 arguments. A different
            // interface alias is conservatively treated as a change until rebound.
            if(s.root.Get()!=value){s={};s.root=value;}});
    }
    void ArgumentValue(ID3D12GraphicsCommandList* list,UINT parameter,ArgumentKind kind,UINT64 value,bool graphics){
        Change(list,[&](Entry& e){auto& s=graphics?e.graphics:e.compute;
            if(parameter>=s.arguments.size()){s={};return;}s.arguments[parameter]={kind,value};});
    }
    void Heaps(ID3D12GraphicsCommandList* list,UINT count,ID3D12DescriptorHeap*const* values){
        Change(list,[&](Entry& e){ComPtr<ID3D12DescriptorHeap> resources,samplers;bool valid=count<=2&&(!count||values);
            if(valid)for(UINT n=0;n<count;++n){if(!values[n]){valid=false;break;}const auto d=values[n]->GetDesc();
                if(d.Type==D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV&&!resources)resources=values[n];
                else if(d.Type==D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER&&!samplers)samplers=values[n];else {valid=false;break;}}
            if(!valid){resources.Reset();samplers.Reset();}
            // Clearing all tables when either heap changes is conservative;
            // existing root CBVs remain valid. A later setter must rebind tables.
            if(resources.Get()!=e.resources.Get()||samplers.Get()!=e.samplers.Get()){Tables(e.compute);Tables(e.graphics);}
            e.resources=std::move(resources);e.samplers=std::move(samplers);
        });
    }
    ComPtr<ID3D12PipelineState> PipelineFor(ID3D12GraphicsCommandList* list){
        if(!Ensure(list))return {};std::lock_guard<std::mutex> guard(mutex);
        const auto found=aliases.find(list);return found==aliases.end()?ComPtr<ID3D12PipelineState>{}:found->second->pipeline;
    }
    bool Read(ID3D12GraphicsCommandList* list,PTNativeRoot::Registry& roots,bool graphics,UINT required,Snapshot& result){
        result={};if(!Ensure(list)||!required||required>=(1u<<(graphics?18:8)))return false;
        std::lock_guard<std::mutex> guard(mutex);const auto found=aliases.find(list);if(found==aliases.end())return false;
        const auto& e=*found->second;const auto& s=graphics?e.graphics:e.compute;auto layout=roots.Find(s.root.Get());
        if(!layout||layout->Type()!=(graphics?PTNativeRoot::Kind::graphics:PTNativeRoot::Kind::compute))return false;
        for(UINT n=0;n<(graphics?18u:8u);++n)if(required&(1u<<n)){
            const bool constant=graphics?(n>=8&&n<16):(n>=2&&n<6);const auto& a=s.arguments[n];
            if(a.kind!=(constant?ArgumentKind::constant:ArgumentKind::table)||!a.value)return false;
            if(constant){if(a.value%256)return false;continue;}
            const bool sampler=graphics?((n>=4&&n<8)||n==16):(n==1||n==6);
            const UINT count=sampler?((graphics?n==16:n==6)?64:4):((graphics?n==17:n==7)?244000:49);
            if(!Inside(sampler?e.samplers.Get():e.resources.Get(),a.value,count))return false;
        }
        result.layout=std::move(layout);result.pipeline=e.pipeline;result.resources=e.resources;result.samplers=e.samplers;result.arguments=s.arguments;return true;
    }
};
inline Tracker tracker;
// Private command recording must use Mute around creation AND recording. Hooks
// remain forwarding but cannot confuse private state with the native bindings.
inline thread_local unsigned nesting=0;
struct Mute {bool outer;Mute():outer(nesting++==0){}~Mute(){--nesting;}};
using ResetFn=HRESULT(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12CommandAllocator*,ID3D12PipelineState*);
using PipelineFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12PipelineState*);
using HeapsFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,ID3D12DescriptorHeap*const*);
using RootFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12RootSignature*);
using TableFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,D3D12_GPU_DESCRIPTOR_HANDLE);
using ConstantFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,D3D12_GPU_VIRTUAL_ADDRESS);
using BarriersFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,const D3D12_RESOURCE_BARRIER*);
using DispatchFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,UINT,UINT);
using DrawFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,UINT,UINT,UINT);
using DrawIndexedFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,UINT,UINT,INT,UINT);
struct Methods {ResetFn reset;PipelineFn pipeline;HeapsFn heaps;RootFn computeRoot,graphicsRoot;TableFn computeTable,graphicsTable;ConstantFn computeConstant,graphicsConstant;
    DispatchFn dispatch;DrawFn draw;DrawIndexedFn drawIndexed;BarriersFn barriers;};
// One session-lifetime renderer. Callbacks see the native snapshot; injected
// setters stay excluded by the enclosing Mute. Native dispatch/draw always
// forwards once, even when the callback declines a pass. Drawn restores any
// temporary PSO/table chosen by drawing. No callback may wait on a native job.
struct Consumer {
    void* context=nullptr;
    void (*dispatched)(void*,ID3D12GraphicsCommandList*,UINT,UINT,UINT) noexcept=nullptr;
    void (*drawing)(void*,ID3D12GraphicsCommandList*) noexcept=nullptr;
    void (*drawn)(void*,ID3D12GraphicsCommandList*) noexcept=nullptr;
    void (*pipeline)(void*,ID3D12GraphicsCommandList*,ID3D12PipelineState*) noexcept=nullptr;
};
inline std::atomic<const Consumer*> consumer{nullptr};
inline bool Register(const Consumer* client){
    if(!client||!client->context||bool(client->drawing)!=bool(client->drawn))return false;
    const Consumer* empty=nullptr;return consumer.compare_exchange_strong(empty,client,std::memory_order_release);
}
inline std::mutex hookMutex;
inline std::map<void**,Methods> methods;
inline bool installed=true;
inline Methods Original(ID3D12GraphicsCommandList* list){std::lock_guard<std::mutex> guard(hookMutex);return methods.at(*reinterpret_cast<void***>(list));}
template<class F> inline void Observe(F&& f)noexcept{try{f();}catch(...){tracker.Fail();}}
inline HRESULT STDMETHODCALLTYPE ResetHook(ID3D12GraphicsCommandList* l,ID3D12CommandAllocator* a,ID3D12PipelineState* p){
    const auto original=Original(l);Mute mute;const HRESULT hr=original.reset(l,a,p);const DWORD error=GetLastError();
    if(mute.outer&&SUCCEEDED(hr)){
        Observe([&]{tracker.Reset(l,p);});
        if(const auto client=consumer.load(std::memory_order_acquire))if(client->pipeline)client->pipeline(client->context,l,p);
    }SetLastError(error);return hr;
}
inline void STDMETHODCALLTYPE PipelineHook(ID3D12GraphicsCommandList* l,ID3D12PipelineState* p){
    const auto original=Original(l);Mute mute;original.pipeline(l,p);const DWORD error=GetLastError();
    if(mute.outer){
        Observe([&]{tracker.Pipeline(l,p);});
        if(const auto client=consumer.load(std::memory_order_acquire))if(client->pipeline)client->pipeline(client->context,l,p);
    }SetLastError(error);
}
inline void STDMETHODCALLTYPE HeapsHook(ID3D12GraphicsCommandList* l,UINT n,ID3D12DescriptorHeap*const* h){
    const auto original=Original(l);Mute mute;original.heaps(l,n,h);const DWORD error=GetLastError();
    if(mute.outer)Observe([&]{tracker.Heaps(l,n,h);});SetLastError(error);
}
template<bool Graphics> inline void STDMETHODCALLTYPE RootHook(ID3D12GraphicsCommandList* l,ID3D12RootSignature* r){
    const auto original=Original(l);Mute mute;(Graphics?original.graphicsRoot:original.computeRoot)(l,r);const DWORD error=GetLastError();
    if(mute.outer)Observe([&]{tracker.Root(l,r,Graphics);});SetLastError(error);
}
template<bool Graphics> inline void STDMETHODCALLTYPE TableHook(ID3D12GraphicsCommandList* l,UINT n,D3D12_GPU_DESCRIPTOR_HANDLE h){
    const auto original=Original(l);Mute mute;(Graphics?original.graphicsTable:original.computeTable)(l,n,h);const DWORD error=GetLastError();
    if(mute.outer)Observe([&]{tracker.ArgumentValue(l,n,ArgumentKind::table,h.ptr,Graphics);});SetLastError(error);
}
template<bool Graphics> inline void STDMETHODCALLTYPE ConstantHook(ID3D12GraphicsCommandList* l,UINT n,D3D12_GPU_VIRTUAL_ADDRESS v){
    const auto original=Original(l);Mute mute;(Graphics?original.graphicsConstant:original.computeConstant)(l,n,v);const DWORD error=GetLastError();
    if(mute.outer)Observe([&]{tracker.ArgumentValue(l,n,ArgumentKind::constant,v,Graphics);});SetLastError(error);
}
inline void STDMETHODCALLTYPE BarriersHook(ID3D12GraphicsCommandList* l,UINT count,const D3D12_RESOURCE_BARRIER* barriers){
    const auto original=Original(l);Mute mute;original.barriers(l,count,barriers);const DWORD error=GetLastError();
    if(mute.outer)Observe([&]{if(!tracker.Watch(l)){tracker.Fail();return;}tracker.Barriers(l,count,barriers);});SetLastError(error);
}
inline void STDMETHODCALLTYPE DispatchHook(ID3D12GraphicsCommandList* l,UINT x,UINT y,UINT z){
    const auto original=Original(l);Mute mute;original.dispatch(l,x,y,z);const DWORD error=GetLastError();
    if(mute.outer)if(const auto client=consumer.load(std::memory_order_acquire))if(client->dispatched)client->dispatched(client->context,l,x,y,z);
    SetLastError(error);
}
inline void STDMETHODCALLTYPE DrawHook(ID3D12GraphicsCommandList* l,UINT vertices,UINT instances,UINT firstVertex,UINT firstInstance){
    const auto original=Original(l);Mute mute;const auto client=mute.outer?consumer.load(std::memory_order_acquire):nullptr;
    const DWORD before=GetLastError();if(client&&client->drawing)client->drawing(client->context,l);SetLastError(before);
    original.draw(l,vertices,instances,firstVertex,firstInstance);const DWORD after=GetLastError();
    if(client&&client->drawn)client->drawn(client->context,l);SetLastError(after);
}
inline void STDMETHODCALLTYPE DrawIndexedHook(ID3D12GraphicsCommandList* l,UINT indices,UINT instances,UINT firstIndex,INT baseVertex,UINT firstInstance){
    const auto original=Original(l);Mute mute;const auto client=mute.outer?consumer.load(std::memory_order_acquire):nullptr;
    const DWORD before=GetLastError();if(client&&client->drawing)client->drawing(client->context,l);SetLastError(before);
    original.drawIndexed(l,indices,instances,firstIndex,baseVertex,firstInstance);const DWORD after=GetLastError();
    if(client&&client->drawn)client->drawn(client->context,l);SetLastError(after);
}
inline bool Patch(void** slot,void* original,void* hook){
    if(*slot!=original)return false;DWORD protection=0;if(!VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&protection))return false;
    const bool changed=InterlockedCompareExchangePointer(reinterpret_cast<void*volatile*>(slot),hook,original)==original;
    DWORD ignored=0;return VirtualProtect(slot,sizeof(void*),protection,&ignored)!=FALSE&&changed;
}
inline bool WatchInterface(ID3D12GraphicsCommandList* list){
    if(!list||!tracker.Watch(list))return false;std::lock_guard<std::mutex> guard(hookMutex);
    if(!installed)return false;auto** table=*reinterpret_cast<void***>(list);if(methods.count(table))return true;if(methods.size()>=32)return false;
    Methods m{reinterpret_cast<ResetFn>(table[10]),reinterpret_cast<PipelineFn>(table[25]),reinterpret_cast<HeapsFn>(table[28]),
        reinterpret_cast<RootFn>(table[29]),reinterpret_cast<RootFn>(table[30]),reinterpret_cast<TableFn>(table[31]),reinterpret_cast<TableFn>(table[32]),
        reinterpret_cast<ConstantFn>(table[37]),reinterpret_cast<ConstantFn>(table[38]),reinterpret_cast<DispatchFn>(table[14]),reinterpret_cast<DrawFn>(table[12]),reinterpret_cast<DrawIndexedFn>(table[13]),reinterpret_cast<BarriersFn>(table[26])};methods.emplace(table,m);
    const UINT slots[]={10,25,28,29,30,31,32,37,38,14,12,13,26};
    void* const before[]={reinterpret_cast<void*>(m.reset),reinterpret_cast<void*>(m.pipeline),reinterpret_cast<void*>(m.heaps),reinterpret_cast<void*>(m.computeRoot),reinterpret_cast<void*>(m.graphicsRoot),
        reinterpret_cast<void*>(m.computeTable),reinterpret_cast<void*>(m.graphicsTable),reinterpret_cast<void*>(m.computeConstant),reinterpret_cast<void*>(m.graphicsConstant),
        reinterpret_cast<void*>(m.dispatch),reinterpret_cast<void*>(m.draw),reinterpret_cast<void*>(m.drawIndexed),reinterpret_cast<void*>(m.barriers)};
    void* const after[]={reinterpret_cast<void*>(&ResetHook),reinterpret_cast<void*>(&PipelineHook),reinterpret_cast<void*>(&HeapsHook),reinterpret_cast<void*>(&RootHook<false>),reinterpret_cast<void*>(&RootHook<true>),
        reinterpret_cast<void*>(&TableHook<false>),reinterpret_cast<void*>(&TableHook<true>),reinterpret_cast<void*>(&ConstantHook<false>),reinterpret_cast<void*>(&ConstantHook<true>),
        reinterpret_cast<void*>(&DispatchHook),reinterpret_cast<void*>(&DrawHook),reinterpret_cast<void*>(&DrawIndexedHook),reinterpret_cast<void*>(&BarriersHook)};
    for(UINT n=0;n<13;++n)if(!Patch(table+slots[n],before[n],after[n])){installed=false;tracker.Fail();return false;}
    return true;
}
inline bool Watch(ID3D12GraphicsCommandList* list){
    if(nesting||!WatchInterface(list))return false;ComPtr<ID3D12GraphicsCommandList> base;
    return SUCCEEDED(list->QueryInterface(IID_PPV_ARGS(&base)))&&WatchInterface(base.Get());
}
}
