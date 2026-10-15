#pragma once
#include "pt_native_view.h"
#include "pt_upload_pool.h"
#include <algorithm>
#include <array>
namespace PTSurfaceComposition {
struct ReconstructionConstants {
    std::array<float,72> current{};
    std::array<float,16> previousWorldToView{},previousViewToClip{};
    UINT width=0,height=0,useHistory=0,maximumHistory=32;
};
static_assert(sizeof(ReconstructionConstants)==432,"HLSL reconstruction layout");
inline bool ReconstructConstants(const PTNativeView::View& view,const PTNativeView::View* previous,ReconstructionConstants& c){
    c={};c.width=view.width;c.height=view.height;c.maximumHistory=32;
    std::memcpy(c.current.data(),view.bytes.data(),288);
    if(!c.width||!c.height)return false;
    if(!previous||previous->width!=view.width||previous->height!=view.height||
       previous->frame==UINT32_MAX||previous->frame+1!=view.frame)return true;
    std::array<float,72> old{};std::memcpy(old.data(),previous->bytes.data(),288);
    double translation=0,alignment=0;
    for(unsigned i=0;i<3;++i){const double delta=c.current[36+i]-old[36+i];translation+=delta*delta;alignment+=double(c.current[32+i])*old[32+i];}
    if(translation>4||alignment<.85)return true; // Discontinuous camera, never reuse history.
    // The native ViewToClip field is not authenticated. Invert the validated
    // ClipToView explicitly rather than trusting unused native constants.
    double a[4][8]{};
    for(unsigned row=0;row<4;++row){for(unsigned col=0;col<4;++col)a[row][col]=old[56+row*4+col];a[row][row+4]=1;}
    for(unsigned col=0;col<4;++col){unsigned pivot=col;
        for(unsigned row=col+1;row<4;++row)if(std::abs(a[row][col])>std::abs(a[pivot][col]))pivot=row;
        if(!std::isfinite(a[pivot][col])||std::abs(a[pivot][col])<1e-12)return true;
        for(unsigned k=0;k<8;++k)std::swap(a[col][k],a[pivot][k]);
        const double scale=a[col][col];for(unsigned k=0;k<8;++k)a[col][k]/=scale;
        for(unsigned row=0;row<4;++row)if(row!=col){const double factor=a[row][col];for(unsigned k=0;k<8;++k)a[row][k]-=factor*a[col][k];}
    }
    for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col){
        const double value=a[row][col+4];if(!std::isfinite(value)||std::abs(value)>1e20)return true;
        c.previousViewToClip[row*4+col]=static_cast<float>(value);
    }
    std::copy(old.begin()+8,old.begin()+24,c.previousWorldToView.begin());c.useHistory=1;return true;
}
class ReconstructionPipeline {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;
public:
    HRESULT Initialize(ID3D12Device* device,const void* bytes,SIZE_T size){
        if(!device||!bytes||!size)return E_INVALIDARG;
        D3D12_ROOT_PARAMETER p[6]{};
        p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;p[0].Descriptor={0,0};
        p[1].ParameterType=p[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;p[1].Descriptor={0,0};p[2].Descriptor={1,0};
        p[3].ParameterType=p[5].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;p[3].Descriptor={0,0};p[5].Descriptor={1,0};
        D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,3,2,0,0};
        p[4].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[4].DescriptorTable={1,&range};
        D3D12_ROOT_SIGNATURE_DESC desc{};desc.NumParameters=6;desc.pParameters=p;
        Microsoft::WRL::ComPtr<ID3DBlob> blob,error;
        HRESULT hr=D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error);if(FAILED(hr))return hr;
        hr=device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root));if(FAILED(hr))return hr;
        D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={bytes,size};
        return device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pipeline));
    }
    ID3D12RootSignature* Root()const{return root.Get();}
    ID3D12PipelineState* Pipeline()const{return pipeline.Get();}
};
struct ReconstructionPass {
    std::shared_ptr<ReconstructionPipeline> pipeline;
    std::shared_ptr<PTUploadPool::Buffer> constants;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> guides;
    Microsoft::WRL::ComPtr<ID3D12Resource> raw,history;
    UINT width=0,height=0;bool historyEnabled=false;
    bool Initialize(ID3D12Device* device,const std::shared_ptr<ReconstructionPipeline>& p,const PTNativeView::View& view,
                    const PTNativeView::View* previous,ID3D12Resource* input,ID3D12Resource* old,
                    const std::array<D3D12_CPU_DESCRIPTOR_HANDLE,3>& descriptors){
        if(!device||!p||!input)return false;pipeline=p;raw=input;history=old?old:input;width=view.width;height=view.height;
        ReconstructionConstants c;if(!ReconstructConstants(view,old?previous:nullptr,c))return false;historyEnabled=c.useHistory!=0;
        if(FAILED(PTUploadPool::Buffer::Acquire(device,512,constants)))return false;
        void* data=nullptr;const D3D12_RANGE noRead{};
        if(FAILED(constants->Resource()->Map(0,&noRead,&data)))return false;
        std::memcpy(data,&c,sizeof(c));constants->Resource()->Unmap(0,nullptr);
        D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.NumDescriptors=3;hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&guides))))return false;
        auto cpu=guides->GetCPUDescriptorHandleForHeapStart();const auto step=device->GetDescriptorHandleIncrementSize(hd.Type);
        for(const auto d:descriptors){device->CopyDescriptorsSimple(1,cpu,d,hd.Type);cpu.ptr+=step;}return true;
    }
    void Record(ID3D12GraphicsCommandList* list,ID3D12Resource* output,ID3D12Resource* metrics){
        ID3D12DescriptorHeap* heaps[]={guides.Get()};list->SetDescriptorHeaps(1,heaps);
        list->SetComputeRootSignature(pipeline->Root());list->SetPipelineState(pipeline->Pipeline());
        list->SetComputeRootConstantBufferView(0,constants->Resource()->GetGPUVirtualAddress());
        list->SetComputeRootShaderResourceView(1,raw->GetGPUVirtualAddress());list->SetComputeRootShaderResourceView(2,history->GetGPUVirtualAddress());
        list->SetComputeRootUnorderedAccessView(3,output->GetGPUVirtualAddress());
        list->SetComputeRootDescriptorTable(4,guides->GetGPUDescriptorHandleForHeapStart());
        list->SetComputeRootUnorderedAccessView(5,metrics->GetGPUVirtualAddress());list->Dispatch((width+7)/8,(height+7)/8,1);
    }
};
}
