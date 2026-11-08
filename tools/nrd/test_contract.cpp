#include "../../src/nrd_diffuse_session.h"
#include "../../src/nrd_diffuse_camera.h"
#include <cstdio>
#include <cstdlib>
static void Require(bool good,const char* what){if(!good){std::fprintf(stderr,"FAIL %s\n",what);std::exit(1);}}
int main(){
 std::array<float,2> jitter{};
 Require(control_nrd::NativeClipShiftToNrdJitter(2*.297851562/2560,-2*.379286706/1440,2560,1440,jitter),"native jitter units");
 Require(std::abs(jitter[0]+.297851562f)<1e-7f&&std::abs(jitter[1]+.379286706f)<1e-7f,"NRD sample offset is inverse raster offset");
 Require(!control_nrd::NativeClipShiftToNrdJitter(1,0,2560,1440,jitter),"invalid jitter rejected");
 Require(control_nrd::NativeClipShiftToNrdJitter(-1.000001/2560,1.000001/1440,2560,1440,jitter)&&jitter[0]==.5f&&jitter[1]==.5f,"jitter endpoint tolerance cannot violate session contract");
 struct Camera {double worldToView[12]{},viewToWorld[12]{},viewToClip[16]{},clipToView[16]{},worldToClip[16]{},clipToWorld[16]{};} camera;
 // Rotated + translated camera: catches transpose and translation placement.
 const double vw[]={0,1,0,-1,0,0,0,0,1,3,5,7};
 const double wv[]={0,-1,0,1,0,0,0,0,1,-5,3,-7};
 std::copy(vw,vw+12,camera.viewToWorld);std::copy(wv,wv+12,camera.worldToView);
 for(unsigned i=0;i<16;++i)camera.viewToClip[i]=camera.clipToView[i]=i%5==0?1:0;
 const auto wc=control_nrd::ExpandAffine(wv),cw=control_nrd::ExpandAffine(vw);
 std::copy(wc.begin(),wc.end(),camera.worldToClip);std::copy(cw.begin(),cw.end(),camera.clipToWorld);
 control_nrd::CameraMatrices converted;
 Require(control_nrd::ConvertCamera(camera,converted),"native camera matrix relations");
 Require(converted.viewToWorld[12]==3&&converted.worldToView[12]==-5&&converted.viewToWorld[1]==1,"camera column-vector convention");
 camera.worldToClip[12]+=1;Require(!control_nrd::ConvertCamera(camera,converted),"mismatched camera snapshot rejected");
 control_nrd::Session session;control_nrd::Frame f;
 f.frame=1;f.view=1;f.width=64;f.height=32;
 f.sameFrameGuides=f.worldNormals=f.linearViewDepth=f.unjitteredMotion=f.radianceContract=f.hitDistanceContract=true;
 auto&s=f.settings;s.timeDeltaBetweenFrames=16.6667f;
 for(unsigned i=0;i<16;++i)s.worldToViewMatrix[i]=s.worldToViewMatrixPrev[i]=(i%5==0?1.f:0.f);
 // LH perspective, column-major, unjittered.
 s.viewToClipMatrix[0]=s.viewToClipMatrix[5]=1;
 s.viewToClipMatrix[10]=1.001f;s.viewToClipMatrix[11]=1;s.viewToClipMatrix[14]=-.1001f;
 std::memcpy(s.viewToClipMatrixPrev,s.viewToClipMatrix,sizeof(s.viewToClipMatrix));
 auto bad=f;bad.worldNormals=false;Require(!session.Prepare(bad,1),"view-space normals rejected");
 bad=f;bad.sameFrameGuides=false;Require(!session.Prepare(bad,1),"stale guides rejected");
 bad=f;bad.radianceContract=false;Require(!session.Prepare(bad,1),"unknown material modulation rejected");
 bad=f;bad.hitDistanceContract=false;Require(!session.Prepare(bad,1),"unknown hit distance rejected");
 bad=f;bad.settings.viewToClipMatrix[0]=0;Require(!session.Prepare(bad,1),"singular projection rejected");
 Require(session.Prepare(f,1),"initial plan");const auto restartCount=session.Plan().size();
 const auto&desc=session.Description();Require(desc.permanentPoolSize>0&&desc.transientPoolSize>0,"NRD owns complete history");
 for(const auto&d:session.Plan()){
  Require(d.pipeline<desc.pipelinesNum&&d.width&&d.height,"dispatch shape");
  Require(d.constants.size()<=desc.constantBufferMaxDataSize,"constant bounds");
  for(const auto&r:d.resources){
   if(r.type==nrd::ResourceType::PERMANENT_POOL)Require(r.indexInPool<desc.permanentPoolSize,"permanent index");
   if(r.type==nrd::ResourceType::TRANSIENT_POOL)Require(r.indexInPool<desc.transientPoolSize,"transient index");
  }
 }
 Require(!session.Prepare(f,1),"pending recording cannot advance NRD");
 Require(!session.Submitted(2,1),"wrong queue rejected");Require(session.Submitted(1,1),"actual submission");
 Require(!session.Prepare(f,1),"duplicate frame rejected");
 ++f.frame;Require(session.Prepare(f,1),"continuous plan");const auto continuousCount=session.Plan().size();
 Require(restartCount>continuousCount,"first frame includes pool clears");
 session.CancelAfterReset();Require(session.Prepare(f,1),"cancelled frame can be retried");
 Require(session.Plan().size()==restartCount,"cancel restarts all history");
 Require(session.Submitted(1,2),"retry submitted");
 f.frame+=3;Require(session.Prepare(f,1),"gap accepted with reset");Require(session.Plan().size()==restartCount,"gap clears history");
 Require(session.Submitted(1,3),"gap submitted");
 ++f.frame;++f.view;Require(session.Prepare(f,1),"new view");Require(session.Plan().size()==restartCount,"view switch clears history");
 std::printf("PASS actual NRD RELAX_DIFFUSE: %u pipelines, %u permanent / %u transient textures; %zu restart / %zu continuous dispatches; input guards, submission and reset lifecycle\n",desc.pipelinesNum,desc.permanentPoolSize,desc.transientPoolSize,restartCount,continuousCount);
}
