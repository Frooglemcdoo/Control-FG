#pragma once
struct ReplayRay {float origin[3];float tmin;float direction[3];float tmax;};
static_assert(sizeof(ReplayRay)==32);
static std::vector<ReplayRay> ReplayRays(const std::vector<Instance>& scene){
 std::vector<ReplayRay> rays;const std::size_t stride=(std::max)(std::size_t(1),scene.size()/128);
 for(std::size_t i=0;i<scene.size()&&rays.size()<128;i+=stride){const auto& v=scene[i];
  bool finite=true;for(float f:v.transform)finite&=std::isfinite(f);if(!finite||!v.blas||!(v.idMask>>24))continue;
  // A local triangle's center is (0,-0.25,0). Cross transformed local X/Y
  // axes to aim a short ray at its plane even under scale/shear/reflection.
  const auto& m=v.transform;float n[3]={m[4]*m[9]-m[8]*m[5],m[8]*m[1]-m[0]*m[9],m[0]*m[5]-m[4]*m[1]};
  const float length=std::sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);if(!std::isfinite(length)||length<1e-10f)continue;
  ReplayRay ray{};ray.tmin=0.0001f;ray.tmax=4;
  for(unsigned axis=0;axis<3;++axis){ray.direction[axis]=n[axis]/length;ray.origin[axis]=m[axis*4+3]-0.25f*m[axis*4+1]-2*ray.direction[axis];}rays.push_back(ray);
 }
 if(rays.empty())throw std::runtime_error("no finite replay ray targets");
 while(rays.size()<128)rays.push_back(rays.front());
 return rays;
}
static void ReplayCapture(GPU& g,const char* path,ID3D12Resource* triangle){
 auto snapshots=rt_capture::Read(path);std::uint32_t capacity=0;std::size_t nanInstances=0;
 std::vector<std::vector<ReplayRay>> rays;
 for(auto& frame:snapshots){capacity=(std::max)(capacity,static_cast<std::uint32_t>(frame.instances.size()));
  for(auto& v:frame.instances){bool hasNaN=false;for(float f:v.transform){if(std::isinf(f))throw std::runtime_error("infinite captured transform");hasNaN|=std::isnan(f);}nanInstances+=hasNaN;
   // Captured GPU addresses are never dereferenced. BLAS geometry is a local
   // owned test triangle; this validates descriptor semantics, not game rays.
   if(v.blas)v.blas=triangle->GetGPUVirtualAddress();}
  rays.push_back(ReplayRays(frame.instances));
 }
 Probe probe(g,128,"replay.dxil");std::vector<Hits> expected;
 std::size_t nativeHits=0;
 for(std::size_t i=0;i<snapshots.size();++i){probe.rays=Upload(g,rays[i].data(),rays[i].size()*sizeof(ReplayRay));auto hits=Native(g,probe,snapshots[i].instances);
  for(const auto& hit:hits)nativeHits+=hit[0]!=0xffffffffu;expected.push_back(std::move(hits));}
 if(nativeHits<128)throw std::runtime_error("insufficient native replay hits");
 NvBackend replacement(g.device.Get(),capacity,1);Planner planner(capacity,1,true);
 std::size_t checked=0,equivalentTies=0;
 for(std::size_t i=0;i<snapshots.size();++i){const auto& scene=snapshots[i].instances;
  std::vector<std::uint32_t> partitions(scene.size());auto plan=planner.Prepare(scene,partitions);
  probe.rays=Upload(g,rays[i].data(),rays[i].size()*sizeof(ReplayRay));auto keepAlive=replacement.Record(g.list.Get(),plan);
  const auto actual=probe.Trace(g,replacement.Address());replacement.Complete();planner.Commit(std::move(plan));
  for(std::size_t r=0;r<actual.size();++r){const auto& a=actual[r];const auto& e=expected[i][r];
   float at=0,et=0;std::memcpy(&at,&a[3],4);std::memcpy(&et,&e[3],4);
   bool same=a[0]==e[0]&&a[2]==e[2]&&std::isfinite(at)&&std::isfinite(et)&&std::abs(at-et)<=0.002f;
   if(a[1]!=e[1]){
    // Exact coincident fixture triangles may legally choose either instance.
    const bool tie=a[1]<scene.size()&&e[1]<scene.size()&&std::memcmp(scene[a[1]].transform.data(),scene[e[1]].transform.data(),48)==0&&scene[a[1]].idMask==scene[e[1]].idMask;
    same&=tie;if(tie)++equivalentTies;
   }
   if(!same){std::cerr<<"replay mismatch snapshot="<<i<<" ray="<<r<<" native_index="<<e[1]<<" ptlas_index="<<a[1]<<" native_t="<<et<<" ptlas_t="<<at<<"\n";throw std::runtime_error("captured descriptor parity failed");}++checked;
  }
  std::cout<<"PASS captured snapshot="<<i<<" instances="<<scene.size()<<" rays="<<actual.size()<<"\n";
 }
 std::cout<<"PASS capture replay snapshots="<<snapshots.size()<<" rays="<<checked<<" native_hits="<<nativeHits<<" nan_instances="<<nanInstances<<" exact_coincident_ties="<<equivalentTies<<" geometry_substituted=1 native_tlas_builds_during_replay_ptlas=0 native_retirement=0 performance_claim=0\n";
}
