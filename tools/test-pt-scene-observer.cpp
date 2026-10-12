#define NOMINMAX
#include <windows.h>
#include "../src/pt_scene_observer.h"
#include "../src/pt_glass_vtable_call.h"
#include <cassert>
#include <cstdio>
#include <vector>
static std::vector<PTSceneObserver::Event> rows;
static void Sink(const PTSceneObserver::Event& e) noexcept {rows.push_back(e);}
static ID3D12GraphicsCommandList4* seenList=nullptr;
static const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* seenDesc=nullptr;
static const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* seenInfo=nullptr;
static UINT seenCount=0;
static void STDMETHODCALLTYPE Native(ID3D12GraphicsCommandList4* l,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* d,UINT n,const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* p){seenList=l;seenDesc=d;seenCount=n;seenInfo=p;}
int main(){
    D3D12_RAYTRACING_GEOMETRY_DESC geometry[2]{};
    auto& g=geometry[0];g.Type=D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;g.Flags=D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    g.Triangles.IndexBuffer=0x100000112;g.Triangles.VertexBuffer={0x200000024,16};g.Triangles.IndexFormat=DXGI_FORMAT_R16_UINT;
    g.Triangles.VertexFormat=DXGI_FORMAT_R32G32B32_FLOAT;g.Triangles.IndexCount=123;g.Triangles.VertexCount=52;g.Triangles.Transform3x4=0x300000000;
    geometry[1].Type=D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC d{};d.DestAccelerationStructureData=0x400000000;d.SourceAccelerationStructureData=0x500000000;
    d.Inputs.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;d.Inputs.DescsLayout=D3D12_ELEMENTS_LAYOUT_ARRAY;d.Inputs.NumDescs=2;d.Inputs.pGeometryDescs=geometry;
    d.Inputs.Flags=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    const auto original=d;const auto originalGeometry=g;
    PTSceneObserver::Observe(nullptr,&d,0x33593,7,&Sink);assert(rows.size()==3);
    const auto e=rows[1];assert(e.kind==3&&e.sequence==7&&e.site==0x33593&&e.ordinal==0&&e.count==2);
    assert(e.index==g.Triangles.IndexBuffer&&e.position==g.Triangles.VertexBuffer.StartAddress&&e.stride==16&&e.transform==g.Triangles.Transform3x4);
    assert(e.dest==d.DestAccelerationStructureData&&e.source==d.SourceAccelerationStructureData&&e.indexCount==123&&e.vertexCount==52);
    assert(rows[2].geometryType==1&&rows[2].position==0);assert(!memcmp(&d,&original,sizeof(d))&&!memcmp(&g,&originalGeometry,sizeof(g)));
    const D3D12_RAYTRACING_GEOMETRY_DESC* pointers[]={&geometry[0],&geometry[1]};d.Inputs.DescsLayout=D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS;d.Inputs.ppGeometryDescs=pointers;
    rows.clear();PTSceneObserver::Observe(nullptr,&d,0x337cc,8,&Sink);assert(rows.size()==3&&rows[1].position==e.position&&rows[1].site==0x337cc);
    pointers[1]=nullptr;rows.clear();PTSceneObserver::Observe(nullptr,&d,0,9,&Sink);assert(rows.size()==3&&rows.back().kind==4);
    d.Inputs.NumDescs=4097;rows.clear();PTSceneObserver::Observe(nullptr,&d,0,10,&Sink);assert(rows.size()==2&&rows.back().kind==4);
    rows.clear();PTSceneObserver::Observe(nullptr,reinterpret_cast<decltype(&d)>(1),0,11,&Sink);assert(rows.size()==1&&rows[0].kind==4);
    d.Inputs.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;d.Inputs.InstanceDescs=0x600000000;
    rows.clear();PTSceneObserver::Observe(nullptr,&d,0,12,&Sink);assert(rows.size()==1&&rows[0].kind==2&&rows[0].index==0x600000000);
    void* vtable[77]{};vtable[72]=reinterpret_cast<void*>(&Native);struct Fake{void** vtable;} fake{vtable};auto* list=reinterpret_cast<ID3D12GraphicsCommandList4*>(&fake);
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC info{};PTSceneObserver::Forward(list,&d,17,&info);
    assert(seenList==list&&seenDesc==&d&&seenCount==17&&seenInfo==&info);
    const unsigned char instruction[]={0xff,0x90,0x40,0x02,0,0};unsigned char replacement[6]{};
    for(const auto site:{0x180033593ull,0x1800337ccull}){assert(PTGlass::EncodeVtableCall6(instruction,site,72,site+0x1000,replacement));assert(!PTGlass::EncodeVtableCall6(instruction,site,73,site+0x1000,replacement));}
    puts("PASS M6: array and pointer-array triangles, full-width addresses, transforms, AABB preservation, bad pointers, cap, TLAS, input immutability, exact native forwarding, both call encodings.");
}
