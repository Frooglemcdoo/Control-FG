#pragma once
#include "d3d12_producer_pipeline.h"
#include "d3d12_producer_table.h"
#include "d3d12_compute_bindings.h"
#include "producer_ray.h"
#include "initial_reservoir.h"
#include "frame_tiles.h"
#include "d3d12_dispatch_gate.h"
#include "d3d12_gi_scratch_pool.h"
#include <memory>
#include "../pt_glass_descriptors.h"
namespace gi_reuse {
// Four stages sharing the admitted native GI6 root. Shader bytecode is prepared
// before recording; the host must own the current scene and publication leases.
class D3D12InitialGIPipeline {
    template<class T> using Ptr=Microsoft::WRL::ComPtr<T>;
    Ptr<ID3D12RootSignature> root_;
    Ptr<ID3D12PipelineState> proposals_,shading_,resolve_;
    D3D12ProducerPipeline producer_;
    bool gated_=false;
public:
    HRESULT Initialize(ID3D12Device5* device,ID3D12RootSignature* root,
        D3D12_SHADER_BYTECODE proposals,D3D12_SHADER_BYTECODE trace,
        D3D12_SHADER_BYTECODE shading,D3D12_SHADER_BYTECODE resolve,bool gated=false) noexcept {
        if(root_||producer_.State()||!device||!root)return E_INVALIDARG;
        for(const auto code:{proposals,trace,shading,resolve})if(!code.pShaderBytecode||!code.BytecodeLength)return E_INVALIDARG;
        Ptr<ID3D12PipelineState> a,b,e;
        D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};desc.pRootSignature=root;
        desc.CS=proposals;HRESULT hr=device->CreateComputePipelineState(&desc,IID_PPV_ARGS(&a));if(FAILED(hr))return hr;
        desc.CS=shading;hr=device->CreateComputePipelineState(&desc,IID_PPV_ARGS(&b));if(FAILED(hr))return hr;
        desc.CS=resolve;hr=device->CreateComputePipelineState(&desc,IID_PPV_ARGS(&e));if(FAILED(hr))return hr;
        hr=producer_.Initialize(device,root,trace);if(FAILED(hr))return hr;
        gated_=gated;root_=root;proposals_=std::move(a);shading_=std::move(b);resolve_=std::move(e);return S_OK;
    }
    bool Gated()const noexcept{return gated_;}
    ID3D12RootSignature* Root() const noexcept{return root_.Get();}
    const D3D12ProducerPipeline& Producer() const noexcept{return producer_;}
    ID3D12PipelineState* Proposals() const noexcept{return proposals_.Get();}
    ID3D12PipelineState* Shading() const noexcept{return shading_.Get();}
    ID3D12PipelineState* Resolve() const noexcept{return resolve_.Get();}
};
// Single-recording bounded frame or subregion. CPU descriptors and constants are immutable
// after preparation. Retain this owner, its pipeline, the native binding owners,
// the producer table and the selected TLAS/BLAS closure through queue completion.
// Descriptor addresses are allocated by the native same-heap allocator; no heap
// switching or shader-visible descriptor reads are permitted here.
class D3D12InitialGITile {
    template<class T> using Ptr=Microsoft::WRL::ComPtr<T>;
    Ptr<ID3D12Device> device_;
    Ptr<ID3D12DescriptorHeap> staging_;
    Ptr<ID3D12Resource> rays_,batches_,requests_,samples_,constants_,controlUpload_;
    unsigned char* controls_=nullptr;
    GIFrameTiles layout_;
    D3D12DispatchGate gate_;
    std::array<D3D12_GPU_DESCRIPTOR_HANDLE,4> tables_{};
    Ptr<ID3D12DescriptorHeap> visible_;
    Ptr<ID3D12StateObject> recordedProducer_;
    UINT64 recordedHits_=0;
    UINT pixels_=0,count_=0,firstPixel_=0,step_=0;
    UINT64 bytes_=0,epoch_=0,version_=0,nativeTable_=0;
    bool recorded_=false,published_=false,deferred_=false,armed_=false;
    UINT sceneRevision_=0;
    D3D12_CPU_DESCRIPTOR_HANDLE At(UINT stage,UINT slot) const noexcept {
        auto h=staging_->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(stage*49+slot)*step_;return h;
    }
    HRESULT Buffer(UINT64 bytes,Ptr<ID3D12Resource>& out,D3D12_HEAP_TYPE type=D3D12_HEAP_TYPE_DEFAULT) noexcept {
        try {out=SharedGIScratchPool().Take(device_.Get(),bytes,type);}catch(...){}
        if(out){bytes_+=bytes;return S_OK;}
        D3D12_HEAP_PROPERTIES heap{};heap.Type=type;heap.CreationNodeMask=heap.VisibleNodeMask=1;
        D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=bytes;d.Height=1;
        d.DepthOrArraySize=d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        if(type==D3D12_HEAP_TYPE_DEFAULT)d.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        auto hr=device_->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,
            type==D3D12_HEAP_TYPE_DEFAULT?D3D12_RESOURCE_STATE_COMMON:D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,IID_PPV_ARGS(&out));if(SUCCEEDED(hr))bytes_+=bytes;return hr;
    }
    void View(UINT stage,UINT slot,ID3D12Resource* resource,UINT stride,UINT count,bool write) noexcept {
        if(write){D3D12_UNORDERED_ACCESS_VIEW_DESC d{};d.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;
            d.Buffer.NumElements=count;d.Buffer.StructureByteStride=stride;device_->CreateUnorderedAccessView(resource,nullptr,&d,At(stage,slot));}
        else{D3D12_SHADER_RESOURCE_VIEW_DESC d{};d.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;
            d.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;d.Buffer.NumElements=count;d.Buffer.StructureByteStride=stride;
            device_->CreateShaderResourceView(resource,&d,At(stage,slot));}
    }
    static void Readable(ID3D12GraphicsCommandList* list,ID3D12Resource* resource) noexcept {
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE};
        list->ResourceBarrier(1,&b);
    }
    bool NativeBindingsMatch(const ComputeBindingSnapshot& native) const noexcept {
        struct CheckBindings {
            UINT mask=0;UINT64 original=0;bool valid=true;
            void Constants(unsigned index,unsigned,unsigned,const std::uint32_t*) noexcept {
                if(index<8&&index!=1)valid=false;
            }
            void Descriptor(unsigned index,RootArgumentKind kind,UINT64 address) noexcept {
                if(index>=8||index==1)return;
                const auto expected=index>=2&&index<=5?RootArgumentKind::CBV:RootArgumentKind::Table;
                if(kind!=expected||!address){valid=false;return;}
                if(index>=2&&index<=5&&address%256){valid=false;return;}
                mask|=1u<<index;if(!index)original=address;
            }
        } checked;
        return native.arguments.Replay(checked)&&checked.valid&&checked.mask==0xfdu&&checked.original==nativeTable_;
    }
public:
    ~D3D12InitialGITile(){
        if(controls_){const D3D12_RANGE written{0,SIZE_T(layout_.tiles)*512};controlUpload_->Unmap(0,&written);}
        // Host retains this owner through command-list replay and GPU completion.
        // Record restores COMMON; an unrecorded allocation also starts COMMON.
        for(auto* p:{std::addressof(rays_),std::addressof(batches_),std::addressof(requests_),std::addressof(samples_),std::addressof(constants_)})
            SharedGIScratchPool().Return(std::move(*p),D3D12_HEAP_TYPE_DEFAULT);
        SharedGIScratchPool().Return(std::move(controlUpload_),D3D12_HEAP_TYPE_UPLOAD);
    }
    // Require native b4 and all GI6 SRVs actually observed for THIS recording.
    static constexpr UINT64 NativeRequired=1ull|(((1ull<<27)-1)<<5)|(1ull<<41)|(1ull<<42);
    HRESULT Initialize(ID3D12Device* device,const PTGlass::LightingTableShadow& native,
        UINT64 recording,UINT64 epoch,UINT64 version,UINT64 sceneAddress,
        UINT frame,UINT sceneRevision,UINT width,UINT height,UINT firstPixel,UINT pixels,UINT rays,float baseRange,bool deferred=false,bool singleUse=false) noexcept {
        if(device_||!device||!recording||!epoch||!version||!sceneAddress||sceneAddress%256||!sceneRevision||
           !width||!height||UINT64(width)*height>UINT32_MAX||!pixels||
           firstPixel>=UINT64(width)*height||pixels>UINT64(width)*height-firstPixel||!rays||rays>64||
           !std::isfinite(baseRange)||baseRange<=0||!native.Has(recording,NativeRequired))return E_INVALIDARG;
        GIFrameTiles layout;if(!GIFrameTiles::Make(width,height,firstPixel,pixels,rays,layout))return E_INVALIDARG;
        const auto source=native.Snapshot();if(!source.heap||source.count!=49)return E_INVALIDARG;
        const auto sourceDesc=source.heap->GetDesc();
        if(sourceDesc.Type!=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV||sourceDesc.Flags!=D3D12_DESCRIPTOR_HEAP_FLAG_NONE||
           source.first>sourceDesc.NumDescriptors||49>sourceDesc.NumDescriptors-source.first)return E_INVALIDARG;
        Ptr<ID3D12Device> sourceDevice;auto hr=source.heap->GetDevice(IID_PPV_ARGS(&sourceDevice));
        if(FAILED(hr)||sourceDevice.Get()!=device)return E_INVALIDARG;
        device_=device;step_=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=4*49;
        hr=device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&staging_));if(FAILED(hr))return hr;
        if(singleUse){hr=gate_.Initialize(device);if(FAILED(hr))return hr;bytes_+=8;}
        const UINT capacity=layout.capacity,total=capacity*rays;
        struct Allocation {UINT64 bytes;Ptr<ID3D12Resource>* resource;};
        const Allocation allocations[]={{UINT64(total)*sizeof(GIProducerRay),&rays_},{UINT64(capacity)*16,&batches_},
            {UINT64(total)*sizeof(GIProducerShadingRequest),&requests_},{UINT64(total)*sizeof(GIShadedProducerSample),&samples_}};
        for(const auto& a:allocations){hr=Buffer(a.bytes,*a.resource);if(FAILED(hr))return hr;}
        // Every tile copies its own immutable controls into one GPU CBV buffer.
        // Rewriting a shader-visible descriptor or upload slice between recorded
        // dispatches would make all dispatches see the final tile instead.
        hr=Buffer(512,constants_);if(FAILED(hr))return hr;
        hr=Buffer(UINT64(layout.tiles)*512,controlUpload_,D3D12_HEAP_TYPE_UPLOAD);if(FAILED(hr))return hr;
        void* mapped=nullptr;const D3D12_RANGE empty{};hr=controlUpload_->Map(0,&empty,&mapped);if(FAILED(hr))return hr;
        if(!mapped){controlUpload_->Unmap(0,&empty);return E_UNEXPECTED;}
        auto* data=static_cast<unsigned char*>(mapped);const SIZE_T controlBytes=SIZE_T(layout.tiles)*512;
        std::memset(data,0,controlBytes);
        for(UINT i=0;i<layout.tiles;++i){const auto span=layout.At(i);auto* dst=data+SIZE_T(i)*512;
            const UINT header[]={frame,deferred?0u:sceneRevision,width,height,span.first,span.pixels,rays};
            const UINT trace[]={frame,deferred?0u:sceneRevision,span.samples,0};
            std::memcpy(dst,header,sizeof(header));std::memcpy(dst+28,&baseRange,4);std::memcpy(dst+256,trace,sizeof(trace));
        }
        controls_=data;layout_=layout; // Persistently mapped, written only before first submission.
        auto from=source.heap->GetCPUDescriptorHandleForHeapStart();from.ptr+=SIZE_T(source.first)*step_;
        for(UINT stage=0;stage<4;++stage){
            device->CopyDescriptorsSimple(49,At(stage,0),from,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            D3D12_CONSTANT_BUFFER_VIEW_DESC cb{constants_->GetGPUVirtualAddress()+((stage==0||stage==3)?0:256),256};device->CreateConstantBufferView(&cb,At(stage,1));
        }
        if(gate_.Ready())for(UINT stage=0;stage<4;++stage){
            D3D12_SHADER_RESOURCE_VIEW_DESC flag{};flag.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;flag.Format=DXGI_FORMAT_R32_TYPELESS;
            flag.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;flag.Buffer.NumElements=1;flag.Buffer.Flags=D3D12_BUFFER_SRV_FLAG_RAW;
            device_->CreateShaderResourceView(gate_.Resource(),&flag,At(stage,40));
        }
        View(0,43,rays_.Get(),sizeof(GIProducerRay),total,true);View(0,44,batches_.Get(),16,capacity,true);
        View(1,32,rays_.Get(),sizeof(GIProducerRay),total,false);View(1,43,requests_.Get(),sizeof(GIProducerShadingRequest),total,true);
        D3D12_SHADER_RESOURCE_VIEW_DESC tlas{};tlas.ViewDimension=D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
        tlas.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;tlas.RaytracingAccelerationStructure.Location=sceneAddress;
        device->CreateShaderResourceView(nullptr,&tlas,At(1,33));
        View(2,32,requests_.Get(),sizeof(GIProducerShadingRequest),total,false);View(2,43,samples_.Get(),sizeof(GIShadedProducerSample),total,true);
        View(3,33,samples_.Get(),sizeof(GIShadedProducerSample),total,false);
        View(3,34,batches_.Get(),16,capacity,false);
        deferred_=deferred;sceneRevision_=sceneRevision;layout_=layout;pixels_=pixels;count_=total;firstPixel_=firstPixel;epoch_=epoch;version_=version;return S_OK;
    }
    // All ranges must be fresh native allocations in the SAME currently bound
    // heap. The host owns allocator retirement and supplies the native range to
    // exclude accidental writes to the original table or global material prefix.
    bool Publish(const PTGlass::DescriptorRange& native,const std::array<PTGlass::DescriptorRange,4>& fresh) noexcept {
        if(!pixels_||published_||!native.heap||native.count!=49)return false;
        const auto d=native.heap->GetDesc();
        if(d.Type!=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV||!(d.Flags&D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)||
           native.first>d.NumDescriptors||native.count>d.NumDescriptors-native.first)return false;
        Ptr<ID3D12Device> owner;if(FAILED(native.heap->GetDevice(IID_PPV_ARGS(&owner)))||owner.Get()!=device_.Get())return false;
        for(UINT i=0;i<4;++i){const auto& r=fresh[i];
            if(r.heap!=native.heap||r.count!=49||r.first<PTGlass::LightingTableLayout::NativeMaterialPrefix||
               r.first>d.NumDescriptors||r.count>d.NumDescriptors-r.first||
               (r.first<native.first+49&&native.first<r.first+49))return false;
            for(UINT j=0;j<i;++j)if(r.first<fresh[j].first+49&&fresh[j].first<r.first+49)return false;
        }
        for(UINT i=0;i<4;++i){auto target=native.heap->GetCPUDescriptorHandleForHeapStart();target.ptr+=SIZE_T(fresh[i].first)*step_;
            device_->CopyDescriptorsSimple(49,target,At(i,0),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            tables_[i]=native.heap->GetGPUDescriptorHandleForHeapStart();tables_[i].ptr+=UINT64(fresh[i].first)*step_;
        }
        visible_=native.heap;nativeTable_=native.heap->GetGPUDescriptorHandleForHeapStart().ptr+UINT64(native.first)*step_;published_=true;return true;
    }
    bool Record(ID3D12GraphicsCommandList4* list,const D3D12InitialGIPipeline& pipeline,
        const D3D12ProducerTable& hits,const ComputeBindingSnapshot& native) noexcept {
        if(!list||recorded_||!published_||(hits.Deferred()&&!deferred_)||!native.Ready()||native.root.Get()!=pipeline.Root()||
           pipeline.Gated()!=gate_.Ready()||!NativeBindingsMatch(native)||native.heaps[0].Get()!=visible_.Get()||hits.Pipeline()!=pipeline.Producer().State()||
           hits.Epoch()!=epoch_||hits.WriterVersion()!=version_||
           (list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT&&list->GetType()!=D3D12_COMMAND_LIST_TYPE_COMPUTE))return false;
        D3D12_DISPATCH_RAYS_DESC rays{};if(!pipeline.Producer().DispatchDescription(hits.Range(),count_,rays))return false;
        // One transaction: validate before issuing any commands. Restore keeps
        // the original heaps; injected calls must be muted in the native tracker.
        native.Restore(list);
        const auto transition=[list](ID3D12Resource* resource,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){
            D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            b.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};list->ResourceBarrier(1,&b);
        };
        for(auto* resource:{rays_.Get(),batches_.Get(),requests_.Get(),samples_.Get()})
            transition(resource,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        for(UINT tile=0;tile<layout_.tiles;++tile){const auto span=layout_.At(tile);
            transition(constants_.Get(),tile?D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER:D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_DEST);
            list->CopyBufferRegion(constants_.Get(),0,controlUpload_.Get(),UINT64(tile)*512,512);
            transition(constants_.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            if(tile)for(auto* resource:{rays_.Get(),batches_.Get(),requests_.Get(),samples_.Get()})
                transition(resource,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            list->SetPipelineState(pipeline.Proposals());list->SetComputeRootDescriptorTable(0,tables_[0]);list->Dispatch((span.pixels+63)/64,1,1);
            Readable(list,rays_.Get());Readable(list,batches_.Get());
            rays.Width=span.samples;
            list->SetPipelineState1(pipeline.Producer().State());list->SetComputeRootDescriptorTable(0,tables_[1]);list->DispatchRays(&rays);
            Readable(list,requests_.Get());
            list->SetPipelineState(pipeline.Shading());list->SetComputeRootDescriptorTable(0,tables_[2]);list->Dispatch((span.samples+63)/64,1,1);
            Readable(list,samples_.Get());
            D3D12_RESOURCE_BARRIER outputBarrier{};outputBarrier.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;
            if(tile==0)list->ResourceBarrier(1,&outputBarrier);
            list->SetPipelineState(pipeline.Resolve());list->SetComputeRootDescriptorTable(0,tables_[3]);list->Dispatch((span.pixels+63)/64,1,1);
            if(tile+1==layout_.tiles)list->ResourceBarrier(1,&outputBarrier);
        }
        if(gate_.Ready())gate_.Close(list);
        // Closed-list replay must start from the same private resource states.
        for(auto* resource:{rays_.Get(),batches_.Get(),requests_.Get(),samples_.Get()})
            transition(resource,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COMMON);
        transition(constants_.Get(),D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,D3D12_RESOURCE_STATE_COMMON);
        native.Restore(list);recordedProducer_=hits.Pipeline();recordedHits_=hits.Range().StartAddress;recorded_=true;return true;
    }
    // Called once after strict submission admission and a committed hit table.
    // The host must either precede submission or hold the entire prefix and
    // consumer behind an unreleased private GPU fence. Once that fence is
    // released, these controls must never be mutated. This is not a live toggle.
    HRESULT CommitAdmission(const D3D12ProducerTable& hits) noexcept {
        if(!deferred_||armed_||!recorded_||hits.Deferred()||hits.Pipeline()!=recordedProducer_.Get()||hits.Range().StartAddress!=recordedHits_||hits.Epoch()!=epoch_||hits.WriterVersion()!=version_)return E_INVALIDARG;
        if(!controls_)return E_UNEXPECTED;
        for(UINT i=0;i<layout_.tiles;++i){auto* dst=controls_+SIZE_T(i)*512;
            std::memcpy(dst+4,&sceneRevision_,4);std::memcpy(dst+260,&sceneRevision_,4);}
        armed_=true;return S_OK;
    }
    // Prefix is submitted only once and before the original list, on its queue.
    bool RecordGate(ID3D12GraphicsCommandList* prefix) const noexcept {
        if(!prefix||!recorded_||!gate_.Ready())return false;
        gate_.Arm(prefix,controlUpload_.Get(),4);return true;
    }
    HRESULT CancelAdmission() noexcept {
        if(!deferred_||!recorded_||!controls_)return E_INVALIDARG;
        const UINT zero=0;for(UINT i=0;i<layout_.tiles;++i){auto* dst=controls_+SIZE_T(i)*512;
            std::memcpy(dst+4,&zero,4);std::memcpy(dst+260,&zero,4);}
        armed_=false;return S_OK;
    }
    UINT FirstPixel()const noexcept{return firstPixel_;}
    UINT Pixels()const noexcept{return pixels_;}
    UINT64 RetainedBytes()const noexcept{return bytes_;}
};
}
