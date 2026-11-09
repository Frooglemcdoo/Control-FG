#pragma once
#include "../../src/rr_reflection_backend.h"
#include "../../src/rr_reflection_present.h"
#include <cassert>
#include <map>
#include <vector>
#include <iostream>
using namespace control_rr_reflection;
struct Api {
 Context current{1,90,91,92,93};
 std::map<Address,unsigned> refs;
 std::vector<Transition> barriers;
 std::vector<std::pair<Address,Address>> copies;
 std::uint64_t sizes[2]{1024,4096},completed=0,signal=0;
 unsigned allocations=0,creates=0,createFail=0,holds=0,holdFail=0,commands=0,commandFail=0,releases=0;
 bool signalFail=false;
 bool QueryBudget(Address,control_rr::MemoryBudget& out){out={8ull<<30,2ull<<30};return true;}
 bool AllocationSize(Address,const Shape& s,std::uint64_t& size){++allocations;size=sizes[s.format==10];return true;}
 bool Create(Address,const Shape&,Address& result){++creates;if(creates==createFail)return false;result=0x10000+creates*0x1000;refs[result]=1;return true;}
 void Release(Address r){assert(refs[r]);--refs[r];++releases;}
 bool Hold(Address r){++holds;++refs[r];return holds!=holdFail;}
 bool Current(const Context& c){return Same(c,current);}
 bool Barriers(const Context&,const Transition* t,std::size_t n){++commands;if(commands==commandFail)return false;barriers.insert(barriers.end(),t,t+n);return true;}
 bool Copy(const Context&,Address d,Address s){++commands;if(commands==commandFail)return false;copies.emplace_back(d,s);return true;}
 bool Signal(Address,Address,std::uint64_t v){if(signalFail)return false;signal=v;return true;}
 std::uint64_t Completed(Address){return completed;}
};
static Shape testMaterialShape{2560,1440,57,2,1,1,0,0,3},testPositionShape{2560,1440,10,2,1,1,0,0,3};
static Snapshot snapshot(const Api& a){Snapshot s{};s.context=a.current;s.material.resource=0x6000;s.position.resource=0x7000;
 s.material.shape=testMaterialShape;s.position.shape=testPositionShape;s.material.state=0xc0;s.position.state=0x40;return s;}

