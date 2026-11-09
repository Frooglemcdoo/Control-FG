#pragma once
#include "../../src/rt_ptlas_plan.h"
#include <fstream>
namespace rt_capture {
struct Snapshot { std::uint64_t tick=0; std::vector<rt_ptlas::Instance> instances; };
inline std::vector<Snapshot> Read(const char* path) {
 std::ifstream f(path,std::ios::binary); if(!f)throw std::runtime_error("capture file missing");
 auto read=[&](void* out,std::size_t size){if(!f.read(static_cast<char*>(out),static_cast<std::streamsize>(size)))throw std::runtime_error("truncated capture");};
 std::uint64_t h[6]{};read(h,sizeof(h));
 if(h[0]!=0x3130333933545243ull||h[1]!=1||h[2]==0||h[2]>48)throw std::runtime_error("capture header");
 std::vector<Snapshot> result;
 for(std::uint64_t i=0;i<h[2];++i){
  std::uint64_t r[6]{};read(r,sizeof(r));
  if(!r[1]||!r[2]||!r[3]||!r[4]||!r[5]||r[5]>2*1024*1024||r[5]%64||(i&&r[0]<result.back().tick))throw std::runtime_error("capture record");
  Snapshot s;s.tick=r[0];s.instances.resize(static_cast<std::size_t>(r[5]/64));read(s.instances.data(),static_cast<std::size_t>(r[5]));result.push_back(std::move(s));
 }
 if(f.peek()!=std::char_traits<char>::eof())throw std::runtime_error("capture trailing bytes");
 return result;
}
}
