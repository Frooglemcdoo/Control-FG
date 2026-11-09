#include <windows.h>
#include "../src/gi_reuse/d3d12_geometry_metadata.h"
#include <cassert>
#include <cstdio>
using namespace gi_reuse;
int main(){
 D3D12_RAYTRACING_GEOMETRY_DESC desc[2]{};
 for(auto& g:desc){g.Type=D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;g.Flags=D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
  auto& t=g.Triangles;t.VertexBuffer={4096,12};t.VertexCount=8;t.VertexFormat=DXGI_FORMAT_R32G32B32_FLOAT;
  t.IndexBuffer=8192;t.IndexCount=6;t.IndexFormat=DXGI_FORMAT_R16_UINT;}
 desc[1].Triangles.VertexBuffer.StartAddress=16384;
 D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS in{};in.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
 in.DescsLayout=D3D12_ELEMENTS_LAYOUT_ARRAY;in.NumDescs=2;in.pGeometryDescs=desc;
 const auto array=CopyBLASGeometry(in);assert(array&&array->At(1)->vertices==16384);
 const D3D12_RAYTRACING_GEOMETRY_DESC* pointers[]={desc+1,desc};
 in.DescsLayout=D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS;in.ppGeometryDescs=pointers;
 const auto indirect=CopyBLASGeometry(in);assert(indirect&&indirect->At(0)->vertices==16384&&indirect->At(1)->vertices==4096);
 desc[1].Triangles.VertexBuffer.StartAddress=32768;assert(indirect->At(0)->vertices==16384);
 pointers[1]=nullptr;assert(!CopyBLASGeometry(in));pointers[1]=desc;
 desc[0].Type=D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;assert(!CopyBLASGeometry(in));
 desc[0].Type=D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
 in.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;assert(!CopyBLASGeometry(in));
 in.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;in.ppGeometryDescs=nullptr;assert(!CopyBLASGeometry(in));
 in.DescsLayout=D3D12_ELEMENTS_LAYOUT_ARRAY;in.pGeometryDescs=nullptr;assert(!CopyBLASGeometry(in));
 std::puts("PASS native D3D12 array/pointer geometry adapters, immutable ownership and unsupported/null rejection");
}
