// Uses real WARP textures and exact native wrapper shapes. It does not pretend
// these wrappers establish a native scene lease or execute a GI ray.
#include "../src/gi_reuse/d3d12_native_gi_textures.h"
static void TestNativeGITextures(Device& d){
 using namespace gi_reuse;
 struct alignas(8) Wrapper {std::array<unsigned char,0x90> native{};std::array<unsigned char,16> container{};
  std::array<unsigned char,0x28> tracker{};ComPtr<ID3D12Resource> resource;
  void State(UINT state,bool external=false){
   void* address=external?tracker.data():nullptr;std::memcpy(native.data()+0x50,&address,8);
   std::memcpy(external?tracker.data()+0x20:native.data()+0x60,&state,4);
  }
 };
 std::array<Wrapper,4> textures;
 D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
 D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=18;desc.Height=3;
 desc.DepthOrArraySize=desc.MipLevels=1;desc.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;desc.SampleDesc.Count=1;
 desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
 for(auto& t:textures){
  desc.MipLevels=(&t==&textures[3])?5:1;
  Check(d.device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&t.resource)));
  auto* resource=t.resource.Get();std::memcpy(t.native.data()+0x88,&resource,8);
  void* native=t.native.data();std::memcpy(t.container.data()+8,&native,8);t.State(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 }
 std::array<const void*,3> bundle{};for(unsigned i=0;i<3;++i)bundle[i]=textures[i].container.data();
 alignas(8) std::array<unsigned char,0x248> owner{};const void* address=bundle.data();std::memcpy(owner.data()+0x240,&address,8);
 textures[2].State(D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
 D3D12NativeGITextures acquired;
 Require(acquired.AcquireAtRay(owner.data(),d.device.Get(),18,3),"native hit acquisition failed");
 Require(!acquired.AcquireAtRay(owner.data(),d.device.Get(),18,3),"live native hit owner overwritten");
 for(unsigned i=0;i<3;++i)Require(acquired.Hits()[i]==textures[i].resource.Get(),"wrong retained hit");
 std::array<D3D12_RESOURCE_STATES,3> states{};
 Require(!acquired.ReadAtShade(owner.data(),states),"writable hits admitted for shading");
 for(unsigned i=0;i<3;++i)textures[i].State(i?D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
  D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,i==2);
 Require(acquired.ReadAtShade(owner.data(),states)&&states[0]==0xc0&&states[1]==0x40&&states[2]==0x40,
  "exact read states/external tracker not retained");
 ComPtr<ID3D12Resource> output;
 D3D12NativeGITextures::OutputStatus diagnostic;
 textures[3].native[0x38]=0x20;
 Require(!acquired.AcquireOutputAtShade(textures[3].container.data(),d.device.Get(),output,&diagnostic)&&
   !output&&diagnostic.nativeFlags==0x20&&std::strcmp(diagnostic.stage,"container")==0,"unsupported output reason lost");
 textures[3].native[0x38]=0;
 textures[3].State(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
 Require(!acquired.AcquireOutputAtShade(textures[3].container.data(),d.device.Get(),output,&diagnostic)&&
   !output&&diagnostic.state==0x40&&std::strcmp(diagnostic.stage,"tracked_state")==0,"output state rejection lost");
 textures[3].State(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 Require(!acquired.AcquireOutputAtShade(bundle[0],d.device.Get(),output)&&!output,"hit admitted as raw output");
 Require(acquired.AcquireOutputAtShade(textures[3].container.data(),d.device.Get(),output,&diagnostic)&&diagnostic.width==18&&diagnostic.height==3&&diagnostic.mips==5&&std::strcmp(diagnostic.stage,"ready")==0&&output==textures[3].resource,
  "raw output acquisition failed");
 output.Reset();
 auto* multi=textures[3].resource.Get();
 std::memcpy(textures[0].native.data()+0x88,&multi,8);
 for(unsigned i=0;i<3;++i)textures[i].State(D3D12_RESOURCE_STATE_UNORDERED_ACCESS,i==2);
 D3D12NativeGITextures multiHit;
 Require(!multiHit.AcquireAtRay(owner.data(),d.device.Get(),18,3),"multi-mip hit admitted");
 auto* original=textures[0].resource.Get();std::memcpy(textures[0].native.data()+0x88,&original,8);
 for(unsigned i=0;i<3;++i)textures[i].State(i?D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
  D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,i==2);
 auto* changed=textures[3].resource.Get();std::memcpy(textures[1].native.data()+0x88,&changed,8);
 Require(!acquired.ReadAtShade(owner.data(),states)&&states[0]==0,"changed resource published partial shade states");
 changed=textures[1].resource.Get();std::memcpy(textures[1].native.data()+0x88,&changed,8);
 for(auto& t:textures)t.State(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 for(unsigned scenario=0;scenario<5;++scenario){
  D3D12NativeGITextures rejected;
  if(scenario==0)textures[2].native[0x68]=1;
  if(scenario==1)textures[2].native[0x38]=0x20;
  if(scenario==2)bundle[2]=bundle[0];
  if(scenario==3)textures[2].State(D3D12_RESOURCE_STATE_COMMON);
  Require(!rejected.AcquireAtRay(owner.data(),d.device.Get(),scenario==4?20:18,3)&&!rejected.Hits()[0],
   "invalid native inputs admitted or partial owner published");
  textures[2].native[0x68]=textures[2].native[0x38]=0;bundle[2]=textures[2].container.data();
  textures[2].State(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 }
 D3D12NativeGITextures unreadable;
 Require(!unreadable.AcquireAtRay(reinterpret_cast<void*>(1),d.device.Get(),18,3),"unreadable owner accepted");
 std::cout<<"PASS native GI textures: original containers, retained resources, exact tracked states, changed wrappers, aliases and transactional rejection\n";
}
