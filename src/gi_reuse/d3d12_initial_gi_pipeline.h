#pragma once
#include "d3d12_producer_pipeline.h"
#include "d3d12_producer_table.h"
#include "d3d12_compute_bindings.h"
#include "producer_ray.h"
#include "initial_reservoir.h"
#include "../pt_glass_descriptors.h"
namespace gi_reuse {
// Five stages sharing the admitted native GI6 root. Shader bytecode is prepared
// before recording; the host must own the current scene and publication leases.
class D3D12InitialGIPipeline {
    template<class T> using Ptr=Microsoft::WRL::ComPtr<T>;
    Ptr<ID3D12RootSignature> root_;
    Ptr<ID3D12PipelineState> proposals_,shading_,reservoirs_,resolve_;
    D3D12ProducerPipeline producer_;
public:
    HRESULT Initialize(ID3D12Device5* device,ID3D12RootSignature* root,
        D3D12_SHADER_BYTECODE proposals,D3D12_SHADER_BYTECODE trace,
        D3D12_SHADER_BYTECODE shading,D3D12_SHADER_BYTECODE reservoirs,D3D12_SHADER_BYTECODE resolve) noexcept {
        if(root_||producer_.State()||!device||!root)return E_INVALIDARG;
        for(const auto code:{proposals,trace,shading,reservoirs,resolve})if(!code.pShaderBytecode||!code.BytecodeLength)return E_INVALIDARG;
        Ptr<ID3D12PipelineState> a,b,c,e;
        D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};desc.pRootSignature=root;
        desc.CS=proposals;HRESULT hr=device->CreateComputePipelineState(&desc,IID_PPV_ARGS(&a));if(FAILED(hr))return hr;
        desc.CS=shading;hr=device->CreateComputePipelineState(&desc,IID_PPV_ARGS(&b));if(FAILED(hr))return hr;
        desc.CS=reservoirs;hr=device->CreateComputePipelineState(&desc,IID_PPV_ARGS(&c));if(FAILED(hr))return hr;
        desc.CS=resolve;hr=device->CreateComputePipelineState(&desc,IID_PPV_ARGS(&e));if(FAILED(hr))return hr;
        hr=producer_.Initialize(device,root,trace);if(FAILED(hr))return hr;
        root_=root;proposals_=std::move(a);shading_=std::move(b);reservoirs_=std::move(c);resolve_=std::move(e);return S_OK;
    }
    ID3D12RootSignature* Root() const noexcept{return root_.Get();}
    const D3D12ProducerPipeline& Producer() const noexcept{return producer_;}
    ID3D12PipelineState* Proposals() const noexcept{return proposals_.Get();}
    ID3D12PipelineState* Shading() const noexcept{return shading_.Get();}
    ID3D12PipelineState* Resolve() const noexcept{return resolve_.Get();}
    ID3D12PipelineState* Reservoirs() const noexcept{return reservoirs_.Get();}
};
// Single-recording bounded tile. CPU descriptors and constants are immutable
// after preparation. Retain this owner, its pipeline, the native binding owners,
// the producer table and the selected TLAS/BLAS closure through queue completion.
// Descriptor addresses are allocated by the native same-heap allocator; no heap
// switching or shader-visible descriptor reads are permitted here.
class D3D12InitialGITile {
    template<class T> using Ptr=Microsoft::WRL::ComPtr<T>;
    Ptr<ID3D12Device> device_;
    Ptr<ID3D12DescriptorHeap> staging_;
    Ptr<ID3D12Resource> rays_,batches_,requests_,samples_,reservoirs_,outputs_,constants_;
    std::array<D3D12_GPU_DESCRIPTOR_HANDLE,5> tables_{};
    Ptr<ID3D12DescriptorHeap> visible_;
    UINT pixels_=0,count_=0,firstPixel_=0,step_=0;
    UINT64 bytes_=0,epoch_=0,version_=0,nativeTable_=0;
    bool recorded_=false,published_=false;
    D3D12_CPU_DESCRIPTOR_HANDLE At(UINT stage,UINT slot) const noexcept {
        auto h=staging_->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(stage*49+slot)*step_;return h;
    }
    HRESULT Buffer(UINT64 bytes,Ptr<ID3D12Resource>& out,D3D12_HEAP_TYPE type=D3D12_HEAP_TYPE_DEFAULT) noexcept {
        D3D12_HEAP_PROPERTIES heap{};heap.Type=type;heap.CreationNodeMask=heap.VisibleNodeMask=1;
        D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=bytes;d.Height=1;
        d.DepthOrArraySize=d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        if(type==D3D12_HEAP_TYPE_DEFAULT)d.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        auto hr=device_->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,
            type==D3D12_HEAP_TYPE_DEFAULT?D3D12_RESOURCE_STATE_UNORDERED_ACCESS:D3D12_RESOURCE_STATE_GENERIC_READ,
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
    // Require native b4 and all GI6 SRVs actually observed for THIS recording.
    static constexpr UINT64 NativeRequired=1ull|(((1ull<<27)-1)<<5)|(1ull<<41)|(1ull<<42);
    HRESULT Initialize(ID3D12Device* device,const PTGlass::LightingTableShadow& native,
        UINT64 recording,UINT64 epoch,UINT64 version,UINT64 sceneAddress,
        UINT frame,UINT sceneRevision,UINT width,UINT height,UINT firstPixel,UINT pixels,UINT rays,float baseRange) noexcept {
        if(device_||!device||!recording||!epoch||!version||!sceneAddress||sceneAddress%256||!sceneRevision||
           !width||!height||UINT64(width)*height>UINT32_MAX||!pixels||pixels>16384||
           firstPixel>=UINT64(width)*height||pixels>UINT64(width)*height-firstPixel||!rays||rays>64||
           !std::isfinite(baseRange)||baseRange<=0||!native.Has(recording,NativeRequired))return E_INVALIDARG;
        const auto source=native.Snapshot();if(!source.heap||source.count!=49)return E_INVALIDARG;
        const auto sourceDesc=source.heap->GetDesc();
        if(sourceDesc.Type!=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV||sourceDesc.Flags!=D3D12_DESCRIPTOR_HEAP_FLAG_NONE||
           source.first>sourceDesc.NumDescriptors||49>sourceDesc.NumDescriptors-source.first)return E_INVALIDARG;
        Ptr<ID3D12Device> sourceDevice;auto hr=source.heap->GetDevice(IID_PPV_ARGS(&sourceDevice));
        if(FAILED(hr)||sourceDevice.Get()!=device)return E_INVALIDARG;
        device_=device;step_=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=5*49;
        hr=device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&staging_));if(FAILED(hr))return hr;
        const UINT total=pixels*rays;
        struct Allocation {UINT64 bytes;Ptr<ID3D12Resource>* resource;};
        const Allocation allocations[]={{UINT64(total)*sizeof(GIProducerRay),&rays_},{UINT64(pixels)*16,&batches_},
            {UINT64(total)*sizeof(GIProducerShadingRequest),&requests_},{UINT64(total)*sizeof(GIShadedProducerSample),&samples_},
            {UINT64(pixels)*sizeof(GIInitialReservoir),&reservoirs_},{UINT64(pixels)*sizeof(GIInitialResult),&outputs_}};
        for(const auto& a:allocations){hr=Buffer(a.bytes,*a.resource);if(FAILED(hr))return hr;}
        hr=Buffer(512,constants_,D3D12_HEAP_TYPE_UPLOAD);if(FAILED(hr))return hr;
        std::array<unsigned char,512> data{};
        const UINT header[]={frame,sceneRevision,width,height,firstPixel,pixels,rays};
        const UINT trace[]={frame,sceneRevision,total,0};
        std::memcpy(data.data(),header,sizeof(header));std::memcpy(data.data()+28,&baseRange,4);std::memcpy(data.data()+256,trace,sizeof(trace));
        void* mapped=nullptr;const D3D12_RANGE empty{};hr=constants_->Map(0,&empty,&mapped);if(FAILED(hr))return hr;
        if(!mapped){constants_->Unmap(0,&empty);return E_UNEXPECTED;}
        std::memcpy(mapped,data.data(),data.size());const D3D12_RANGE written{0,data.size()};constants_->Unmap(0,&written);
        auto from=source.heap->GetCPUDescriptorHandleForHeapStart();from.ptr+=SIZE_T(source.first)*step_;
        for(UINT stage=0;stage<5;++stage){
            device->CopyDescriptorsSimple(49,At(stage,0),from,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            D3D12_CONSTANT_BUFFER_VIEW_DESC cb{constants_->GetGPUVirtualAddress()+((stage==0||stage==4)?0:256),256};device->CreateConstantBufferView(&cb,At(stage,1));
        }
        View(0,43,rays_.Get(),sizeof(GIProducerRay),total,true);View(0,44,batches_.Get(),16,pixels,true);
        View(1,32,rays_.Get(),sizeof(GIProducerRay),total,false);View(1,43,requests_.Get(),sizeof(GIProducerShadingRequest),total,true);
        D3D12_SHADER_RESOURCE_VIEW_DESC tlas{};tlas.ViewDimension=D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
        tlas.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;tlas.RaytracingAccelerationStructure.Location=sceneAddress;
        device->CreateShaderResourceView(nullptr,&tlas,At(1,33));
        View(2,32,requests_.Get(),sizeof(GIProducerShadingRequest),total,false);View(2,43,samples_.Get(),sizeof(GIShadedProducerSample),total,true);
        View(3,32,samples_.Get(),sizeof(GIShadedProducerSample),total,false);View(3,33,batches_.Get(),16,pixels,false);
        View(3,43,reservoirs_.Get(),sizeof(GIInitialReservoir),pixels,true);View(3,44,outputs_.Get(),sizeof(GIInitialResult),pixels,true);
        View(4,32,outputs_.Get(),sizeof(GIInitialResult),pixels,false);View(4,33,samples_.Get(),sizeof(GIShadedProducerSample),total,false);
        View(4,34,batches_.Get(),16,pixels,false);
        pixels_=pixels;count_=total;firstPixel_=firstPixel;epoch_=epoch;version_=version;return S_OK;
    }
    // All ranges must be fresh native allocations in the SAME currently bound
    // heap. The host owns allocator retirement and supplies the native range to
    // exclude accidental writes to the original table or global material prefix.
    bool Publish(const PTGlass::DescriptorRange& native,const std::array<PTGlass::DescriptorRange,5>& fresh) noexcept {
        if(!pixels_||published_||!native.heap||native.count!=49)return false;
        const auto d=native.heap->GetDesc();
        if(d.Type!=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV||!(d.Flags&D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)||
           native.first>d.NumDescriptors||native.count>d.NumDescriptors-native.first)return false;
        Ptr<ID3D12Device> owner;if(FAILED(native.heap->GetDevice(IID_PPV_ARGS(&owner)))||owner.Get()!=device_.Get())return false;
        for(UINT i=0;i<5;++i){const auto& r=fresh[i];
            if(r.heap!=native.heap||r.count!=49||r.first<PTGlass::LightingTableLayout::NativeMaterialPrefix||
               r.first>d.NumDescriptors||r.count>d.NumDescriptors-r.first||
               (r.first<native.first+49&&native.first<r.first+49))return false;
            for(UINT j=0;j<i;++j)if(r.first<fresh[j].first+49&&fresh[j].first<r.first+49)return false;
        }
        for(UINT i=0;i<5;++i){auto target=native.heap->GetCPUDescriptorHandleForHeapStart();target.ptr+=SIZE_T(fresh[i].first)*step_;
            device_->CopyDescriptorsSimple(49,target,At(i,0),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            tables_[i]=native.heap->GetGPUDescriptorHandleForHeapStart();tables_[i].ptr+=UINT64(fresh[i].first)*step_;
        }
        visible_=native.heap;nativeTable_=native.heap->GetGPUDescriptorHandleForHeapStart().ptr+UINT64(native.first)*step_;published_=true;return true;
    }
    bool Record(ID3D12GraphicsCommandList4* list,const D3D12InitialGIPipeline& pipeline,
        const D3D12ProducerTable& hits,const ComputeBindingSnapshot& native) noexcept {
        if(!list||recorded_||!published_||!native.Ready()||native.root.Get()!=pipeline.Root()||
           !NativeBindingsMatch(native)||native.heaps[0].Get()!=visible_.Get()||hits.Pipeline()!=pipeline.Producer().State()||
           hits.Epoch()!=epoch_||hits.WriterVersion()!=version_||
           (list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT&&list->GetType()!=D3D12_COMMAND_LIST_TYPE_COMPUTE))return false;
        D3D12_DISPATCH_RAYS_DESC rays{};if(!pipeline.Producer().DispatchDescription(hits.Range(),count_,rays))return false;
        // One transaction: validate before issuing any commands. Restore keeps
        // the original heaps; injected calls must be muted in the native tracker.
        native.Restore(list);
        list->SetPipelineState(pipeline.Proposals());list->SetComputeRootDescriptorTable(0,tables_[0]);list->Dispatch((pixels_+63)/64,1,1);
        Readable(list,rays_.Get());Readable(list,batches_.Get());
        list->SetPipelineState1(pipeline.Producer().State());list->SetComputeRootDescriptorTable(0,tables_[1]);list->DispatchRays(&rays);
        Readable(list,requests_.Get());
        list->SetPipelineState(pipeline.Shading());list->SetComputeRootDescriptorTable(0,tables_[2]);list->Dispatch((count_+63)/64,1,1);
        Readable(list,samples_.Get());
        list->SetPipelineState(pipeline.Reservoirs());list->SetComputeRootDescriptorTable(0,tables_[3]);list->Dispatch((pixels_+63)/64,1,1);
        Readable(list,reservoirs_.Get());Readable(list,outputs_.Get());
        D3D12_RESOURCE_BARRIER outputBarrier{};outputBarrier.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;
        list->ResourceBarrier(1,&outputBarrier);
        list->SetPipelineState(pipeline.Resolve());list->SetComputeRootDescriptorTable(0,tables_[4]);list->Dispatch((pixels_+63)/64,1,1);
        list->ResourceBarrier(1,&outputBarrier);
        native.Restore(list);recorded_=true;return true;
    }
    ID3D12Resource* Reservoirs()const noexcept{return reservoirs_.Get();}
    ID3D12Resource* Outputs()const noexcept{return outputs_.Get();}
    UINT FirstPixel()const noexcept{return firstPixel_;}
    UINT Pixels()const noexcept{return pixels_;}
    UINT64 RetainedBytes()const noexcept{return bytes_;}
};
}
