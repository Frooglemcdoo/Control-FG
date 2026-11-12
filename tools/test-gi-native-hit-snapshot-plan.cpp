#include "../src/gi_reuse/native_hit_snapshot_plan.h"
#include "../src/gi_reuse/native_gi_table.h"
#include <cassert>
#include <iostream>
int main(){
 using Plan=gi_reuse::NativeHitSnapshotPlan;using U64=std::uint64_t;
 PTNativeBinding::Binding b{};b.tag=7;b.type=1;b.indices=0x1000;b.attributes=0x2180;
 b.constants[0]=2;b.constants[1]=2;b.constants[8]=10;b.constants[9]=70;
 for(unsigned i=2;i<8;++i)b.constants[i]=i*3;
 auto second=b;second.attributes=0x2280;second.constants[9]=4;
 std::vector<PTNativeBinding::Binding> bindings{b,second};
 const std::vector<Plan::Allocation> allocations{{2,0x2000,1024},{1,0x1000,64}};
 Plan plan;assert(plan.Build(bindings,allocations,1024));
 assert(plan.Copies().size()==2&&plan.Bytes()==768);
 assert(plan.Copies()[0].bytes==10&&plan.Copies()[1].offset==256&&plan.Copies()[1].bytes==388);
 std::vector<PTNativeBinding::Binding> remapped;
 assert(plan.Rebase(0x10000,plan.Bytes(),remapped));
 assert(remapped[0].indices==0x10000&&remapped[0].attributes==0x10180&&remapped[1].attributes==0x10280);
 // Simulated copy followed by native mutation verifies address translation at
 // both ends of every declared view, shared-source deduplication and R16 tail.
 std::vector<unsigned char> index(64),attributes(1024),snapshot(static_cast<std::size_t>(plan.Bytes()),0);
 for(std::size_t i=0;i<index.size();++i)index[i]=static_cast<unsigned char>(i+13);
 for(std::size_t i=0;i<attributes.size();++i)attributes[i]=static_cast<unsigned char>(i*7+5);
 for(const auto& c:plan.Copies()){
  const auto& input=c.source.token==1?index:attributes;
  std::memcpy(snapshot.data()+c.destinationOffset,input.data()+c.offset,static_cast<std::size_t>(c.bytes));
 }
 const auto originalIndex=index,originalAttributes=attributes;
 std::fill(index.begin(),index.end(),0);std::fill(attributes.begin(),attributes.end(),0);
 for(std::size_t i=0;i<bindings.size();++i){
  assert(!std::memcmp(snapshot.data()+remapped[i].indices-0x10000,originalIndex.data(),bindings[i].constants[8]));
  assert(!std::memcmp(snapshot.data()+remapped[i].attributes-0x10000,originalAttributes.data()+bindings[i].attributes-0x2000,bindings[i].constants[9]));
  auto restored=remapped[i];restored.indices=bindings[i].indices;restored.attributes=bindings[i].attributes;
  assert(!std::memcmp(&restored,&bindings[i],sizeof(restored)));
 }
 PTShaderTableAdapter::Identifier opaque{},alpha{};opaque[0]=1;alpha[0]=2;
 gi_reuse::NativeGITable table;assert(gi_reuse::BuildNativeGITable(1,2,1,remapped,opaque,alpha,table));
 assert(table.active==std::vector<unsigned char>({1,0,1,0}));
 for(std::size_t row=0;row<4;++row)if(row%2)for(std::size_t j=0;j<96;++j)assert(table.bytes[row*96+j]==0);
 auto reject=[&](std::vector<PTNativeBinding::Binding> bs,std::vector<Plan::Allocation> as,U64 budget=1024){
  assert(!plan.Build(bs,as,budget)&&!plan.Bytes()&&plan.Copies().empty());
 };
 reject(bindings,allocations,767);
 auto invalid=allocations;invalid.push_back({3,0x2100,512});reject(bindings,invalid);
 invalid=allocations;invalid[1].token=2;reject(bindings,invalid);
 invalid=allocations;invalid[0].bytes=512;reject(bindings,invalid);
 invalid=allocations;invalid[0].address=(std::numeric_limits<U64>::max)()-255;invalid[0].bytes=512;reject(bindings,invalid);
 auto bad=bindings;bad[0].constants[8]=1;reject(bad,allocations);
 bad=bindings;bad[0].constants[9]=0;reject(bad,allocations);
 bad=bindings;bad[0].constants[1]=0;reject(bad,allocations);
 bad=bindings;bad[0].type=2;reject(bad,allocations);
 assert(plan.Build(bindings,allocations,1024));
 assert(!plan.Rebase(0x2000,1024,remapped)&&remapped.empty());
 assert(!plan.Rebase(0x10001,1024,remapped));assert(!plan.Rebase(0x10000,767,remapped));
 assert(!plan.Rebase((std::numeric_limits<U64>::max)()-255,1024,remapped));
 std::cout<<"PASS native hit snapshot planning: immutable copy simulation, shared ranges, native rows/constants, bounds/alias/budget rejection. No GPU/image claim.\n";
}
