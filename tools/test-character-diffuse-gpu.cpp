#include "gpu_test_device.h"
#include <array>
static void Compile(){
const char* h=R"HLSL(
Texture2D<float4> G1:register(t0);Texture2D<float4> G2:register(t1);
Texture2D<float4> Diffuse:register(t3);Texture2D<uint> Map:register(t11);
RWStructuredBuffer<float4> Result:register(u0);
cbuffer F:register(b0){uint4 Depth:packoffset(c2);float4 Position:packoffset(c3);};
[numthreads(1,1,1)]void main(){
 uint2 p=(uint2)Position.xy;uint m=Map.Load(int3(p/2,0));
 uint bits=(m>>((p.x%2)*8+(p.y%2)*16))&3;
 uint2 c=(p&~1u)|uint2(bits&1,bits>>1);
 Result[0]=Diffuse.Load(int3(c,0));Result[1]=G1.Load(int3(c,0));Result[2]=G2.Load(int3(c,0));Result[3]=asfloat(Depth);
})HLSL";
ComPtr<ID3DBlob> b,e;HRESULT hr=D3DCompile(h,strlen(h),nullptr,nullptr,nullptr,"main","cs_5_1",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&b,&e);if(e)std::cerr<<(char*)e->GetBufferPointer();Check(hr);
std::ofstream f("build/diffuse-fixture-base.dxbc",std::ios::binary);f.write((char*)b->GetBufferPointer(),b->GetBufferSize());Require(bool(f),"write fixture shader");
}
static void Texture(Device& g,UINT slot,UINT width,UINT height,DXGI_FORMAT format,const std::vector<char>& bytes){
D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=width;d.Height=height;d.DepthOrArraySize=d.MipLevels=1;d.Format=format;d.SampleDesc.Count=1;
D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;ComPtr<ID3D12Resource> r;
Check(g.device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&r)));
D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT64 size=0;g.device->GetCopyableFootprints(&d,0,1,0,&fp,nullptr,nullptr,&size);
const UINT stride=format==DXGI_FORMAT_R32_UINT?4:16;Require(bytes.size()==size_t(width)*height*stride,"texture input size");
std::vector<char> padded(size_t(size),0);for(UINT y=0;y<height;++y)memcpy(padded.data()+fp.Offset+size_t(y)*fp.Footprint.RowPitch,bytes.data()+size_t(y)*width*stride,size_t(width)*stride);
auto u=g.Upload(padded);D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=u.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;from.PlacedFootprint=fp;to.pResource=r.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;g.list->CopyTextureRegion(&to,0,0,0,&from,nullptr);g.Transition(r.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=format;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;auto cpu=g.views->GetCPUDescriptorHandleForHeapStart();cpu.ptr+=size_t(slot)*g.descriptorSize;g.device->CreateShaderResourceView(r.Get(),&srv,cpu);g.retained.push_back(r);
}
int main(int argc,char** argv){try{
if(argc>1&&std::string(argv[1])=="--compile"){Compile();return 0;}
auto shader=Read("build/diffuse-fixture-filtered.dxbc");
for(unsigned test=0;test<8;++test){
 Device g(false);std::string prefix="build/diffuse-fixture-"+std::to_string(test)+"-";
 Texture(g,0,9,9,DXGI_FORMAT_R32G32B32A32_FLOAT,Read((prefix+"g1.bin").c_str()));Texture(g,1,9,9,DXGI_FORMAT_R32G32B32A32_FLOAT,Read((prefix+"g2.bin").c_str()));Texture(g,3,9,9,DXGI_FORMAT_R32G32B32A32_FLOAT,Read((prefix+"diffuse.bin").c_str()));Texture(g,11,5,5,DXGI_FORMAT_R32_UINT,Read((prefix+"map.bin").c_str()));
 D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,12,0,0,0};D3D12_ROOT_PARAMETER p[3]{};p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;p[0].Descriptor.ShaderRegister=0;p[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[1].DescriptorTable={1,&range};p[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;p[2].Descriptor.ShaderRegister=0;
 D3D12_ROOT_SIGNATURE_DESC rd{3,p,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> rb,error;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&rb,&error));ComPtr<ID3D12RootSignature> root;Check(g.device->CreateRootSignature(0,rb->GetBufferPointer(),rb->GetBufferSize(),IID_PPV_ARGS(&root)));
 D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={shader.data(),shader.size()};ComPtr<ID3D12PipelineState> ps;Check(g.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&ps)));
 auto constants=g.Upload(Read((prefix+"cb.bin").c_str()));auto out=g.Buffer(64,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);auto read=g.Buffer(64,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 g.list->SetComputeRootSignature(root.Get());g.list->SetPipelineState(ps.Get());g.list->SetComputeRootConstantBufferView(0,constants->GetGPUVirtualAddress());g.list->SetComputeRootDescriptorTable(1,g.views->GetGPUDescriptorHandleForHeapStart());g.list->SetComputeRootUnorderedAccessView(2,out->GetGPUVirtualAddress());g.list->Dispatch(1,1,1);g.Transition(out.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);g.list->CopyBufferRegion(read.Get(),0,out.Get(),0,64);g.Submit();
 auto expectedBytes=Read((prefix+"expected.bin").c_str());Require(expectedBytes.size()==16,"expected count");std::array<float,4> expected{};memcpy(expected.data(),expectedBytes.data(),16);void* mapped=nullptr;Check(read->Map(0,nullptr,&mapped));auto actual=(float*)mapped;
 for(unsigned c=0;c<4;++c)if(!std::isfinite(actual[c])||std::abs(actual[c]-expected[c])>0.0001f){std::cerr<<"case "<<test<<" channel "<<c<<" actual "<<actual[c]<<" expected "<<expected[c]<<'\n';throw std::runtime_error("diffuse GPU result mismatch");}
 read->Unmap(0,nullptr);
}
std::cout<<"PASS eight real WARP dispatch/readback cases for exact emitted diffuse filter: energy, noise, material/normal/radiance guards, border, MSAA fallback\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
