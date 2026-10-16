#include "../src/pt_material_history.h"
#include <cassert>
#include <cstdio>
using Record=std::array<unsigned char,128>;
int main(){
    Record a{},b{};unsigned material=7;std::memcpy(a.data()+68,&material,4);a[32]=4;a[40]=8;a[48]=12;
    b=a;b[32]=5;std::vector<Record> rows{a,b,a};auto first=PTMaterialHistory::GeometryKeys(rows);
    material=91;for(auto& r:rows){std::memcpy(r.data()+68,&material,4);r[0]=42;}
    std::reverse(rows.begin(),rows.end());auto moved=PTMaterialHistory::GeometryKeys(rows);
    assert(first[7]&&first[7]==moved[91]&&!moved[7]);
    rows.pop_back();assert(PTMaterialHistory::GeometryKeys(rows)[91]==first[7]);
    rows[0][48]++;assert(PTMaterialHistory::GeometryKeys(rows)[91]!=first[7]);
    rows={a};assert(PTMaterialHistory::GeometryKeys(rows)[7]!=first[7]);
    b=a;material=8;std::memcpy(b.data()+68,&material,4);rows={a,b};
    auto ambiguous=PTMaterialHistory::GeometryKeys(rows);assert(!ambiguous[7]&&!ambiguous[8]);rows={a};
    material=PTMaterialHistory::Count;std::memcpy(rows[0].data()+68,&material,4);
    assert(PTMaterialHistory::GeometryKeys(rows)==PTMaterialHistory::Keys{});
    puts("PASS: material geometry fingerprints survive slot renumbering, export changes, duplicate rays and row reorder; changed bindings/membership and out-of-range slots rejected");
}
