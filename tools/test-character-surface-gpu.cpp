#include "gpu_test_device.h"
#include <atomic>
#include <d3d12shader.h>
#include <d3d12sdklayers.h>
#include <cstdarg>
#include <sstream>
namespace native_lighting {
inline constexpr GUID kKindGuid{0x72134610,0x3213,0x4321,{1,2,3,4,5,6,7,8}};
static void Trace(const char* format,...){va_list args;va_start(args,format);vprintf(format,args);va_end(args);puts("");}
#include "../src/character_surface_runtime.h"
}
static HRESULT STDMETHODCALLTYPE Create(ID3D12Device* d,const D3D12_GRAPHICS_PIPELINE_STATE_DESC* p,REFIID iid,void** out){return d->CreateGraphicsPipelineState(p,iid,out);}
int main(){try{
    using namespace native_lighting;
    namespace surface=character_surface;
    ComPtr<ID3D12Debug> debug;
    if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))debug->EnableDebugLayer();
    std::cout<<"Create WARP device\n";Device gpu(false);
    for(unsigned q=0;q<3;++q){
        Require(surface::SelectedVariant(0,q)==(q==0?-1:q==1?1:0),"Gameplay quality selection mismatch");
        Require(surface::SelectedVariant(1,q)==-1 && surface::SelectedVariant(8,q)==-1,"Native comparison is not native");
        Require(surface::SelectedVariant(2,q)==0 && surface::SelectedVariant(3,q)==1,"Quality comparison mismatch");
    }
    for(unsigned family=0;family<surface::kCharacterFamilyCount;++family){
    const auto native=surface::kCharacterOriginals[family];
    const unsigned before=surface::matched.load();
    std::cout<<"Family "<<family<<"\n";
    std::cout<<"Reflect native character shader\n";
    ComPtr<ID3D12ShaderReflection> reflect;
    Check(D3DReflect(native.pShaderBytecode,native.BytecodeLength,IID_PPV_ARGS(&reflect)));
    D3D12_SHADER_DESC info{};Check(reflect->GetDesc(&info));
    std::ostringstream source;source<<"struct Output {";
    std::vector<std::pair<UINT,bool>> fields;
    for(UINT i=0;i<info.InputParameters;++i){
        D3D12_SIGNATURE_PARAMETER_DESC p{};Check(reflect->GetInputParameterDesc(i,&p));
        if(p.SystemValueType==D3D_NAME_IS_FRONT_FACE || p.SystemValueType==D3D_NAME_PRIMITIVE_ID || p.SystemValueType==D3D_NAME_SAMPLE_INDEX)continue;
        const bool integer=p.ComponentType!=D3D_REGISTER_COMPONENT_FLOAT32;
        unsigned components=0;for(unsigned bit=0;bit<4;++bit)if(p.Mask&(1u<<bit))++components;
        source<<(integer?"nointerpolation ":"")<<(p.ComponentType==D3D_REGISTER_COMPONENT_UINT32?"uint":p.ComponentType==D3D_REGISTER_COMPONENT_SINT32?"int":"float")<<components<<" f"<<i<<":"<<p.SemanticName<<p.SemanticIndex<<";";
        fields.push_back({i,p.SystemValueType==D3D_NAME_POSITION});
    }
    source<<"}; Output main(uint id:SV_VertexID){Output o;";
    for(auto f:fields)source<<"o.f"<<f.first<<(f.second?"=float4((id==1)?3:-1,(id==2)?3:-1,0,1);":"=0;");
    source<<"return o;}";
    std::cout<<"Compile fixture vertex shader\n";
    const auto hlsl=source.str();ComPtr<ID3DBlob> vs,error;
    const auto hr=D3DCompile(hlsl.data(),hlsl.size(),nullptr,nullptr,nullptr,"main","vs_5_1",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&vs,&error);
    if(error)std::cerr<<static_cast<const char*>(error->GetBufferPointer());Check(hr);
    D3D12_DESCRIPTOR_RANGE ranges[2]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,24,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,UINT_MAX,0,1,0}};
    D3D12_ROOT_PARAMETER params[10]{};
    for(UINT i=0;i<8;++i){params[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;params[i].Descriptor.ShaderRegister=i;params[i].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;}
    for(UINT i=0;i<2;++i){params[8+i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[8+i].DescriptorTable={1,&ranges[i]};params[8+i].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;}
    D3D12_STATIC_SAMPLER_DESC samplers[3]{};
    for(UINT i=0;i<3;++i){auto& s=samplers[i];s.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;s.AddressU=s.AddressV=s.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;s.ShaderRegister=i==0?6:i==1?19:0;s.RegisterSpace=1;s.MaxLOD=D3D12_FLOAT32_MAX;s.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;s.MaxAnisotropy=1;s.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;}
    samplers[2].Filter=D3D12_FILTER_MIN_MAG_MIP_POINT;samplers[2].AddressU=samplers[2].AddressV=samplers[2].AddressW=D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    D3D12_ROOT_SIGNATURE_DESC rs{10,params,3,samplers,D3D12_ROOT_SIGNATURE_FLAG_NONE};
    std::cout<<"Create compatible root signature\n";
    ComPtr<ID3DBlob> rootBytes;Check(D3D12SerializeRootSignature(&rs,D3D_ROOT_SIGNATURE_VERSION_1,&rootBytes,&error));
    ComPtr<ID3D12RootSignature> root;Check(gpu.device->CreateRootSignature(0,rootBytes->GetBufferPointer(),rootBytes->GetBufferSize(),IID_PPV_ARGS(&root)));
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};desc.pRootSignature=root.Get();desc.VS={vs->GetBufferPointer(),vs->GetBufferSize()};desc.PS=native;
    auto& blend=desc.BlendState.RenderTarget[0];blend.SrcBlend=D3D12_BLEND_ONE;blend.DestBlend=D3D12_BLEND_ZERO;blend.BlendOp=D3D12_BLEND_OP_ADD;
    blend.SrcBlendAlpha=D3D12_BLEND_ONE;blend.DestBlendAlpha=D3D12_BLEND_ZERO;blend.BlendOpAlpha=D3D12_BLEND_OP_ADD;blend.LogicOp=D3D12_LOGIC_OP_NOOP;blend.RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
    desc.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;desc.DepthStencilState.DepthFunc=D3D12_COMPARISON_FUNC_LESS;
    desc.DepthStencilState.StencilReadMask=D3D12_DEFAULT_STENCIL_READ_MASK;desc.DepthStencilState.StencilWriteMask=D3D12_DEFAULT_STENCIL_WRITE_MASK;
    desc.DepthStencilState.FrontFace={D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_COMPARISON_FUNC_ALWAYS};desc.DepthStencilState.BackFace=desc.DepthStencilState.FrontFace;
    desc.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;desc.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;desc.RasterizerState.DepthClipEnable=TRUE;
    desc.SampleMask=UINT_MAX;desc.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;desc.NumRenderTargets=1;desc.RTVFormats[0]=DXGI_FORMAT_R16G16B16A16_FLOAT;desc.SampleDesc.Count=1;
    std::cout<<"Create original character graphics PSO\n";
    ComPtr<ID3D12PipelineState> original;const auto createHr=gpu.device->CreateGraphicsPipelineState(&desc,IID_PPV_ARGS(&original));
    if(FAILED(createHr)){
        ComPtr<ID3D12InfoQueue> queue;if(SUCCEEDED(gpu.device.As(&queue))){
            for(UINT64 i=0;i<queue->GetNumStoredMessages();++i){SIZE_T length=0;queue->GetMessage(i,nullptr,&length);std::vector<unsigned char> bytes(length);auto* msg=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());if(SUCCEEDED(queue->GetMessage(i,msg,&length)))std::cerr<<msg->pDescription<<'\n';}
        }
    }Check(createHr);
    surface::Observe(gpu.device.Get(),&desc,original.Get(),&Create);
    Require(surface::matched.load()==before+1 && surface::ready.load()==before+1,"Surface variants not admitted");
    unsigned tag=0;UINT size=sizeof(tag);Check(original->GetPrivateData(kKindGuid,&size,&tag));Require(tag==surface::kind,"Surface tag mismatch");
    unsigned group=99;size=sizeof(group);Check(original->GetPrivateData(surface::VariantGuid(6),&size,&group));Require(group==surface::kCharacterGroups[family],"Material coverage group mismatch");
    for(UINT i=0;i<2;++i){ID3D12PipelineState* p=nullptr;size=sizeof(p);Check(original->GetPrivateData(surface::VariantGuid(i),&size,&p));Require(p!=nullptr,"Missing owned variant");p->Release();}
    surface::Observe(gpu.device.Get(),&desc,original.Get(),&Create);Require(surface::matched.load()==before+1,"Duplicate preparation");
    // Stripped reflection must still resolve to the same exact executable family.
    {
        Require(surface::Family(native)==int(family),"Native family mismatch");
        ComPtr<ID3DBlob> stripped;Check(D3DStripShader(native.pShaderBytecode,native.BytecodeLength,D3DCOMPILER_STRIP_REFLECTION_DATA|D3DCOMPILER_STRIP_DEBUG_INFO,&stripped));
        desc.PS={stripped->GetBufferPointer(),stripped->GetBufferSize()};
        Require(surface::Family(desc.PS)==int(family),"Stripped executable not recognized");
        ComPtr<ID3D12PipelineState> live;Check(gpu.device->CreateGraphicsPipelineState(&desc,IID_PPV_ARGS(&live)));
        surface::Observe(gpu.device.Get(),&desc,live.Get(),&Create);
        Require(surface::matched.load()==before+2 && surface::ready.load()==before+2,"Stripped variant creation failed");
        for(UINT i=0;i<2;++i){ID3D12PipelineState* p=nullptr;size=sizeof(p);Check(live->GetPrivateData(surface::VariantGuid(i),&size,&p));Require(p!=nullptr,"Missing stripped variant");p->Release();}
        auto changed=std::vector<unsigned char>(static_cast<const unsigned char*>(native.pShaderBytecode),static_cast<const unsigned char*>(native.pShaderBytecode)+native.BytecodeLength);
        const auto chunk=surface::Chunk(native,"SHEX");
        changed[static_cast<const unsigned char*>(chunk.pShaderBytecode)-static_cast<const unsigned char*>(native.pShaderBytecode)+400]^=1;
        Require(surface::Family({changed.data(),changed.size()})==-1,"Changed instructions admitted");
        changed.resize(31);Require(surface::Family({changed.data(),changed.size()})==-1,"Truncated container admitted");
    }
    }
    std::cout<<"PASS all twelve skin/cloth/hair families and stripped containers; both quality variants each on WARP; changed instructions rejected\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
