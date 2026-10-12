#include "../src/pt_shader_table_adapter.h"
#include <cstdio>
#include <cstdlib>
using namespace PTShaderTableAdapter;
void check(bool x,const char* n){if(!x){std::printf("FAIL: %s\n",n);std::exit(1);}}
int main(){
    Exports ids{};ids.nativeOpaque[0]=1;ids.nativeAlpha[0]=2;ids.privateOpaque[0]=3;ids.privateAlpha[0]=4;
    PTSceneAdmission::Geometry g{};g.indices={{4096,24,1},4096};g.positions={{8192,36,1},8192};g.attributes={{12288,48,1},12288};
    g.indexStride=2;g.indexCount=3;g.vertexCount=3;g.positionStride=12;g.attributeStride=16;g.normalOffset=4;g.uvOffset=12;g.materialCount=1;g.epoch=1;
    g.float3Positions=g.sharedVertexIndexing=g.noGeometryTransform=true;
    PTSceneAdmission::Record record;check(PTSceneAdmission::Admit(g,record)==PTSceneAdmission::Error::none,"fixture admission");
    unsigned char native[NativeStride]{};std::memcpy(native,ids.nativeOpaque.data(),32);std::memcpy(native+32,&record.indices,8);std::memcpy(native+40,&record.attributes,8);std::memcpy(native+48,record.constants,40);
    Bytes output;check(Convert(native,96,ids,&g,output)==Status::ready,"opaque conversion");
    check(output[0]==3&&!std::memcmp(output.data()+48,&record.positions,8)&&!std::memcmp(output.data()+56,record.constants,48),"private ABI bytes");
    native[0]=2;check(Convert(native,96,ids,&g,output)==Status::ready&&output[0]==4,"alpha classification");
    check(Convert(native,96,ids,nullptr,output)==Status::unverified_mapping&&output==Bytes{},"missing mapping rejected, no partially published record");
    native[0]=9;check(Convert(native,96,ids,&g,output)==Status::unknown_export,"foreign state object export");native[0]=1;
    native[48+3*4]=1;check(Convert(native,96,ids,&g,output)==Status::native_mismatch,"material mismatch");native[48+3*4]=0;
    check(Convert(native,88,ids,&g,output)==Status::record_size,"truncated source");
    unsigned char table[192]{};std::memcpy(table,native,96);std::memcpy(table+96,native,96);
    std::vector<Binding> bindings={{&ids,&g},{&ids,&g}};std::vector<Bytes> published;std::size_t failed=0;
    check(ConvertTable(table,192,bindings,published,failed)==Status::ready&&published.size()==2,"complete table publishes");
    bindings[1].geometry=nullptr;
    check(ConvertTable(table,192,bindings,published,failed)==Status::unverified_mapping&&published.empty()&&failed==1,"later failure publishes no partial table");
    Exports empty{};check(Convert(native,96,empty,&g,output)==Status::invalid_exports,"missing identifiers");
    g.positions.allocation.epoch=2;check(Convert(native,96,ids,&g,output)==Status::invalid_geometry,"stale position mapping");
    std::puts("PASS: native SBT to private SBT conversion and fail-closed publication");
}
