#include "gpu_test_device.h"
#include "../src/gi_reuse/d3d12_diagnostic_sampler.h"
#include <array>
int main()try{
 Device d(false);D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=2;td.Height=2;td.DepthOrArraySize=1;td.MipLevels=2;td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;td.SampleDesc.Count=1;
 D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;ComPtr<ID3D12Resource> tex;Check(d.device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&tex)));
 std::array<D3D12_PLACED_SUBRESOURCE_FOOTPRINT,2> layouts{};std::array<UINT,2> rows{};std::array<UINT64,2> rowBytes{};UINT64 total=0;
 d.device->GetCopyableFootprints(&td,0,2,0,layouts.data(),rows.data(),rowBytes.data(),&total);
 // Nonconstant mip 0 catches nearest-vs-bilinear and wrap-edge errors.
 const float colors[5][4]={{0,0,.25f,1},{1,0,.25f,1},{0,1,.25f,1},{1,1,.25f,1},{.125f,.75f,.5f,1}};
 std::vector<unsigned char> bytes(size_t(total),0);
 for(UINT mip=0;mip<2;++mip)for(UINT y=0;y<layouts[mip].Footprint.Height;++y)for(UINT x=0;x<layouts[mip].Footprint.Width;++x)std::memcpy(bytes.data()+layouts[mip].Offset+UINT64(y)*layouts[mip].Footprint.RowPitch+x*16,colors[mip?4:y*2+x],16);
 auto upload=d.Upload(bytes);for(UINT mip=0;mip<2;++mip){D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=upload.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint=layouts[mip];dst.pResource=tex.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;dst.SubresourceIndex=mip;d.list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);}d.Transition(tex.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
 for(UINT i=0;i<3;++i){D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=td.Format;v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;v.Texture2D.MostDetailedMip=i==2?1:0;v.Texture2D.MipLevels=i==0?2:1;auto handle=d.views->GetCPUDescriptorHandleForHeapStart();handle.ptr+=SIZE_T(i)*d.descriptorSize;d.device->CreateShaderResourceView(tex.Get(),&v,handle);}
 auto code=Read("texture-probe.dxil");std::array<ComPtr<ID3D12RootSignature>,2> roots;std::array<ComPtr<ID3D12PipelineState>,2> pipelines;
 D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0};D3D12_ROOT_PARAMETER p[3]{};p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;p[0].Constants={0,0,1};p[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[1].DescriptorTable={1,&range};p[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;p[2].Descriptor={0,0};
 for(UINT clamp=0;clamp<2;++clamp){D3D12_STATIC_SAMPLER_DESC samplers[2]={gi_reuse::GIDiagnosticSampler(),gi_reuse::GIDiagnosticSampler()};samplers[0].RegisterSpace=0;samplers[0].MaxLOD=clamp?0.f:16.f;
  D3D12_ROOT_SIGNATURE_DESC rd{3,p,2,samplers,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&roots[clamp])));D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=roots[clamp].Get();pd.CS={code.data(),code.size()};Check(d.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pipelines[clamp])));
 }
 constexpr UINT count=12,stride=192;auto output=d.Buffer(count*stride,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);auto read=d.Buffer(count*stride,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 for(UINT i=0;i<count;++i){UINT scenario=i%4,clamp=scenario==1?1:0,view=scenario<2?0:scenario-1;auto handle=d.views->GetGPUDescriptorHandleForHeapStart();handle.ptr+=UINT64(view)*d.descriptorSize;d.list->SetComputeRootSignature(roots[clamp].Get());d.list->SetPipelineState(pipelines[clamp].Get());d.list->SetComputeRoot32BitConstant(0,i,0);d.list->SetComputeRootDescriptorTable(1,handle);d.list->SetComputeRootUnorderedAccessView(2,output->GetGPUVirtualAddress());d.list->Dispatch(1,1,1);d.Uav(output.Get());}
 d.Transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);d.list->CopyBufferRegion(read.Get(),0,output.Get(),0,count*stride);d.Submit();UINT* data=nullptr;Check(read->Map(0,nullptr,reinterpret_cast<void**>(&data)));
 // UV .375,.625 => .25,.75; seam UV 0,1 => .5,.5; negative/repeated UV -.125,1.125 => .75,.25.
 const float expected[3][4]={{.25f,.75f,.25f,1},{.5f,.5f,.25f,1},{.75f,.25f,.25f,1}};
 for(UINT i=0;i<count;++i){const UINT scenario=i%4;const UINT* u=data+i*48;const auto* f=reinterpret_cast<const float*>(u);Require(u[0]==123&&u[4]==(scenario==3?1u:2u)&&u[5]==(scenario==3?1u:2u)&&u[6]==(scenario<2?2u:1u),"view mip dimensions incorrect");Require(u[3]==(scenario<2?7u:3u),"probe flags incorrect");
  const float* base=scenario==3?colors[4]:expected[i/4];const float* level=scenario==2?base:colors[4];
  for(UINT c=0;c<4;++c){auto close=[](float a,float b){return std::abs(a-b)<1e-6f;};Require(close(f[12+c],base[c]),"native sample0 incorrect");Require(close(f[16+c],scenario==1?base[c]:level[c]),"native sample1 clamp incorrect");Require(close(f[32+c],base[c])&&close(f[36+c],level[c]),"controlled sampler did not remove native clamp");Require(close(f[40+c],base[c])&&close(f[44+c],level[c]),"manual bilinear mismatch at interior/wrap seam/negative UV or single-mip view");Require(f[24+c]==(scenario<2?colors[4][c]:0.f),"nearest Load1 availability wrong");}
 }
 D3D12_RANGE empty{};read->Unmap(0,&empty);std::cout<<"PASS actual probe and manual bilinear: gradient texture, sampler maxLOD=0, single/shifted views, interior/seam/negative UV; controlled sampler isolated.\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
