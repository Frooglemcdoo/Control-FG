#include "gpu_test_device.h"
#include "../src/gi_reuse/d3d12_saved_heap_switch.h"
#include "../src/gi_reuse/d3d12_native_gi_heap_lease.h"
#include "test-gi-native-heap-lease.inl"
int main()try{
 using namespace gi_reuse;Device d(false);
 const char* cs="cbuffer Input:register(b0){uint value;};cbuffer Index:register(b1){uint index;};RWStructuredBuffer<uint> output:register(u0);[numthreads(1,1,1)]void main(){output[index]=value;}";
 const char* vs="float4 main(uint id:SV_VertexID):SV_Position{return float4(id==2?3:-1,id==1?3:-1,0,1);}";
 const char* ps="cbuffer Input:register(b0){uint value;};uint main():SV_Target{return value;}";
 const auto compile=[](const char* source,const char* profile){ComPtr<ID3DBlob> code,error;
  Check(D3DCompile(source,strlen(source),nullptr,nullptr,nullptr,"main",profile,D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&code,&error));return code;};
 auto compute=compile(cs,"cs_5_0"),vertex=compile(vs,"vs_5_0"),pixel=compile(ps,"ps_5_0");
 D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_CBV,1,0,0,0};
 D3D12_ROOT_PARAMETER params[3]{};params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[0].DescriptorTable={1,&range};
 params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;params[1].Descriptor={0,0};
 params[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[2].Constants={1,0,1};
 D3D12_ROOT_SIGNATURE_DESC rd{3,params,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT};
 ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
 ComPtr<ID3D12RootSignature> root;Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 D3D12_COMPUTE_PIPELINE_STATE_DESC cd{};cd.pRootSignature=root.Get();cd.CS={compute->GetBufferPointer(),compute->GetBufferSize()};
 ComPtr<ID3D12PipelineState> pipeline;Check(d.device->CreateComputePipelineState(&cd,IID_PPV_ARGS(&pipeline)));
 TestNativeHeapLease(d,root.Get(),pipeline.Get());
 std::vector<UINT> av(64),bv(64);av[0]=17;bv[0]=29;auto a=d.Upload(av),b=d.Upload(bv);
 ComPtr<ID3D12DescriptorHeap> other;auto hd=d.views->GetDesc();Check(d.device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&other)));
 D3D12_CONSTANT_BUFFER_VIEW_DESC view{a->GetGPUVirtualAddress(),256};d.device->CreateConstantBufferView(&view,d.views->GetCPUDescriptorHandleForHeapStart());
 view.BufferLocation=b->GetGPUVirtualAddress();d.device->CreateConstantBufferView(&view,other->GetCPUDescriptorHandleForHeapStart());
 auto output=d.Buffer(8,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
 auto read=d.Buffer(8,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 ComputeBindingTracker tracker;tracker.Reset(pipeline.Get());tracker.Root(root.Get());
 ID3D12DescriptorHeap* heaps[]={d.views.Get()};tracker.Heaps(1,heaps);
 tracker.Descriptor(0,RootArgumentKind::Table,d.views->GetGPUDescriptorHandleForHeapStart().ptr);
 tracker.Descriptor(1,RootArgumentKind::UAV,output->GetGPUVirtualAddress());UINT index=0;tracker.Constants(2,0,1,&index);
 ComputeBindingSnapshot ray,shade;Require(tracker.Capture(ray),"ray snapshot");
 heaps[0]=other.Get();tracker.Heaps(1,heaps);tracker.Descriptor(0,RootArgumentKind::Table,other->GetGPUDescriptorHandleForHeapStart().ptr);
 index=1;tracker.Constants(2,0,1,&index);Require(tracker.Capture(shade),"shade snapshot");
 GraphicsTableState graphics;
 Require(!D3D12SavedHeapSwitch::Accept(ray,shade,graphics),"unknown graphics admitted heap switch");
 Require(D3D12SavedHeapSwitch::Accept(ray,ray,graphics),"same heaps needlessly rejected");
 graphics.Reset();graphics.Root(reinterpret_cast<UINT64>(root.Get()));graphics.Table(0,other->GetGPUDescriptorHandleForHeapStart().ptr);
 auto malformed=shade;malformed.heaps[0].Reset();Require(!D3D12SavedHeapSwitch::Accept(ray,malformed,graphics),"missing shader heap admitted");
 D3D12SavedHeapSwitch::Bind(d.list.Get(),shade);Require(shade.Restore(d.list.Get()),"initial shade restore");
 d.list->SetGraphicsRootSignature(root.Get());d.list->SetGraphicsRootDescriptorTable(0,other->GetGPUDescriptorHandleForHeapStart());
 Require(D3D12SavedHeapSwitch::ToRay(d.list.Get(),ray,shade,graphics),"switch to recorded ray heap");
 d.list->Dispatch(1,1,1);d.Uav(output.Get());
 Require(D3D12SavedHeapSwitch::ToShade(d.list.Get(),ray,shade,graphics),"restore consumer heap");d.list->Dispatch(1,1,1);
 // A real graphics draw follows without rebinding its descriptor table. The
 // helper must restore it after SetDescriptorHeaps invalidated graphics tables.
 D3D12_GRAPHICS_PIPELINE_STATE_DESC gd{};gd.pRootSignature=root.Get();gd.VS={vertex->GetBufferPointer(),vertex->GetBufferSize()};gd.PS={pixel->GetBufferPointer(),pixel->GetBufferSize()};
 gd.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;gd.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;gd.RasterizerState.DepthClipEnable=TRUE;
 gd.BlendState.RenderTarget[0].RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;gd.SampleMask=UINT_MAX;gd.DepthStencilState.DepthFunc=D3D12_COMPARISON_FUNC_LESS_EQUAL;
 gd.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;gd.NumRenderTargets=1;gd.RTVFormats[0]=DXGI_FORMAT_R32_UINT;gd.SampleDesc.Count=1;
 ComPtr<ID3D12PipelineState> graphicsPipeline;Check(d.device->CreateGraphicsPipelineState(&gd,IID_PPV_ARGS(&graphicsPipeline)));
 D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=td.Height=1;td.DepthOrArraySize=td.MipLevels=1;td.Format=DXGI_FORMAT_R32_UINT;td.SampleDesc.Count=1;td.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
 D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;hp.CreationNodeMask=hp.VisibleNodeMask=1;ComPtr<ID3D12Resource> target;Check(d.device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&target)));
 hd={};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;hd.NumDescriptors=1;ComPtr<ID3D12DescriptorHeap> rtv;Check(d.device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&rtv)));
 auto handle=rtv->GetCPUDescriptorHandleForHeapStart();d.device->CreateRenderTargetView(target.Get(),nullptr,handle);
 d.list->SetPipelineState(graphicsPipeline.Get());d.list->OMSetRenderTargets(1,&handle,FALSE,nullptr);
 D3D12_VIEWPORT viewport{0,0,1,1,0,1};D3D12_RECT scissor{0,0,1,1};d.list->RSSetViewports(1,&viewport);d.list->RSSetScissorRects(1,&scissor);
 d.list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);d.list->DrawInstanced(3,1,0,0);
 d.Transition(target.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);
 D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 bytes=0;d.device->GetCopyableFootprints(&td,0,1,0,&footprint,nullptr,nullptr,&bytes);
 auto graphicsRead=d.Buffer(bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 D3D12_TEXTURE_COPY_LOCATION dst{};dst.pResource=graphicsRead.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=footprint;
 D3D12_TEXTURE_COPY_LOCATION src{};src.pResource=target.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
 d.list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
 d.Transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);d.list->CopyBufferRegion(read.Get(),0,output.Get(),0,8);d.Submit();
 UINT* values=nullptr;Check(read->Map(0,nullptr,reinterpret_cast<void**>(&values)));Require(values[0]==17&&values[1]==29,"wrong compute heap contents");D3D12_RANGE empty{};read->Unmap(0,&empty);
 Check(graphicsRead->Map(0,nullptr,reinterpret_cast<void**>(&values)));Require(values[0]==29,"graphics table not restored");graphicsRead->Unmap(0,&empty);
 std::cout<<"PASS recorded heap switch: two compute descriptor tables and following graphics draw retain correct values; unknown graphics state rejected.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
