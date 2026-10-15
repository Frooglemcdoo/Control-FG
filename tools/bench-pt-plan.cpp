#include "../src/pt_scene_dispatch_plan.h"
#include <chrono>
#include <cstdio>
#include <cassert>
int main(){using namespace PTSceneDispatch;PTOwnedScene::Index owners;Scene scene;Table table;ExportSet exports;constexpr unsigned bottoms=1024,rows=bottoms*4;
scene.address=0x80000000;scene.version=1;scene.tlas={1,256};owners.Insert({1,scene.address,256});table.address=0x90000000;table.version=1;table.pipeline=2;table.rays=2;table.bytes.resize(rows*192);exports.nativePipeline=2;exports.privatePipeline=3;exports.rays.resize(2);
for(unsigned ray=0;ray<2;++ray){auto& e=exports.rays[ray];e.nativeOpaque[0]=static_cast<unsigned char>(1+ray);e.nativeAlpha[0]=static_cast<unsigned char>(3+ray);e.privateOpaque[0]=static_cast<unsigned char>(5+ray);e.privateAlpha[0]=static_cast<unsigned char>(7+ray);}
auto backing=std::make_shared<int>(1);
for(unsigned n=0;n<bottoms;++n){const U64 as=0x81000000ull+n*256;owners.Insert({2+n,as,256});scene.blas[as]={2+n,256,1};std::vector<PTSceneTopology::Geometry> geometry;
 for(unsigned g=0;g<4;++g){unsigned i=n*4+g;const U64 ix=0x100000ull+i*64,position=0x200000ull+i*64,attribute=0x300000ull+i*64;
 auto f=std::make_shared<PTSceneTopology::FrozenGeometry>();f->indexBase=ix;f->indexBytes=8;f->positionBase=position;f->positionBytes=36;f->privateIndex=ix+0x10000000;f->privatePosition=position+0x10000000;f->owner=backing;
 geometry.push_back({ix,position,12,2,3,3,10000+i,20000+i,f});owners.Insert({30000+i,attribute,64});
 PTNativeBinding::Binding b{};b.indices=ix;b.attributes=attribute;b.constants[1]=2;b.constants[2]=16;b.constants[6]=8;b.constants[8]=6;b.constants[9]=64;
 table.bindings.push_back(b);table.allocationTokens.push_back({10000+i,30000+i});
 for(unsigned ray=0;ray<2;++ray){auto* p=table.bytes.data()+(i*2+ray)*96;std::memcpy(p,exports.rays[ray].nativeOpaque.data(),32);std::memcpy(p+32,&b.indices,8);std::memcpy(p+40,&b.attributes,8);std::memcpy(p+48,b.constants,40);}}
 assert(scene.topology.Build(as,std::move(geometry)));for(unsigned repeat=0;repeat<4;++repeat){PTSceneTopology::Instance instance{};instance.idMask=0xff000001;instance.blas=as;instance.contributionFlags=n*8;scene.instances.push_back(instance);}}
Selection selected{scene.address,1,table.address,1,2};Plan plan;auto start=std::chrono::steady_clock::now();for(unsigned i=0;i<250;++i){auto result=Prepare(selected,scene,table,owners,exports,plan);assert(result.status==Status::prepared);}
U64 hash=1469598103934665603ull;for(const auto& record:plan.records)for(auto b:record){hash^=b;hash*=1099511628211ull;}
printf("prepare_ms=%.6f records=%zu hash=%llu tokens=%zu versions=%zu\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/250,plan.records.size(),static_cast<unsigned long long>(hash),plan.allocationTokens.size(),plan.bottomVersions.size());}
