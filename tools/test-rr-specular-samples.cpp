#include "../src/rr_specular_sample_patch.h"
#include "../src/native_lighting_live_state.h"
#include <cassert>
#include <cstdio>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif
// Execute the actual emitted instruction relay against synthetic native memory.
// Return flags in the upper DWORD and the producer's R9D in the lower DWORD.
int main(){
#ifdef _WIN32
 auto* memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,8192,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
#else
 auto* memory=static_cast<unsigned char*>(mmap(nullptr,8192,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));
 assert(memory!=MAP_FAILED);
#endif
 assert(memory);
 auto* option=reinterpret_cast<unsigned*>(memory+4096);
 auto* state=reinterpret_cast<control_rr::SpecularSampleObservation*>(memory+4112);
 *state={};
 // xor eax,eax; stc sets CF/PF/ZF and clears SF/OF before the patched load.
 memory[0]=0x31;memory[1]=0xc0;memory[2]=0xf9;
 const std::uintptr_t site=reinterpret_cast<std::uintptr_t>(memory+3);
 const std::uintptr_t native=reinterpret_cast<std::uintptr_t>(option);
 unsigned char original[7]={0x44,0x8b,0x0d,0,0,0,0};
 const auto displacement=static_cast<std::int32_t>(native-site-7);std::memcpy(original+3,&displacement,4);
 control_rr::SpecularSampleCode code;
 assert(control_rr::BuildSpecularSampleCode(original,site,native,reinterpret_cast<std::uintptr_t>(memory+128),reinterpret_cast<std::uintptr_t>(state),code));
 std::memcpy(memory+3,code.replacement.data(),7);
 const unsigned char tail[]={0x9c,0x58,0x48,0xc1,0xe0,0x20,0x4c,0x09,0xc8,0xc3};
 std::memcpy(memory+10,tail,sizeof(tail));std::memcpy(memory+128,code.relay.data(),code.relay.size());
 control_rr::SpecularSampleCode rejected;
 assert(!control_rr::BuildSpecularSampleCode(original,site,native+4,reinterpret_cast<std::uintptr_t>(memory+128),reinterpret_cast<std::uintptr_t>(state),rejected));
 assert(!control_rr::BuildSpecularSampleCode(original,site,native,site+0x100000000ull,reinterpret_cast<std::uintptr_t>(state),rejected));
 original[0]=0x45;
 assert(!control_rr::BuildSpecularSampleCode(original,site,native,reinterpret_cast<std::uintptr_t>(memory+128),reinterpret_cast<std::uintptr_t>(state),rejected));
#ifdef _WIN32
 DWORD previous=0;assert(VirtualProtect(memory,4096,PAGE_EXECUTE_READ,&previous));
 assert(FlushInstructionCache(GetCurrentProcess(),memory,4096));
#else
 assert(mprotect(memory,4096,PROT_READ|PROT_EXEC)==0);
#endif
 using Producer=std::uint64_t(*)();auto producer=reinterpret_cast<Producer>(memory);
 for(unsigned n:{0u,1u,2u,8u,16u,17u,0xffffffffu}){
  for(unsigned request:{0u,1u,2u,4u,8u,12u,16u,17u,0xffffffffu}){
   *option=n;*state={request,0,0,0};const auto result=producer();
   const unsigned expected=n>=1&&n<=16&&request>=1&&request<=16?request:n;
   assert(static_cast<unsigned>(result)==expected);
   assert(((result>>32)&0x8c5)==0x45); // CF/PF/ZF preserved, OF/SF clear.
   assert(*option==n&&state->requested==request&&state->nativeCount==n&&state->effectiveCount==expected&&state->seen==1);
  }
 }
 using namespace native_lighting;
 LiveSelection live;
 for(auto mode:{Mode::RtSpotSpec1,Mode::RtSpotSpec2,Mode::RtSpotSpec4,Mode::RtSpotSpec8,Mode::RtSpotSpec12,Mode::RtSpotSpec16}){
  assert(IsSpecularSampleMode(mode)&&IsRtSpotMode(mode)&&UsesWorldAlbedo(mode)&&PreservesGiFiltering(mode));
  assert(!PreservesSpecularFiltering(mode)&&!UsesRawSpotVisibility(mode)&&!UsesPerspectiveGuide(mode));
  assert(GiSampleRequest(mode,6)==6&&RequiredVariant(mode)==kRtSpotReadyBit);
  const auto selection=live.Select(mode);assert(LiveSelection::GetMode(selection)==mode&&LiveSelection::Observed(selection)==0);
 }
 assert(SpecularSampleRequest(Mode::RtSpotWorldAlbedo)==0);
 assert(SpecularSampleRequest(Mode::Native)==0);
 puts("PASS: executed specular relay; 63 native/request combinations; flags and options preserved; invalid values pass through; exact-site/range rejection; mode policies");
}
