#include "../src/gi_reuse/visibility_table.h"
#include <iostream>
#include <stdexcept>
using namespace gi_reuse;
static void Require(bool b,const char* why) {if(!b)throw std::runtime_error(why);}
int main() try {
    std::vector<VisibilityIdentifierPair> ids(2);
    ids[0].native.fill(1);ids[0].replacement.fill(11);
    ids[1].native.fill(2);ids[1].replacement.fill(12);
    // Native GI-style spacing: even rows active, odd rows retained and never compacted.
    std::vector<unsigned char> native(4*96);
    for(std::size_t i=0;i<native.size();++i)native[i]=static_cast<unsigned char>(i);
    std::memcpy(native.data(),ids[0].native.data(),32);
    std::memcpy(native.data()+192,ids[1].native.data(),32);
    const auto original=native;std::vector<unsigned char> output{99};
    Require(RemapVisibilityTable(native.data(),native.size(),96,{1,0,1,0},ids,output),"Valid table rejected");
    for(std::size_t i=0;i<native.size();++i) {
        const std::size_t row=i/96,offset=i%96;
        const unsigned char expected=(row%2==0&&offset<32)?ids[row/2].replacement[offset]:native[i];
        Require(output[i]==expected,"Local arguments, padding, inactive row or identity corrupted");
    }
    Require(native==original,"Native table mutated");const auto good=output;
    auto reject=[&](const unsigned char* source,std::size_t bytes,unsigned stride,const std::vector<unsigned char>& active,const std::vector<VisibilityIdentifierPair>& map) {
        Require(!RemapVisibilityTable(source,bytes,stride,active,map,output),"Invalid table admitted");
        Require(output==good,"Failed remap published partial table");
    };
    reject(nullptr,native.size(),96,{1,0,1,0},ids);
    reject(native.data(),native.size()-1,96,{1,0,1,0},ids);
    reject(native.data(),native.size(),48,{1,0,1,0},ids);
    reject(native.data(),native.size(),96,{1,1,1,0},ids);
    reject(native.data(),native.size(),96,{1,0,1},ids);
    reject(native.data(),native.size(),96,{0,0,0,0},ids);
    reject(native.data(),native.size(),96,{2,0,1,0},ids);
    auto bad=ids;bad[1].native=bad[0].native;reject(native.data(),native.size(),96,{1,0,1,0},bad);
    bad=ids;bad[1].replacement=bad[0].replacement;reject(native.data(),native.size(),96,{1,0,1,0},bad);
    bad=ids;bad[0].replacement.fill(0);reject(native.data(),native.size(),96,{1,0,1,0},bad);
    output=native;
    Require(RemapVisibilityTable(output.data(),output.size(),96,{1,0,1,0},ids,output),"Aliased source failed");
    Require(output==good,"Aliased source corrupted");
    std::cout<<"PASS visibility table: opaque/alpha IDs, native local arguments, inactive rows, transactional rejection and aliased source\n";return 0;
} catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
