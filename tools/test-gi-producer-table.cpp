#include "../src/gi_reuse/producer_table.h"
#include "../src/gi_reuse/producer_backing_ranges.h"
#include "../src/gi_reuse/producer_ray.h"
#include <random>
#include <iostream>
#include <stdexcept>
using namespace gi_reuse;
static void Require(bool b){if(!b)throw std::runtime_error("producer table invariant");}
int main()try{
 // Compare the bulk prefix index with exhaustive containment, including
 // equal ends, nested aliases, endpoints, missing ranges and integer overflow.
 std::mt19937 rng(27);
 for(unsigned batch=0;batch<64;++batch){
  ProducerBackingRanges index;std::vector<std::pair<std::uint64_t,std::uint64_t>> ranges;
  for(unsigned i=0;i<128;++i){std::uint64_t begin=1+rng()%4096,bytes=1+rng()%512;
   Require(index.Add(begin,bytes));ranges.emplace_back(begin,bytes);
  }
  Require(!index.Unique(1,1));Require(!index.Add(UINT64_MAX,1));index.Seal();Require(!index.Add(1,1));
  for(unsigned i=0;i<4096;++i){std::uint64_t begin=1+rng()%4608,bytes=1+rng()%512;
   unsigned count=0;for(auto r:ranges)if(begin>=r.first&&begin-r.first<=r.second&&bytes<=r.second-(begin-r.first))++count;
   Require(index.Unique(begin,bytes)==(count==1));
  }
  Require(!index.Unique(0,1)&&!index.Unique(1,0)&&!index.Unique(UINT64_MAX,1));
 }
 ProducerBackingRanges edge;Require(edge.Add(100,100)&&edge.Add(120,80));edge.Seal();
 Require(edge.Unique(100,20)&&!edge.Unique(120,80)&&!edge.Unique(200,1));
 for(unsigned width:{2u,4u}){
  PTShaderTableAdapter::Exports ids{};
  ids.nativeOpaque.fill(1);ids.nativeAlpha.fill(2);ids.privateOpaque.fill(3);ids.privateAlpha.fill(4);
  PTSceneAdmission::Geometry g{};
  g.indices={{4096,width==2?8u:12u,11},4096};g.positions={{8192,44,11},8192};g.attributes={{12288,48,11},12288};
  g.indexStride=width;g.indexCount=3;g.vertexCount=3;g.positionStride=16;g.attributeStride=16;g.normalOffset=4;g.uvOffset=12;
  g.material=1;g.materialCount=3;g.epoch=11;g.float3Positions=g.sharedVertexIndexing=g.noGeometryTransform=true;
  auto other=g;other.material=2;std::vector<PTSceneAdmission::Geometry> maps{g,other};
  std::vector<PTNativeBinding::Binding> bindings(2);
  for(unsigned i=0;i<2;++i){PTSceneAdmission::Record r;
   Require(PTSceneAdmission::Admit(maps[i],r)==PTSceneAdmission::Error::none);
   bindings[i].type=i;bindings[i].indices=r.indices;bindings[i].attributes=r.attributes;
   std::memcpy(bindings[i].constants,r.constants,40);
  }
  NativeGITable native;Require(BuildNativeGITable(7,8,1,bindings,ids.nativeOpaque,ids.nativeAlpha,native));
  // Reserved records are deliberately poisoned: they are not live native records.
  for(unsigned row:{1u,3u})std::memset(native.bytes.data()+row*96,0x9a,96);
  const auto original=native.bytes;GIProducerTable out;std::size_t failed=99;
  Require(PrepareGIProducerTable(native,7,8,9,11,ids,maps,out,&failed)==GIProducerTableError::None&&failed==2);
  Require(out.records.size()==4&&out.records[0][0]==3&&out.records[2][0]==4);
  Require(out.records[1]==PTShaderTableAdapter::Bytes{}&&out.records[3]==PTShaderTableAdapter::Bytes{});
  Require(native.bytes==original&&out.writerVersion==8&&out.epoch==11&&out.producerPipeline==9);
  std::uint64_t position=0;std::uint32_t material=0;
  std::memcpy(&position,out.records[2].data()+48,8);std::memcpy(&material,out.records[2].data()+56+12,4);
  Require(position==8192&&material==2);const auto accepted=out.records;
  auto reject=[&](const NativeGITable& table,const std::vector<PTSceneAdmission::Geometry>& geometry,
                  const PTShaderTableAdapter::Exports& exports,std::uint64_t pipe=7,std::uint64_t version=8){
   Require(PrepareGIProducerTable(table,pipe,version,9,11,exports,geometry,out)!=GIProducerTableError::None);
   Require(out.records==accepted&&out.writerVersion==8); // no partial publication
  };
  reject(native,maps,ids,99);reject(native,maps,ids,7,99);
  auto broken=native;broken.active[1]=1;reject(broken,maps,ids);
  broken=native;broken.bytes.pop_back();reject(broken,maps,ids);
  auto wrong=maps;wrong[1].epoch=12;reject(native,wrong,ids);
  wrong=maps;wrong[1].material=1;reject(native,wrong,ids); // shared buffers don't identify a material
  wrong=maps;wrong[1].positions.allocation.bytes=43;reject(native,wrong,ids);
  auto badIds=ids;badIds.privateAlpha=badIds.privateOpaque;reject(native,maps,badIds);
 }
 std::cout<<"PASS GI producer table: R16/R32, opaque/alpha, shared geometry materials, sparse row preservation and transactional rejection\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
