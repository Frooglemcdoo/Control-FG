#include <windows.h>
#include <d3d12.h>
#include <atomic>
#include <cstdio>
#include <cstdarg>
#include <string>
#include <cstdlib>
std::atomic<unsigned long long> presentCount{42};
std::string logText;
void Trace(const char* fmt, ...) { char b[2048]; va_list a; va_start(a,fmt); vsnprintf(b,sizeof(b),fmt,a); va_end(a); logText+=b; logText+='\n'; }
bool PatchSlot(void** slot,void* expected,void* replacement) {if(*slot!=expected)return false;*slot=replacement;return true;}
#include "../src/gi_geometry_probe.inl"
void Check(bool ok){if(!ok){std::fprintf(stderr,"FAILED\n%s",logText.c_str());std::exit(1);}}
unsigned buildCalls=0,copyCalls=0;
const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* expectedDesc;
const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* expectedInfo;
void STDMETHODCALLTYPE NativeBuild(ID3D12GraphicsCommandList4*,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* d,UINT n,const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* p){Check(d==expectedDesc&&n==1&&p==expectedInfo);++buildCalls;}
void STDMETHODCALLTYPE NativeCopy(ID3D12GraphicsCommandList4*,D3D12_GPU_VIRTUAL_ADDRESS d,D3D12_GPU_VIRTUAL_ADDRESS s,D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE m){Check(d==400&&s==300&&m==D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT);++copyCalls;}
int main(){
 void* table[80]{};void** object=table;auto* list=reinterpret_cast<ID3D12GraphicsCommandList4*>(&object);
 gi_geometry_probe::hooks[0]={table,NativeBuild,NativeCopy};table[72]=reinterpret_cast<void*>(&gi_geometry_probe::BuildHook);table[74]=reinterpret_cast<void*>(&gi_geometry_probe::CopyHook);
 D3D12_RAYTRACING_GEOMETRY_DESC g{};g.Type=D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;g.Triangles.VertexBuffer={1000,12};g.Triangles.VertexCount=3;g.Triangles.VertexFormat=DXGI_FORMAT_R32G32B32_FLOAT;g.Triangles.IndexBuffer=2000;g.Triangles.IndexCount=3;g.Triangles.IndexFormat=DXGI_FORMAT_R16_UINT;g.Triangles.Transform3x4=3000;
 D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC d{};d.Inputs.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;d.Inputs.DescsLayout=D3D12_ELEMENTS_LAYOUT_ARRAY;d.Inputs.NumDescs=1;d.Inputs.pGeometryDescs=&g;
 D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC p{};expectedDesc=&d;expectedInfo=&p;
 const auto before=g;
 list->BuildRaytracingAccelerationStructure(&d,1,&p);Check(buildCalls==1&&memcmp(&before,&g,sizeof(g))==0);Check(logText.find("vertex_va=1000 vertex_stride=12")!=std::string::npos&&logText.find("transform_va=3000")!=std::string::npos);
 const D3D12_RAYTRACING_GEOMETRY_DESC* ptrs[]={&g,nullptr};d.Inputs.DescsLayout=D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS;d.Inputs.NumDescs=2;d.Inputs.ppGeometryDescs=ptrs;
 list->BuildRaytracingAccelerationStructure(&d,1,&p);Check(buildCalls==2&&logText.find("geometry=1")!=std::string::npos);
 d.Inputs.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;d.Inputs.InstanceDescs=999;list->BuildRaytracingAccelerationStructure(&d,1,&p);Check(logText.find("instances_gpu_va=999")!=std::string::npos);
 list->CopyRaytracingAccelerationStructure(400,300,D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT);Check(copyCalls==1);
 gi_geometry_probe::builds=512;list->BuildRaytracingAccelerationStructure(&d,1,&p);Check(buildCalls==4&&logText.find("inventory_incomplete=1")!=std::string::npos);
 std::puts("R279 COM slots, forwarding identity, descriptor preservation, array layouts, TLAS and limits passed");
}
