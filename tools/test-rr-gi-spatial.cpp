#include "../src/rr_gi_spatial_policy.h"
#include <cassert>
#include <cstdio>
struct Api {
 std::uintptr_t container=42,texture=99;bool readable=true,clearOkay=true;
 unsigned reads=0,textures=0,clears=0;
 bool Read(std::uintptr_t slot,std::uintptr_t& value){assert(slot==123);++reads;value=container;return readable;}
 std::uintptr_t Texture(std::uintptr_t p){assert(p==42);++textures;return texture;}
 bool Clear(std::uintptr_t p){assert(p==99);++clears;return clearOkay;}
};
int main(){using namespace control_rr_gi_spatial;
 for(unsigned fault=0;fault<7;++fault){Api a;auto slot=std::uintptr_t(123);
  if(fault==1)slot=124;if(fault==2)a.readable=false;if(fault==3)a.container=0;
  if(fault==4)a.texture=0;if(fault==5)a.clearOkay=false;if(fault==6)slot=0;
  auto r=Prepare(a,slot,123);
  assert(r==(fault==0?Result::Cleared:fault==3?Result::NativeInitialization:Result::Failed));
  assert(a.clears==unsigned(fault==0||fault==5));
  if(fault==1||fault==6)assert(a.reads==0);
  if(fault==2||fault==3)assert(a.textures==0);
 }
 puts("PASS GI history identity, native missing-history initialization, in-place clear and failure isolation");
}
