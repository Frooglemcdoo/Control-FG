#include "capture.h"
#include <iostream>
int main(int argc,char** argv){try {
 if(argc!=2)throw std::runtime_error("usage: replay-cpu capture.bin");
 const auto frames=rt_capture::Read(argv[1]);std::uint32_t capacity=0;
 for(const auto& f:frames)capacity=(std::max)(capacity,static_cast<std::uint32_t>(f.instances.size()));
 rt_ptlas::Planner p(capacity,1,true);std::vector<rt_ptlas::Instance> mirror(capacity);
 std::size_t writes=0,blocked=0;
 for(const auto& f:frames){
  std::vector<std::uint32_t> parts(f.instances.size());auto plan=p.Prepare(f.instances,parts);
  bool nonfinite=false;for(const auto& v:f.instances)for(float x:v.transform)nonfinite|=!std::isfinite(x);blocked+=nonfinite;
  for(const auto& w:plan.writes){auto& v=mirror.at(w.index);v.transform=w.transform;v.idMask=w.id|(w.mask<<24);v.contributionFlags=w.contribution|(w.flags<<24);v.blas=w.blas;}
  for(std::size_t i=0;i<capacity;++i){rt_ptlas::Instance expected{};expected.transform[0]=expected.transform[5]=expected.transform[10]=1;if(i<f.instances.size())expected=f.instances[i];
   if(std::memcmp(&expected,&mirror[i],sizeof(expected)))throw std::runtime_error("replay byte mismatch");}
  writes+=plan.writes.size();p.Commit(std::move(plan));if(!p.Prepare(f.instances,parts).writes.empty())throw std::runtime_error("unchanged replay writes");
 }
 std::cout<<"PASS byte-exact CPU replay snapshots="<<frames.size()<<" capacity="<<capacity<<" writes="<<writes<<" strict_nonfinite_frames="<<blocked<<" GPU_validation=0 native_retirement=0\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}}
