#include "../src/pt_scene_admission.h"
#include <cstdio>
#include <cstdlib>
using namespace PTSceneAdmission;
void check(bool pass,const char* name){if(!pass){std::printf("FAIL: %s\n",name);std::exit(1);}}
int main(){
    Geometry g{};g.indices={{0x1000,1000000,1},0x1000+357318};
    g.positions={{0x200000,2000000,1},0x200000+368896};g.attributes={{0x500000,2000000,1},0x500000};
    // One captured dynamic BLAS layout: R16, 3840 indices, 6183 float3 positions.
    // Attribute identity below is synthetic; no native SBT match is asserted.
    g.indexStride=2;g.indexCount=3840;g.vertexCount=6183;g.positionStride=g.attributeStride=12;
    g.normalOffset=0;g.uvOffset=8;g.material=2;g.materialCount=3;g.epoch=1;
    g.float3Positions=g.sharedVertexIndexing=g.noGeometryTransform=true;Record r;
    check(Admit(g,r)==Error::none&&r.constants[0]==357318&&r.positions==g.positions.address,"native-style offset at 2-byte alignment");
    auto bad=g;bad.positions.allocation.epoch=2;check(Admit(bad,r)==Error::epoch&&r.positions==0,"stale generation fails closed");
    bad=g;bad.positions.allocation.bytes=368896+6183*12-1;check(Admit(bad,r)==Error::allocation,"position overrun");
    bad=g;bad.indices.allocation.bytes=357318+3840*2;check(Admit(bad,r)==Error::range,"unaligned tail needs backing word");
    bad=g;bad.sharedVertexIndexing=false;check(Admit(bad,r)==Error::format,"unproven index mapping");
    bad=g;bad.noGeometryTransform=false;check(Admit(bad,r)==Error::format,"unhandled BLAS transform");
    bad=g;bad.indices.allocation.address=UINT64_MAX-10;check(Admit(bad,r)!=Error::none,"GPU address overflow");
    bad=g;bad.material=3;check(Admit(bad,r)==Error::material,"material range");
    bad=g;bad.uvOffset=10;check(Admit(bad,r)==Error::alignment,"attribute alignment");
    bad=g;bad.vertexCount=UINT32_MAX;check(Admit(bad,r)!=Error::none,"wide span overflow");
    std::puts("PASS: native-style geometry admission, bounds, identity and epoch rejection");
}
